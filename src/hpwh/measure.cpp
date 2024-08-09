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
                    std::string sCustomDrawProfile);

CLI::App* add_measure(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("measure", "Measure the metrics for a model");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File)");

    static std::string sModelName = "";
    subcommand->add_option("-m,--model", sModelName, "Model name")->required();

    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    static std::string sCustomDrawProfile = "";
    subcommand->add_option("-p,--profile", sCustomDrawProfile, "Custom draw profile");

    subcommand->callback([&]() { measure(sSpecType, sModelName, sOutputDir, sCustomDrawProfile); });

    return subcommand;
}

void measure(const std::string& sSpecType,
             const std::string& sModelName,
             std::string sOutputDir,
             std::string sCustomDrawProfile)
{
    HPWH::StandardTestSummary standardTestSummary;

    HPWH::StandardTestOptions standardTestOptions;
    standardTestOptions.saveOutput = false;
    standardTestOptions.sOutputFilename = "";
    standardTestOptions.sOutputDirectory = "";
    standardTestOptions.changeSetpoint = true;
    standardTestOptions.nTestTCouples = 6;
    standardTestOptions.setpointT_C = 51.7;

    // process command line arguments
    std::string sPresetOrFile = (sSpecType != "") ? sSpecType : "Preset";
    if (sOutputDir != "")
    {
        standardTestOptions.saveOutput = true;
        standardTestOptions.sOutputDirectory = sOutputDir;
    }
    standardTestOptions.saveOutput = true;
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
                hpwh.customTestOptions.overrideFirstHourRating = true;
                hpwh.customTestOptions.desig = key;
                foundProfile = true;
                break;
            }
        }
        if (!foundProfile)
        {
            std::cout << "Invalid input: Draw profile name not found.\n";
            exit(1);
        }
    }

    std::cout << "Spec type: " << sPresetOrFile << "\n";
    std::cout << "Model name: " << sModelName << "\n";

    standardTestOptions.sOutputFilename = "test24hr_" + sPresetOrFile + "_" + sModelName + ".csv";

    HPWH::FirstHourRating firstHourRating;
    hpwh.measureMetrics(firstHourRating, standardTestOptions, standardTestSummary);
}
} // namespace hpwh_cli
