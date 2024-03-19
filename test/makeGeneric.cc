/*
 * Utility to make HPWH generic models
 */

#include "HPWH.hh"

int main(int argc, char* argv[])
{
    enum class Target
    {
        UEF
    };

    enum class Param
    {
        copT2const,
        copT2lin
    };

    HPWH::StandardTestSummary standardTestSummary;
    bool validNumArgs = false;

    HPWH::StandardTestOptions standardTestOptions;
    standardTestOptions.saveOutput = false;
    standardTestOptions.changeSetpoint = true;
    standardTestOptions.nTestTCouples = 6;
    standardTestOptions.setpointT_C = 51.7;

    // process command line arguments
    std::string sModelName;
    std::string sOutputDirectory;
    if (argc == 2)
    {
        sModelName = argv[1];
        validNumArgs = true;
    }

    if (!validNumArgs)
    {
        std::cout << "Invalid input:\n\
            To make a generic model, provide TWO arguments:\n\
            \t[model spec Name, i.e., Sanden80]\n\
            \t[output Directory, i.e., .\\output]\n\
            \t[target UEF, i.e., 3.1]\n";
        exit(1);

        HPWH hpwh;
        bool validModel = false;
        if (hpwh.initPreset(sModelName) == 0)
        {
            validModel = true;
        }

        if (!validModel)
        {
            std::cout << "Invalid input: Model name not found.\n";
            exit(1);
        }

        std::cout << "Model name: " << sModelName << "\n";

        HPWH::FirstHourRating firstHourRating;
        if (hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
        {
            std::cout << "\tFirst-Hour Rating:\n";
            std::cout << "\t\tVolume Drawn (L): " << firstHourRating.drawVolume_L << "\n";

            if (hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
            {

                std::cout << "\t24-Hour Test Results:\n";
                if (!standardTestSummary.qualifies)
                {
                    std::cout << "\t\tDoes not qualify as consumer water heater.\n";
                }
                std::cout << "\t\tUEF: " << standardTestSummary.UEF << "\n";
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
