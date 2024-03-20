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
    standardTestOptions.changeSetpoint = true;
    standardTestOptions.nTestTCouples = 6;
    standardTestOptions.setpointT_C = 51.7;

    // process command line arguments
    std::string sPresetOrFile = "Preset";
    std::string sModelName;
    std::string sOutputDirectory;
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
        sOutputDirectory = argv[3];
        validNumArgs = true;
        standardTestOptions.saveOutput = true;
    }

    if (!validNumArgs)
    {
        std::cout << "Invalid input:\n\
            To determine performance metrics for a particular model spec, provide ONE, TWO, or THREE arguments:\n\
            \t[model spec Type, i.e., Preset (default) or File]\n\
            \t[model spec Name, i.e., Sanden80]\n\
            \t[output Directory, i.e., .\\output]\n";
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

    sPresetOrFile[0] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(sPresetOrFile[0])));
    std::cout << "Spec type: " << sPresetOrFile << "\n";
    std::cout << "Model name: " << sModelName << "\n";

    if (standardTestOptions.saveOutput)
    {
        std::string sOutputFilename = "test24hr_" + sPresetOrFile + "_" + sModelName + ".csv";

        std::string sFullOutputFilename = sOutputDirectory + "/" + sOutputFilename;
        standardTestOptions.outputFile.open(sFullOutputFilename.c_str(), std::ifstream::out);
        if (!standardTestOptions.outputFile.is_open())
        {
            std::cout << "Could not open output file " << sFullOutputFilename << "\n";
            exit(1);
        }
        std::cout << "Output file: " << sFullOutputFilename << "\n";

        std::string strPreamble;
        std::string sHeader = "minutes,Ta,Tsetpoint,inletT,draw,";

        int csvOptions = HPWH::CSVOPT_NONE;
        hpwh.WriteCSVHeading(standardTestOptions.outputFile,
                             sHeader.c_str(),
                             standardTestOptions.nTestTCouples,
                             csvOptions);
    }

    HPWH::FirstHourRating firstHourRating;
    if (hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
    {
        const std::string sFirstHourRatingDesig =
            HPWH::FirstHourRating::sDesigMap[firstHourRating.desig];

        std::cout << "\tFirst-Hour Rating:\n";
        std::cout << "\t\tVolume Drawn (L): " << firstHourRating.drawVolume_L << "\n";
        std::cout << "\t\tDesignation: " << sFirstHourRatingDesig << "\n";

        if (hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
        {

            std::cout << "\t24-Hour Test Results:\n";
            if (!standardTestSummary.qualifies)
            {
                std::cout << "\t\tDoes not qualify as consumer water heater.\n";
            }

            std::cout << "\t\tRecovery Efficiency: " << standardTestSummary.recoveryEfficiency
                      << "\n";

            std::cout << "\t\tStandby Loss Coefficient (kJ/h degC): "
                      << standardTestSummary.standbyLossCoefficient_kJperhC << "\n";

            std::cout << "\t\tUEF: " << standardTestSummary.UEF << "\n";

            std::cout << "\t\tAverage Inlet Temperature (degC): " << standardTestSummary.avgInletT_C
                      << "\n";

            std::cout << "\t\tAverage Outlet Temperature (degC): "
                      << standardTestSummary.avgOutletT_C << "\n";

            std::cout << "\t\tTotal Volume Drawn (L): " << standardTestSummary.removedVolume_L
                      << "\n";

            std::cout << "\t\tDaily Water-Heating Energy Consumption (kWh): "
                      << KJ_TO_KWH(standardTestSummary.waterHeatingEnergy_kJ) << "\n";

            std::cout << "\t\tAdjusted Daily Water-Heating Energy Consumption (kWh): "
                      << KJ_TO_KWH(standardTestSummary.adjustedConsumedWaterHeatingEnergy_kJ)
                      << "\n";

            std::cout << "\t\tModified Daily Water-Heating Energy Consumption (kWh): "
                      << KJ_TO_KWH(standardTestSummary.modifiedConsumedWaterHeatingEnergy_kJ)
                      << "\n";

            std::cout << "\tAnnual Values:\n";
            std::cout << "\t\tAnnual Electrical Energy Consumption (kWh): "
                      << KJ_TO_KWH(standardTestSummary.annualConsumedElectricalEnergy_kJ) << "\n";

            std::cout << "\t\tAnnual Energy Consumption (kWh): "
                      << KJ_TO_KWH(standardTestSummary.annualConsumedEnergy_kJ) << "\n";
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

    if (standardTestOptions.saveOutput)
    {
        standardTestOptions.outputFile.close();
    }

    return 0;
}
