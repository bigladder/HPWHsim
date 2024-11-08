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
    standardTestOptions.setpointT_C = 51.7;

    HPWH hpwh;

    // process command line arguments
    std::string sSpecType_mod = (sSpecType != "") ? sSpecType : "Preset";
    for (auto& c : sSpecType_mod)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (sSpecType_mod == "preset")
        sSpecType_mod = "Preset";
    else if (sSpecType_mod == "file")
        sSpecType_mod = "File";
    else if (sSpecType_mod == "json")
        sSpecType_mod = "JSON";

    if (sSpecType_mod == "Preset")
    {
        hpwh.initPreset(sModelName);
    }
    else if (sSpecType_mod == "File")
    {
        hpwh.initFromFile(sModelName);
    }
    else if (sSpecType_mod == "JSON")
    {
        hpwh.initFromJSON(sModelName);
    }

    standardTestOptions.sOutputDirectory = sOutputDir;

    std::cout << "Model name: " << sModelName << "\n";
    std::cout << "Target UEF: " << targetUEF << "\n";
    std::cout << "Output directory: " << standardTestOptions.sOutputDirectory << "\n\n";

    hpwh.makeGeneric(targetUEF);

    standardTestOptions.sOutputFilename = "test24hr_" + sSpecType_mod + "_" + sModelName + ".csv";

    hpwh.measureMetrics(firstHourRating, standardTestOptions, standardTestSummary);
}
} // namespace hpwh_cli
