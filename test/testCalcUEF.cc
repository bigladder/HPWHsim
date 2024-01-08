/*
 * test UEF calculation
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <string>

/* Evaluate UEF based on simulations using standard profiles */
static bool testCalcMetrics(const std::string& sModelName,
                            HPWH::Usage& usage,
                            HPWH::DailyTestSummary& dailyTestSummary)
{
    HPWH hpwh;

    HPWH::MODELS model = mapStringToPreset(sModelName);
    if (hpwh.HPWHinit_presets(model) != 0)
    {
        return false;
    }

    if (!hpwh.findUsageFromFirstHourRating(usage))
        return false;

    return hpwh.runDailyTest(usage, dailyTestSummary);
}

int main(int argc, char* argv[])
{
    HPWH::Usage usage;
    HPWH::DailyTestSummary dailyTestSummary;
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
        ASSERTTRUE(testCalcMetrics("AquaThermAire", usage, dailyTestSummary));
        ASSERTTRUE(usage == HPWH::Usage::High);
        ASSERTTRUE(cmpd(dailyTestSummary.UEF, 3.2797));

        ASSERTTRUE(testCalcMetrics("AOSmithHPTS50", usage, dailyTestSummary));
        ASSERTTRUE(usage == HPWH::Usage::Medium);
        ASSERTTRUE(cmpd(dailyTestSummary.UEF, 4.8246));

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

    if (hpwh.findUsageFromFirstHourRating(usage))
    {
        std::string sUsage = "";
        switch (usage)
        {
        case HPWH::Usage::VerySmall:
        {
            sUsage = "Very Small";
            break;
        }
        case HPWH::Usage::Low:
        {
            sUsage = "Low";
            break;
        }
        case HPWH::Usage::Medium:
        {
            sUsage = "Medium";
            break;
        }
        case HPWH::Usage::High:
        {
            sUsage = "High";
            break;
        }
        }
        std::cout << "\tUsage: " << sUsage << "\n";

        if (hpwh.runDailyTest(usage, dailyTestSummary))
        {
            std::cout << "\tRecovery Efficiency: " << dailyTestSummary.recoveryEfficiency << "\n";
            std::cout << "\tUEF: " << dailyTestSummary.UEF << "\n";
            std::cout << "\tAdjusted Daily Water Heating Energy Consumption (kWh): "
                      << KJ_TO_KWH(dailyTestSummary.adjustedDailyWaterHeatingEnergyConsumption_kJ) << "\n";
            std::cout << "\tAnnual Electrical Energy Consumption (kWh): "
                      << KJ_TO_KWH(dailyTestSummary.annualElectricalEnergyConsumption_kJ) << "\n";
            std::cout << "\tAnnual Energy Consumption (kWh): "
                      << KJ_TO_KWH(dailyTestSummary.annualEnergyConsumption_kJ) << "\n";
        }
        else
        {
            std::cout << "Unable to determine efficiency metrics.\n";
        }
    }
    else
    {
        std::cout << "Unable to determine first-hour rating.\n";
    }

    return 0;
}
