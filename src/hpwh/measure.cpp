/*
 * Measure the performance metrics of a HPWH model
 */

#include <CLI/CLI.hpp>
#include "HPWH.hh"

namespace hpwh_cli
{

/// measure
static void measure(const std::string& sSpecType,
                    const std::string& sModelName,
                    std::string sOutputDir,
                    bool sSupressOutput,
                    std::string sResultsFilename,
                    std::string sCustomDrawProfile,
                    std::string sTestConfig);

CLI::App* add_measure(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("measure", "Measure the metrics for a model");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File)");

    static std::string sModelName = "";
    subcommand->add_option("-m,--model", sModelName, "Model name")->required();

    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    static bool noData = false;
    subcommand->add_flag("-n,--no_data", noData, "Suppress data output");

    static std::string sResultsFilename = "";
    subcommand->add_option("-r,--results", sResultsFilename, "Results filename");

    static std::string sCustomDrawProfile = "";
    subcommand->add_option("-p,--profile", sCustomDrawProfile, "Custom draw profile");

    static std::string sTestConfig = "UEF";
    subcommand->add_option("-c,--config", sTestConfig, "test configuration");

    subcommand->callback(
        [&]()
        {
            measure(sSpecType,
                    sModelName,
                    sOutputDir,
                    noData,
                    sResultsFilename,
                    sCustomDrawProfile,
                    sTestConfig);
        });

    return subcommand;
}

void measure(const std::string& sSpecType,
             const std::string& sModelName,
             std::string sOutputDir,
             bool sSupressOutput,
             std::string sResultsFilename,
             std::string sCustomDrawProfile,
             std::string sTestConfig)
{
    HPWH::TestOptions testOptions;
    testOptions.saveOutput = false;
    testOptions.sOutputFilename = "";
    testOptions.sOutputDirectory = "";
    testOptions.resultsStream = NULL;

    bool useResultsFile = false;
    std::string sPresetOrFile = (sSpecType != "") ? sSpecType : "Preset";
    if (sOutputDir != "")
    {
        testOptions.saveOutput = true;
        testOptions.sOutputDirectory = sOutputDir;

        if (sResultsFilename != "")
        {
            std::ostream* tempStream = new std::ofstream;
            std::ofstream* resultsFile = static_cast<std::ofstream*>(tempStream);
            resultsFile->open(sResultsFilename.c_str(), std::ofstream::out | std::ofstream::trunc);
            if (resultsFile->is_open())
            {
                testOptions.resultsStream = resultsFile;
                useResultsFile = true;
            }
        }
    }
    testOptions.saveOutput = !sSupressOutput;
    bool useCustomDrawProfile = (sCustomDrawProfile != "");

    for (auto& c : sPresetOrFile)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (sPresetOrFile.length() > 0)
    {
        sPresetOrFile[0] =
            static_cast<char>(std::toupper(static_cast<unsigned char>(sPresetOrFile[0])));
    }

    HPWH hpwh;
    if (sPresetOrFile == "Preset")
    {
        hpwh.initPreset(sModelName);
    }
    else
    {
        std::string inputFile = sModelName;
        hpwh.initFromFile(inputFile);
    }

    sPresetOrFile[0] = // capitalize first char
        static_cast<char>(std::toupper(static_cast<unsigned char>(sPresetOrFile[0])));

    std::string results = "";
    results.append(fmt::format("\tSpecification Type: {}\n", sPresetOrFile));
    results.append(fmt::format("\tModel Name: {}\n", sModelName));

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
            hpwh.get_courier()->send_error("Invalid input: Draw profile name not found.");
            exit(1);
        }
    }
    else
    {
        auto firstHourRating = hpwh.findFirstHourRating();
        results.append(firstHourRating.report());
    }

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

    testOptions.sOutputFilename = "test24hr_" + sPresetOrFile + "_" + sModelName + ".csv";

    auto testSummary = hpwh.measureMetrics(testOptions);
    results.append(testSummary.report());

    hpwh.get_courier()->send_info("\n" + results);
    if (testOptions.resultsStream)
        *testOptions.resultsStream << results;

    if (useResultsFile)
    {
        delete testOptions.resultsStream;
    }
}
} // namespace hpwh_cli
