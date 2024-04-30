/*
 * Utility to make HPWH generic models
 */

#include <cmath>
#include "HPWH.hh"

int main(int argc, char* argv[])
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

    bool validNumArgs = false;

    // process command line arguments
    std::string sPresetOrFile = "Preset";
    std::string sModelName;
    std::string sOutputDirectory;
    double targetUEF = 1.;
    if (argc == 5)
    {
        sPresetOrFile = argv[1];
        sModelName = argv[2];
        targetUEF = std::stod(argv[3]);
        standardTestOptions.sOutputDirectory = argv[4];
        validNumArgs = true;
    }

    if (!validNumArgs)
    {
        std::cout << "Invalid input:\n\
            To make a generic model, provide FOUR arguments:\n\
            \t[model spec Type, i.e., Preset (default) or File]\n\
            \t[model spec Name, i.e., Sanden80]\n\
            \t[target UEF, i.e., 3.1]\n\
            \t[output Directory, i.e., .\\output]\n\
            ";
        exit(1);
    }

    for (auto& c : sPresetOrFile)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    HPWH hpwh;
    bool validModel = false;
    if (sPresetOrFile == "preset")
    {
        if (hpwh.initPreset(sModelName) == 0)
        {
            validModel = true;
        }
    }
    else
    {
        std::string sInputFile = sModelName + ".txt";
        if (hpwh.initFromFile(sInputFile) == 0)
        {
            validModel = true;
        }
    }

    if (!validModel)
    {
        std::cout << "Invalid input: Model name not found.\n";
        exit(1);
    }

    std::cout << "Model name: " << sModelName << "\n";
    std::cout << "Target UEF: " << targetUEF << "\n";
    std::cout << "Output directory: " << sOutputDirectory << "\n\n";

    if (hpwh.makeGeneric(targetUEF))
    {
        std::cout << "Could not generate generic model.\n";
        exit(1);
    }

    sPresetOrFile[0] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(sPresetOrFile[0])));
    standardTestOptions.sOutputFilename = "test24hr_" + sPresetOrFile + "_" + sModelName + ".csv";

    return hpwh.measureMetrics(firstHourRating, standardTestOptions, standardTestSummary) ? 0 : 1;
}
