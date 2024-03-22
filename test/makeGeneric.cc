/*
 * Utility to make HPWH generic models
 */

#include <cmath>
#include "HPWH.hh"

int main(int argc, char* argv[])
{
    bool validNumArgs = false;

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

    if (!hpwh.makeGeneric(targetUEF))
    {
        std::cout << "Could not generate generic model.\n";
        exit(1);
    }
}
