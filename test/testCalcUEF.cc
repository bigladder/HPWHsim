/*
 * test UEF calculation
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <string>

/* Evaluate UEF based on simulations using standard profiles */
static bool testCalcUEF(const std::string& sModelName, double& UEF)
{
    HPWH hpwh;

    HPWH::MODELS model = mapStringToPreset(sModelName);
    if (hpwh.HPWHinit_presets(model) != 0)
    {
        return false;
    }

    HPWH::DailyTestSummary dailyTestSummary;
    return hpwh.runDailyTest(hpwh.findUsageFromMaximumGPM_Rating(), dailyTestSummary);
    UEF = dailyTestSummary.UEF;
}

int main(int argc, char* argv[])
{
    bool validNumArgs = false;
    bool runUnitTests = false;

    // process command line arguments
    std::string sPresetOrFile = "Preset";
    std::string sModelName;
    if (argc == 1)
    {
        runUnitTests = true;
    }
    else if (argc == 2)
    {
        sModelName = argv[1];
        validNumArgs = true;
        runUnitTests = false;
    }
    else if (argc == 3)
    {
        sPresetOrFile = argv[1];
        sModelName = argv[2];
        validNumArgs = true;
        runUnitTests = false;
    }

    if (runUnitTests)
    {
        double UEF;
        ASSERTTRUE(testCalcUEF("AOSmithHPTS50", UEF));
        ASSERTTRUE(cmpd(UEF, 4.4091));

        ASSERTTRUE(testCalcUEF("AquaThermAire", UEF));
        ASSERTTRUE(cmpd(UEF, 3.5848));

        return 0;
    }

    if (!validNumArgs) {
        cout << "Invalid input:\n\
            To run all unit tests, provide ZERO arguments.\n\
            To determine the UEF for a particular model spec, provide ONE or TWO arguments:\n\
            \t[model spec Type (i.e., Preset (default) or File)]\n\
            \t[model spec Name (i.e., Sanden80)]\n";
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
        HPWH::MODELS model = mapStringToPreset(sModelName);
        if (hpwh.HPWHinit_presets(model) == 0)
        {
            validModel = true;
        }
    }
    else
    {
        std::string inputFile = sModelName + ".txt";
        if (hpwh.HPWHinit_file(inputFile) == 0)
        {
            validModel = true;
        }
    }

    if (!validModel)
    {
        cout << "Invalid input: Model name not found.\n";
        exit(1);
    }

    sPresetOrFile[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(sPresetOrFile[0])));
    std::cout << "Spec type: " << sPresetOrFile << "\n";
    std::cout << "Model name: " << sModelName << "\n";

    HPWH::DailyTestSummary dailyTestSummary;
    if(hpwh.runDailyTest(hpwh.findUsageFromMaximumGPM_Rating(), dailyTestSummary))
    {
        std::cout << "UEF: " << dailyTestSummary.UEF << "\n";
    }
    else
    {
        std::cout << "Unable to determine UEF.\n";
    }

    return 0;
}
