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
    double targetUEF = 1.;
    if (argc == 4)
    {
        sModelName = argv[1];
        targetUEF = std::stod(argv[2]);
        sOutputDirectory = argv[3];
        validNumArgs = true;
    }

    if (!validNumArgs)
    {
        std::cout << "Invalid input:\n\
            To make a generic model, provide THREE arguments:\n\
            \t[model spec Name, i.e., Sanden80]\n\
            \t[target UEF, i.e., 3.1]\n\
            \t[output Directory, i.e., .\\output]\n\
            ";
        exit(1);
    }

    HPWH hpwh;
    std::string sInputFile = sModelName + ".txt";

    if (hpwh.initFromFile(sInputFile) != 0)
    {
        std::cout << "Invalid input: Could not load model.\n";
        exit(1);
    }

    std::cout << "Model name: " << sModelName << "\n";
    std::cout << "Target UEF: " << targetUEF << "\n";
    std::cout << "Output directory: " << sOutputDirectory << "\n";

    HPWH::FirstHourRating firstHourRating;
    if (!hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
    {
        std::cout << "Unable to complete first-hour rating test.\n";
        exit(1);
    }

    const std::string sFirstHourRatingDesig =
        HPWH::FirstHourRating::sDesigMap[firstHourRating.desig];

    std::cout << "\tFirst-Hour Rating:\n";
    std::cout << "\t\tVolume Drawn (L): " << firstHourRating.drawVolume_L << "\n";

    double initialUEF = 0.;
    if (!hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
    {
        std::cout << "Unable to complete 24-hr test.\n";
        exit(1);
    }

    if (!standardTestSummary.qualifies)
    {
        std::cout << "\t\tDoes not qualify as consumer water heater.\n";
        exit(1);
    }

    initialUEF = standardTestSummary.UEF;
    std::cout << "\tInitial UEF: " << initialUEF << "\n";

    return 0;
}
