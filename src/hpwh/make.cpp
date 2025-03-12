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
                 double ambientT_C,
                 double targetUEF,
                 std::string sOutputDir,
                 std::string sCustomDrawProfile);

CLI::App* add_make(CLI::App& app)
{
    const auto subcommand = app.add_subcommand("make", "Make a model with a specified UEF");

    static std::string sSpecType = "Preset";
    subcommand->add_option("-s,--spec", sSpecType, "Specification type (Preset, File)");

    static std::string sModelName = "";
    subcommand->add_option("-m,--model", sModelName, "Model name")->required();

    static double targetUEF = -1.;
    subcommand->add_option("-u,--uef", targetUEF, "target UEF")->required();

    static double ambientT_C = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435
    subcommand->add_option("-a,--amb", ambientT_C, "ambientT (degC)");

    static std::string sOutputDir = ".";
    subcommand->add_option("-d,--dir", sOutputDir, "Output directory");

    static std::string sCustomDrawProfile = "";
    subcommand->add_option("-p,--profile", sCustomDrawProfile, "Custom draw profile");

    subcommand->callback(
        [&]()
        { make(sSpecType, sModelName, targetUEF, ambientT_C, sOutputDir, sCustomDrawProfile); });

    return subcommand;
}

void make(const std::string& sSpecType,
          const std::string& sModelName,
          double targetUEF,
          double ambientT_C,
          std::string sOutputDir,
          std::string sCustomDrawProfile)
{
    HPWH::FirstHourRating firstHourRating;
    HPWH::StandardTestSummary standardTestSummary;

    HPWH::StandardTestOptions standardTestOptions;
    standardTestOptions.saveOutput = true;
    standardTestOptions.sOutputFilename = "";
    standardTestOptions.sOutputDirectory = "";
    standardTestOptions.outputStream = &std::cout;
    standardTestOptions.changeSetpoint = true;
    standardTestOptions.nTestTCouples = 6;
    standardTestOptions.setpointT_C = 51.7;

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

    HPWH::GenericOptions genericOptions;

    HPWH::UEF_MeritInput uef_merit(targetUEF, ambientT_C);
    genericOptions.meritInputs.push_back(&uef_merit);

    HPWH::COP_CoefInput copCoeffInput0(1, 0);
    genericOptions.paramInputs.push_back(&copCoeffInput0);

    HPWH::COP_CoefInput copCoeffInput1(1, 1);
    genericOptions.paramInputs.push_back(&copCoeffInput1);

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

    std::cout << "Model name: " << sModelName << "\n";
    std::cout << "Target UEF: " << targetUEF << "\n";
    std::cout << "Output directory: " << standardTestOptions.sOutputDirectory << "\n\n";

    hpwh.makeGeneric(genericOptions, standardTestOptions);

    sPresetOrFile[0] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(sPresetOrFile[0])));
    standardTestOptions.sOutputFilename = "test24hr_" + sPresetOrFile + "_" + sModelName + ".csv";

    hpwh.measureMetrics(firstHourRating, standardTestOptions, standardTestSummary);
}
} // namespace hpwh_cli
