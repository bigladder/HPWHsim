/*
 * Utility to make HPWH generic models
 */

#include "HPWH.hh"

double getError(const double targetUEF, const double UEF)
{
    const double tolUEF = 1.e-4;
    return (targetUEF - UEF) / tolUEF;
}

struct ParamInfo
{
    enum class Type
    {
        none,
        copCoef
    };

    virtual Type paramType() = 0;

    virtual bool assign(HPWH& hpwh, double*& val) = 0;
};

struct CopCoefInfo : public ParamInfo
{
    int heatSourceIndex;
    int tempIndex;
    int power;

    Type paramType() override { return Type::copCoef; }

    CopCoefInfo(const int heatSourceIndex_in, const int tempIndex_in, const int power_in)
        : ParamInfo(), heatSourceIndex(heatSourceIndex_in), tempIndex(tempIndex_in), power(power_in)
    {
    }

    bool assign(HPWH& hpwh, double*& val) override
    {
        val = nullptr;
        HPWH::HeatSource* heatSource;
        if (!hpwh.getNthHeatSource(heatSourceIndex, heatSource))
        {
            std::cout << "Invalid heat source index.\n";
            return false;
        }

        auto& perfMap = heatSource->perfMap;
        if (tempIndex >= perfMap.size())
        {
            std::cout << "Invalid heat-source performance-map temperature index.\n";
            return false;
        }

        auto& perfPoint = perfMap[tempIndex];
        auto& copCoeffs = perfPoint.COP_coeffs;
        if (power >= copCoeffs.size())
        {
            std::cout << "Invalid heat-source performance-map cop-coefficient power.\n";
            return false;
        }
        std::cout << "Valid parameter.\n";
        val = &copCoeffs[power];
        return true;
    };
};

struct Param
{
    double* val;

    Param() : val(nullptr) {}

    virtual bool assignVal(HPWH& hpwh) = 0;
};

struct CopCoef : public Param,
                 CopCoefInfo
{
    CopCoef(CopCoefInfo& copCoefInfo) : Param(), CopCoefInfo(copCoefInfo) {}

    bool assignVal(HPWH& hpwh) override { return assign(hpwh, val); }
};

int main(int argc, char* argv[])
{
    enum class Target
    {
        UEF
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

    std::vector<Param*> params;

    CopCoefInfo copT2constInfo = {2, 1, 0}; // heatSourceIndex, tempIndex, power
    CopCoefInfo copT2linInfo = {2, 1, 1};   // heatSourceIndex, tempIndex, power

    CopCoef copT2const(copT2constInfo);
    CopCoef copT2lin(copT2linInfo);

    params.push_back(&copT2const);
    params.push_back(&copT2lin);

    //
    bool res = true;
    for (auto& param : params)
    {
        res &= param->assignVal(hpwh);
    }

    const int maxIters = 1000;
    for (auto iter = 0; iter < maxIters; ++iter)
    {
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

        double UEF = standardTestSummary.UEF;
        double err = getError(targetUEF, UEF);
        std::cout << "\t UEF: " << UEF << ", error: " << err << "\n";
    }
    return 0;
}
