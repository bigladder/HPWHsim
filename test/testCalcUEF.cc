/*
 * test UEF calculation
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <string>

/* Measure metrics based on simulations using standard profiles */
static bool testMeasureMetrics(const std::string& sModelName,
                            HPWH::StandardTestSummary& standardTestSummary)
{
    HPWH hpwh;

    HPWH::MODELS model = mapStringToPreset(sModelName);
    if (hpwh.HPWHinit_presets(model) != 0)
    {
        return false;
    }

    if (!hpwh.findUsageFromFirstHourRating(standardTestSummary))
    {
        return false;
    }

    return hpwh.run24hrTest(standardTestSummary);
}

int main(int argc, char* argv[])
{
    HPWH::StandardTestSummary standardTestSummary;
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
        ASSERTTRUE(testMeasureMetrics("AquaThermAire", standardTestSummary));
        ASSERTTRUE(standardTestSummary.qualifies );
        ASSERTTRUE(standardTestSummary.usage == HPWH::Usage::Medium);
        ASSERTTRUE(cmpd(standardTestSummary.UEF, 2.6326));

        ASSERTTRUE(testMeasureMetrics("AOSmithHPTS50", standardTestSummary));
        ASSERTTRUE(standardTestSummary.qualifies );
        ASSERTTRUE(standardTestSummary.usage == HPWH::Usage::Low);
        ASSERTTRUE(cmpd(standardTestSummary.UEF, 4.4914));

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

    if (hpwh.findUsageFromFirstHourRating(standardTestSummary))
    {
        if (standardTestSummary.qualifies)
        {
            std::string sUsage = "";
            switch (standardTestSummary.usage)
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
        }
        else
        {
            std::cout << "\tDoes not qualify as consumer water heater.\n";
        }

        if (hpwh.run24hrTest(standardTestSummary))
        {

            std::cout << "\tRecovery Efficiency: " << standardTestSummary.recoveryEfficiency
                      << "\n";
            std::cout << "\tUEF: " << standardTestSummary.UEF << "\n";
            std::cout << "\tAdjusted Daily Water Heating Energy Consumption (kWh): "
                      << KJ_TO_KWH(
                             standardTestSummary.adjustedDailyWaterHeatingEnergyConsumption_kJ)
                      << "\n";
            std::cout << "\tAnnual Electrical Energy Consumption (kWh): "
                      << KJ_TO_KWH(standardTestSummary.annualElectricalEnergyConsumption_kJ)
                      << "\n";
            std::cout << "\tAnnual Energy Consumption (kWh): "
                      << KJ_TO_KWH(standardTestSummary.annualEnergyConsumption_kJ) << "\n";
        }
        else
        {
            std::cout << "Unable to complete 24-hr test.\n";
        }
    }
    else
    {
        std::cout << "Unable to complete first-hour rating test.\n";
    }

    return 0;
}
