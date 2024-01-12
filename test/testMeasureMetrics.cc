/*
 * test measure performance metrics
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <string>

/* Measure metrics using 24-hr test based on model name (preset only) */
static bool testMeasureMetrics(const std::string& sModelName,
                               HPWH::FirstHourRating& firstHourRating,
                               HPWH::StandardTestSummary& standardTestSummary)
{
    HPWH hpwh;

    HPWH::MODELS model = mapStringToPreset(sModelName);
    if (hpwh.HPWHinit_presets(model) != 0)
    {
        return false;
    }

    if (!hpwh.findFirstHourRating(firstHourRating))
    {
        return false;
    }

    return hpwh.run24hrTest(firstHourRating, standardTestSummary);
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
        HPWH::FirstHourRating firstHourRating;
        ASSERTTRUE(testMeasureMetrics("AquaThermAire", firstHourRating, standardTestSummary));
        ASSERTTRUE(standardTestSummary.qualifies);
        ASSERTTRUE(firstHourRating == HPWH::FirstHourRating::Medium);
        ASSERTTRUE(cmpd(standardTestSummary.UEF, 3.2212));

        ASSERTTRUE(testMeasureMetrics("AOSmithHPTS50", firstHourRating, standardTestSummary));
        ASSERTTRUE(standardTestSummary.qualifies);
        ASSERTTRUE(firstHourRating == HPWH::FirstHourRating::Low);
        ASSERTTRUE(cmpd(standardTestSummary.UEF, 4.4914));

        ASSERTTRUE(testMeasureMetrics("AOSmithHPTS80", firstHourRating, standardTestSummary));
        ASSERTTRUE(standardTestSummary.qualifies);
        ASSERTTRUE(firstHourRating == HPWH::FirstHourRating::High);
        ASSERTTRUE(cmpd(standardTestSummary.UEF, 3.5230));

        return 0;
    }

    if (!validNumArgs)
    {
        cout << "Invalid input:\n\
            To run all unit tests, provide ZERO arguments.\n\
            To determine performance metrics for a particular model spec, provide ONE or TWO arguments:\n\
            \t[model spec Type, i.e., Preset (default) or File]\n\
            \t[model spec Name, i.e., Sanden80]\n";
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

    HPWH::FirstHourRating firstHourRating;
    if (hpwh.findFirstHourRating(firstHourRating))
    {
        std::string sFirstHourRating = "";
        switch (firstHourRating)
        {
        case HPWH::FirstHourRating::VerySmall:
        {
            sFirstHourRating = "Very Small";
            break;
        }
        case HPWH::FirstHourRating::Low:
        {
            sFirstHourRating = "Low";
            break;
        }
        case HPWH::FirstHourRating::Medium:
        {
            sFirstHourRating = "Medium";
            break;
        }
        case HPWH::FirstHourRating::High:
        {
            sFirstHourRating = "High";
            break;
        }
        }
        std::cout << "\tFirst-Hour Rating: " << sFirstHourRating << "\n";

        if (hpwh.run24hrTest(firstHourRating, standardTestSummary))
        {

            if (!standardTestSummary.qualifies)
            {
                std::cout << "\tDoes not qualify as consumer water heater.\n";
            }

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
