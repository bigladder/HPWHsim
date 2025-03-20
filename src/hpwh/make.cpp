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

    static std::string sCustomDrawProfile = "";
    subcommand->add_option("-p,--profile", sCustomDrawProfile, "Custom draw profile");

    subcommand->callback(
        [&]()
        { make(sSpecType, sModelName, targetUEF, sTestConfig, sOutputDir, sCustomDrawProfile); });

    return subcommand;
}

void make(const std::string& sSpecType,
          const std::string& sModelName,
          double targetEF,
          std::string sTestConfig,
          std::string sOutputDir,
          std::string sCustomDrawProfile)
{
    HPWH::TestOptions testOptions;
    testOptions.saveOutput = true;
    testOptions.sOutputFilename = "";
    testOptions.sOutputDirectory = "";
    testOptions.resultsStream = NULL;

    // process command line arguments
    std::string sPresetOrFile = (sSpecType != "") ? sSpecType : "Preset";
    testOptions.sOutputDirectory = sOutputDir;

    // select test configuration
    transform(sTestConfig.begin(),
              sTestConfig.end(),
              sTestConfig.begin(),
              ::toupper); // make uppercase
    if (sTestConfig == "E50")
        testOptions.testConfiguration = HPWH::testConfiguration_E50;
    else if (sTestConfig == "E95")
        testOptions.testConfiguration = HPWH::testConfiguration_E95;
    else
        testOptions.testConfiguration = HPWH::testConfiguration_UEF;

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

    bool useCustomDrawProfile = (sCustomDrawProfile != "");
    if (useCustomDrawProfile)
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
                testOptions.desig = key;
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
        results.append(firstHourRating.report());
    }

    hpwh.makeGenericEF(targetEF, testOptions);

    testOptions.sOutputFilename = "test24hr_" + sPresetOrFile + "_" + sModelName + ".csv";

    auto testSummary = hpwh.measureMetrics(testOptions);
    results.append(testSummary.report());

    hpwh.get_courier()->send_info("\n" + results);
    if (testOptions.resultsStream)
        *testOptions.resultsStream << results;
}
} // namespace hpwh_cli
