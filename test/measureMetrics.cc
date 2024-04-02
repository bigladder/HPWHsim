/*
 * Utility to measure HPWH performance metrics
 */

#include "HPWH.hh"

int main(int argc, char* argv[])
{
    HPWH::StandardTestSummary standardTestSummary;
    bool validNumArgs = false;

    HPWH::StandardTestOptions standardTestOptions;
    standardTestOptions.saveOutput = false;
    standardTestOptions.sOutputFilename = "";
    standardTestOptions.sOutputDirectory = "";
    standardTestOptions.changeSetpoint = true;
    standardTestOptions.nTestTCouples = 6;
    standardTestOptions.setpointT_C = 51.7;

    // process command line arguments
    std::string sPresetOrFile = "Preset";
    std::string sModelName;
    bool useCustomDrawProfile = false;
    std::string sCustomDrawProfile;
    if (argc == 2)
    {
        sModelName = argv[1];
        validNumArgs = true;
    }
    else if (argc == 3)
    {
        sPresetOrFile = argv[1];
        sModelName = argv[2];
        validNumArgs = true;
    }
    else if (argc == 4)
    {
        sPresetOrFile = argv[1];
        sModelName = argv[2];
        standardTestOptions.sOutputDirectory = argv[3];
        validNumArgs = true;
        standardTestOptions.saveOutput = true;
    }
    else if (argc == 5)
    {
        sPresetOrFile = argv[1];
        sModelName = argv[2];
        standardTestOptions.sOutputDirectory = argv[3];
        sCustomDrawProfile = argv[4];
        useCustomDrawProfile = true;
        validNumArgs = true;
        standardTestOptions.saveOutput = true;
    }

    if (!validNumArgs)
    {
        std::cout << "Invalid input:\n\
            To determine performance metrics for a particular model spec, provide ONE, TWO, THREE, or FOUR arguments:\n\
            \t[model spec Type, i.e., Preset (default) or File]\n\
            \t[model spec Name, i.e., Sanden80]\n\
            \t[output Directory, i.e., .\\output]\n\
            \t[draw profile, i.e., Medium]\n";
        exit(1);
    }

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
    bool validModel = false;
    if (sPresetOrFile == "Preset")
    {
        if (hpwh.initPreset(sModelName) == 0)
        {
            validModel = true;
        }
    }
    else
    {
        std::string inputFile = sModelName + ".txt";
        if (hpwh.initFromFile(inputFile) == 0)
        {
            validModel = true;
        }
    }

    if (!validModel)
    {
        std::cout << "Invalid input: Model name not found.\n";
        exit(1);
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
    return hpwh.measureMetrics(firstHourRating, standardTestOptions, standardTestSummary) ? 0 : 1;
}
