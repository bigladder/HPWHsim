/*
 * Utility to make HPWH generic models
 */

#include <cmath>
#include "HPWH.hh"

void makeCommand(const std::string& sSpecType,
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
        std::string sInputFile = sModelName + ".txt";
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
