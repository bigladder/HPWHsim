/*
 * test UEF calculation
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <string>

/* Evaluate UEF based on simulations using standard profiles */
static bool testCalcMetrics(const std::string& sModelName, HPWH::DailyTestSummary& dailyTestSummary)
{
    HPWH hpwh;

    HPWH::MODELS model = mapStringToPreset(sModelName);
    if (hpwh.HPWHinit_presets(model) != 0)
    {
        return false;
    }

    return hpwh.runDailyTest(hpwh.findUsageFromMaximumGPM_Rating(), dailyTestSummary);
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
        HPWH::DailyTestSummary dailyTestSummary;

        ASSERTTRUE(testCalcMetrics("AquaThermAire", dailyTestSummary));
        ASSERTTRUE(cmpd(dailyTestSummary.UEF, 2.8442));

        ASSERTTRUE(testCalcMetrics("AOSmithHPTS50", dailyTestSummary));
        ASSERTTRUE(cmpd(dailyTestSummary.UEF, 3.4366));

        return 0;
    }

    if (!validNumArgs)
    {
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

    sPresetOrFile[0] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(sPresetOrFile[0])));
    std::cout << "Spec type: " << sPresetOrFile << "\n";
    std::cout << "Model name: " << sModelName << "\n";

    HPWH::DailyTestSummary dailyTestSummary;
    if (hpwh.runDailyTest(hpwh.findUsageFromMaximumGPM_Rating(), dailyTestSummary))
    {
        std::cout << "\tRecovery Efficiency: " << dailyTestSummary.recoveryEfficiency << "\n";
        std::cout << "\tAdjusted Daily Water Heating Energy Consumption (kJ): " << dailyTestSummary.adjustedDailyWaterHeatingEnergyConsumption_kJ << "\n";
        std::cout << "\tUEF: " << dailyTestSummary.UEF << "\n";
        std::cout << "\tAnnual Electrical Energy Consumption (kJ): " << dailyTestSummary.annualElectricalEnergyConsumption_kJ << "\n";
        std::cout << "\tAnnual Energy Consumption (kJ): " << dailyTestSummary.annualEnergyConsumption_kJ << "\n";
   }
    else
    {
        std::cout << "Unable to determine efficiency metrics.\n";
    }

    return 0;
}
