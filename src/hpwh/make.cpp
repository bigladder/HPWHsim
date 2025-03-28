/*
 * Make a HPWH generic model with specified UEF
 */

#include <cmath>
#include "HPWH.hh"
#include "HPWHFitter.hh"
#include <CLI/CLI.hpp>

namespace hpwh_cli
{

/// make
static void make(const std::string& sSpecType,
                 const std::string& sModelName,
                 double targetEF,
                 std::string sTestConfig,
                 std::string sOutputDir,
                 bool saveTestData,
                 std::string sResultsFilename,
                 std::string sCustomDrawProfile);

CLI::App* add_make(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("make", "Make a model with a specified EF");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "specification type (Preset, File)");

    static std::string sModelName = "";
    subcommand->add_option("-m,--model", sModelName, "model name")->required();

    static double targetUEF = -1.;
    subcommand->add_option("-e,--ef", targetUEF, "target EF")->required();

    static std::string sTestConfig = "UEF";
    subcommand->add_option("-c,--config", sTestConfig, "test configuration");

    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    static bool saveTestData = true;
    subcommand->add_flag("-v,--save_data", saveTestData, "Save test data");

    static std::string sResultsFilename = "";
    subcommand->add_option("-r,--results", sResultsFilename, "Results filename");

    static std::string sCustomDrawProfile = "";
    subcommand->add_option("-p,--profile", sCustomDrawProfile, "Custom draw profile");

    subcommand->callback(
        [&]()
        {
            make(sSpecType,
                 sModelName,
                 targetUEF,
                 sTestConfig,
                 sOutputDir,
                 saveTestData,
                 sResultsFilename,
                 sCustomDrawProfile);
        });

    return subcommand;
}

void make(const std::string& sSpecType,
          const std::string& sModelName,
          double targetEF,
          std::string sTestConfig,
          std::string sOutputDir,
          bool saveTestData,
          std::string sResultsFilename,
          std::string sCustomDrawProfile)
{
    // process command line arguments
    std::string sPresetOrFile = (sSpecType != "") ? sSpecType : "Preset";

    // select test configuration
    transform(sTestConfig.begin(),
              sTestConfig.end(),
              sTestConfig.begin(),
              ::toupper); // make uppercase

    HPWH::TestConfiguration testConfiguration;
    if (sTestConfig == "E50")
        testConfiguration = HPWH::testConfiguration_E50;
    else if (sTestConfig == "E95")
        testConfiguration = HPWH::testConfiguration_E95;
    else
        testConfiguration = HPWH::testConfiguration_UEF;

    for (auto& c : sPresetOrFile)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    HPWH hpwh;
    if (sPresetOrFile == "preset")
    {
        hpwh.initPreset(sModelName);
    }
    else
    {
        std::string sInputFile = sModelName;
        hpwh.initFromFile(sInputFile);
    }

    sPresetOrFile[0] = // capitalize first char
        static_cast<char>(std::toupper(static_cast<unsigned char>(sPresetOrFile[0])));

    std::string results = "";
    results.append(fmt::format("\tSpecification Type: {}\n", sPresetOrFile));
    results.append(fmt::format("\tModel Name: {}\n", sModelName));

    auto desig = HPWH::FirstHourRating::Desig::Medium;
    if (sCustomDrawProfile != "")
    {
        bool foundProfile = false;
        for (auto& c : sCustomDrawProfile)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (sCustomDrawProfile.length() > 0)
        {
            sCustomDrawProfile[0] =
                static_cast<char>(std::toupper(static_cast<unsigned char>(sCustomDrawProfile[0])));
        }
        for (const auto& [key, value] : HPWH::FirstHourRating::sDesigMap)
        {
            if (value == sCustomDrawProfile)
            {
                desig = key;
                foundProfile = true;
                results.append(fmt::format("\tCustom Draw Profile: {}\n", sCustomDrawProfile));
                break;
            }
        }
        if (!foundProfile)
        {
            std::cout << "Invalid input: Draw profile name not found.\n";
            exit(1);
        }
    }
    else
    {
        auto firstHourRating = hpwh.findFirstHourRating();
        desig = firstHourRating.desig;
        results.append(firstHourRating.report());
    }

    hpwh.makeGenericEF(targetEF, testConfiguration, desig);

    std::ofstream outputFile;
    auto testSummary = hpwh.run24hrTest(testConfiguration, desig, saveTestData);
    if (saveTestData)
    {
        std::string sOutputFilename = "test24hr_" + sPresetOrFile + "_" + sModelName + ".csv";
        if (sOutputDir != "")
            sOutputFilename = sOutputDir + "/" + sOutputFilename;
        outputFile.open(sOutputFilename.c_str(), std::ifstream::out);
        if (!outputFile.is_open())
        {
            hpwh.get_courier()->send_error(
                fmt::format("Could not open output file {}", sOutputFilename));
        }
    }

    results.append(testSummary.report());

    hpwh.get_courier()->send_info("\n" + results);
    if (sResultsFilename != "")
    {
        auto resultsStream = std::make_shared<std::ofstream>();
        if (sOutputDir != "")
            sResultsFilename = sOutputDir + "/" + sResultsFilename;
        resultsStream->open(sResultsFilename.c_str(), std::ofstream::out | std::ofstream::trunc);
        if (resultsStream->is_open())
        {
            *resultsStream << results;
        }
    }
}
} // namespace hpwh_cli
