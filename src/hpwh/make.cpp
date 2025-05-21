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
                 const std::string& modelName,
                 double targetEF,
                 std::string sTestConfig,
                 std::string sOutputDir,
                 bool saveTestData,
                 std::string sResultsFilename,
                 std::string drawProfileName);

CLI::App* add_make(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("make", "Make a model with a specified EF");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "specification type (Preset)");

    static std::string modelName = "";
    subcommand->add_option("-m,--model", modelName, "model name")->required();

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

    static std::string drawProfileName = "";
    subcommand->add_option("-p,--profile", drawProfileName, "Draw profile");

    subcommand->callback(
        [&]()
        {
            make(sSpecType,
                 modelName,
                 targetUEF,
                 sTestConfig,
                 sOutputDir,
                 saveTestData,
                 sResultsFilename,
                 drawProfileName);
        });

    return subcommand;
}

void make(const std::string& sSpecType,
          const std::string& modelName,
          double targetEF,
          std::string sTestConfig,
          std::string sOutputDir,
          bool saveTestData,
          std::string sResultsFilename,
          std::string drawProfileName)
{
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

    HPWH hpwh;

    // process command line arguments
    std::string sSpecType_mod = (sSpecType != "") ? sSpecType : "Preset";
    for (auto& c : sSpecType_mod)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (sSpecType_mod == "preset")
        sSpecType_mod = "Preset";
    else if (sSpecType_mod == "json")
        sSpecType_mod = "JSON";
    else if (sSpecType_mod == "legacy")
        sSpecType_mod = "Legacy";

    // Parse the model
    if (sSpecType_mod == "Preset")
    {
        hpwh.initPreset(modelName);
    }
    else if (sSpecType_mod == "JSON")
    {
        hpwh.initFromJSON(modelName);
    }
    else if (sSpecType_mod == "Legacy")
    {
        hpwh.initLegacy(modelName);
    }
    else
    {
        std::cout << "Invalid argument, received '" << sSpecType_mod
                  << "', expected 'Preset' or 'JSON'.\n";
        exit(1);
    }

    std::string results = "";
    results.append(fmt::format("\tSpecification Type: {}\n", sSpecType_mod));
    results.append(fmt::format("\tModel Name: {}\n", modelName));

    auto designation = HPWH::FirstHourRating::Designation::Medium;
    if (drawProfileName != "")
    {
        bool foundProfile = false;
        for (auto& c : drawProfileName)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (drawProfileName.length() > 0)
        {
            drawProfileName[0] =
                static_cast<char>(std::toupper(static_cast<unsigned char>(drawProfileName[0])));
        }
        for (const auto& [key, value] : HPWH::FirstHourRating::DesignationMap)
        {
            if (value == drawProfileName)
            {
                designation = key;
                foundProfile = true;
                results.append(fmt::format("\tCustom Draw Profile: {}\n", drawProfileName));
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
        designation = firstHourRating.designation;
        results.append(firstHourRating.report());
    }

    hpwh.makeGenericEF(targetEF, testConfiguration, designation);

    std::ofstream outputFile;
    auto testSummary = hpwh.run24hrTest(testConfiguration, designation, saveTestData);
    if (saveTestData)
    {
        std::string sOutputFilename = "test24hr_" + sSpecType_mod + "_" + modelName + ".csv";
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
