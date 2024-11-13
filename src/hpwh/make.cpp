/*
 * Make a HPWH generic model with specified UEF
 */

#include <cmath>
#include "HPWH.hh"
#include <CLI/CLI.hpp>

namespace hpwh_cli
{

/// make
static void make(const std::string& sSpecType,
                 const std::string& sModelName,
                 double targetUEF,
                 std::string sOutputDir);

CLI::App* add_make(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("make", "Make a model with a specified UEF");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File)");

    static std::string sModelName = "";
    subcommand->add_option("-m,--model", sModelName, "Model name")->required();

    static double targetUEF = -1.;
    subcommand->add_option("-u,--uef", targetUEF, "target UEF")->required();

    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    subcommand->callback([&]() { make(sSpecType, sModelName, targetUEF, sOutputDir); });

    return subcommand;
}

void make(const std::string& sSpecType,
          const std::string& sModelName,
          double targetUEF,
          std::string sOutputDir)
{
    HPWH::FirstHourRating firstHourRating;
    HPWH::StandardTestSummary standardTestSummary;

    HPWH::StandardTestOptions standardTestOptions;
    standardTestOptions.saveOutput = true;
    standardTestOptions.sOutputFilename = "";
    standardTestOptions.sOutputDirectory = "";
    standardTestOptions.changeSetpoint = true;
    standardTestOptions.nTestTCouples = 6;
    standardTestOptions.setpointT = {51.7, Units::F};

    // process command line arguments
    std::string sPresetOrFile = (sSpecType != "") ? sSpecType : "Preset";
    standardTestOptions.sOutputDirectory = sOutputDir;

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

    std::cout << "Model name: " << sModelName << "\n";
    std::cout << "Target UEF: " << targetUEF << "\n";
    std::cout << "Output directory: " << standardTestOptions.sOutputDirectory << "\n\n";

    hpwh.makeGeneric(targetUEF);

    sPresetOrFile[0] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(sPresetOrFile[0])));
    standardTestOptions.sOutputFilename = "test24hr_" + sPresetOrFile + "_" + sModelName + ".csv";

    hpwh.measureMetrics(firstHourRating, standardTestOptions, standardTestSummary);
}
} // namespace hpwh_cli
