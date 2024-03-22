/*
 * Utility to make HPWH generic models
 */

#include <cmath>
#include "HPWH.hh"

static bool makeGeneric(HPWH& hpwh, const double targetUEF)
{

    static HPWH::FirstHourRating firstHourRating;
    static HPWH::StandardTestOptions standardTestOptions;

    struct Inverter
    {
        static bool getLeftDampedInv(const double nu,
                                     const std::vector<double>& matV, // 1 x 2
                                     std::vector<double>& invMatV     // 2 x 1
        )
        {
            constexpr double thresh = 1.e-12;

            if (matV.size() != 2)
            {
                return false;
            }

            double a = matV[0];
            double b = matV[1];

            double A = (1. + nu) * a * a;
            double B = (1. + nu) * b * b;
            double C = a * b;
            double det = A * B - C * C;

            if (abs(det) < thresh)
            {
                return false;
            }

            invMatV.resize(2);
            invMatV[0] = (a * B - b * C) / det;
            invMatV[1] = (-a * C + b * A) / det;
            return true;
        }
    };

    struct ParamInfo
    {
        enum class Type
        {
            none,
            copCoef
        };

        virtual Type paramType() = 0;

        virtual bool assign(HPWH& hpwh, double*& val) = 0;
        virtual void showInfo(std::ostream& os) = 0;
    };

    struct CopCoefInfo : public ParamInfo
    {
        unsigned heatSourceIndex;
        unsigned tempIndex;
        unsigned power;

        Type paramType() override { return Type::copCoef; }

        CopCoefInfo(const unsigned heatSourceIndex_in,
                    const unsigned tempIndex_in,
                    const unsigned power_in)
            : ParamInfo()
            , heatSourceIndex(heatSourceIndex_in)
            , tempIndex(tempIndex_in)
            , power(power_in)
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

        void showInfo(std::ostream& os) override
        {
            os << " heat-source index: " << heatSourceIndex;
            os << ", temperature index: " << tempIndex;
            os << ", power: " << power;
        }
    };

    struct Param
    {
        double* val;
        double dVal;

        Param() : val(nullptr), dVal(1.e3) {}

        virtual bool assignVal(HPWH& hpwh) = 0;
        virtual void show(std::ostream& os) = 0;
    };

    struct CopCoef : public Param,
                     CopCoefInfo
    {
        CopCoef(CopCoefInfo& copCoefInfo) : Param(), CopCoefInfo(copCoefInfo) { dVal = 1.e-9; }

        bool assignVal(HPWH& hpwh) override { return assign(hpwh, val); }

        void show(std::ostream& os) override
        {
            showInfo(os);
            os << ": " << *val << "\n";
        }
    };

    struct Merit
    {
        double val;
        double targetVal;
        double tolVal;

        Merit() : val(0.), targetVal(0.), tolVal(1.e-6) {}

        virtual bool eval(HPWH& hpwh) = 0;
        virtual bool evalDiff(HPWH& hpwh, double& diff) = 0;
    };

    struct UEF_Merit : public Merit
    {
        UEF_Merit(double targetVal_in) : Merit() { targetVal = targetVal_in; }

        bool eval(HPWH& hpwh) override
        {
            static HPWH::StandardTestSummary standardTestSummary;
            if (!hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
            {
                std::cout << "Unable to complete 24-hr test.\n";
                return false;
            }
            val = standardTestSummary.UEF;
            return true;
        }

        bool evalDiff(HPWH& hpwh, double& diff) override
        {
            if (eval(hpwh))
            {
                diff = (val - targetVal) / tolVal;
                return true;
            };
            return false;
        }
    };

    standardTestOptions.saveOutput = false;
    standardTestOptions.changeSetpoint = true;
    standardTestOptions.nTestTCouples = 6;
    standardTestOptions.setpointT_C = 51.7;
    if (!hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
    {
        std::cout << "Unable to complete first-hour rating test.\n";
        exit(1);
    }

    const std::string sFirstHourRatingDesig =
        HPWH::FirstHourRating::sDesigMap[firstHourRating.desig];

    std::cout << "\tFirst-Hour Rating:\n";
    std::cout << "\t\tVolume Drawn (L): " << firstHourRating.drawVolume_L << "\n";

    // set up merit parameter
    Merit* pMerit;
    UEF_Merit uefMerit(targetUEF);
    pMerit = &uefMerit;

    // set up parameters
    std::vector<Param*> pParams;

    CopCoefInfo copT2constInfo = {2, 1, 0}; // heatSourceIndex, tempIndex, power
    CopCoefInfo copT2linInfo = {2, 1, 1};   // heatSourceIndex, tempIndex, power

    CopCoef copT2const(copT2constInfo);
    CopCoef copT2lin(copT2linInfo);

    pParams.push_back(&copT2const);
    pParams.push_back(&copT2lin);

    std::vector<double> dParams;

    //
    bool foundParams = true;
    for (auto& pParam : pParams)
    {
        foundParams &= pParam->assignVal(hpwh);
        dParams.push_back(pParam->dVal);
    }
    if (!foundParams)
    {
        return false;
    }
    auto nParams = pParams.size();

    double nu = 0.1;
    const int maxIters = 20;
    for (auto iter = 0; iter < maxIters; ++iter)
    {
        if (!pMerit->eval(hpwh))
        {
            return false;
        }
        std::cout << iter << ": ";
        std::cout << "UEF: " << pMerit->val << "; ";

        bool first = true;
        for (std::size_t j = 0; j < nParams; ++j)
        {
            if (!first)
                std::cout << ", ";
            std::cout << *(pParams[j]->val);
            first = false;
        }

        double dMerit0 = 0.;
        if (!(pMerit->evalDiff(hpwh, dMerit0)))
        {
            return false;
        }
        double FOM0 = dMerit0 * dMerit0;
        double FOM1 = 0., FOM2 = 0.;
        std::cout << ", FOM: " << FOM0 << "\n";

        std::vector<double> paramV(nParams);
        for (std::size_t i = 0; i < nParams; ++i)
        {
            paramV[i] = *(pParams[i]->val);
        }

        std::vector<double> jacobiV(nParams); // 1 x 2
        for (std::size_t j = 0; j < nParams; ++j)
        {
            *(pParams[j]->val) = paramV[j] + (pParams[j]->dVal);
            // for (std::size_t i = 0; i < 1; ++i)
            {
                double dMerit;
                if (!(pMerit->evalDiff(hpwh, dMerit)))
                {
                    return false;
                }
                jacobiV[j] = (dMerit - dMerit0) / (pParams[j]->dVal);
            }
            *(pParams[j]->val) = paramV[j];
        }

        // try nu
        std::vector<double> invJacobiV1;
        bool got1 = Inverter::getLeftDampedInv(nu, jacobiV, invJacobiV1);

        std::vector<double> inc1ParamV(2);
        std::vector<double> paramV1 = paramV;
        if (got1)
        {
            for (std::size_t j = 0; j < nParams; ++j)
            {
                inc1ParamV[j] = -invJacobiV1[j] * dMerit0;
                paramV1[j] += inc1ParamV[j];
                *(pParams[j]->val) = paramV1[j];
            }
            double dMerit;
            if (!pMerit->evalDiff(hpwh, dMerit))
            {
                return false;
            }
            FOM1 = dMerit * dMerit;

            // restore
            for (std::size_t j = 0; j < nParams; ++j)
            {
                *(pParams[j]->val) = paramV[j];
            }
        }

        // try nu / 2
        std::vector<double> invJacobiV2;
        bool got2 = Inverter::getLeftDampedInv(nu / 2., jacobiV, invJacobiV2);

        std::vector<double> inc2ParamV(2);
        std::vector<double> paramV2 = paramV;
        if (got2)
        {
            for (std::size_t j = 0; j < nParams; ++j)
            {
                inc2ParamV[j] = -invJacobiV2[j] * dMerit0;
                paramV2[j] += inc2ParamV[j];
                *(pParams[j]->val) = paramV2[j];
            }
            double dMerit;
            if (!pMerit->evalDiff(hpwh, dMerit))
            {
                return false;
            }
            FOM2 = dMerit * dMerit;

            // restore
            for (std::size_t j = 0; j < nParams; ++j)
            {
                *(pParams[j]->val) = paramV[j];
            }
        }

        // check for improvement
        if (got1 && got2)
        {
            if ((FOM1 < FOM0) || (FOM2 < FOM0))
            { // at least one improved
                if (FOM1 < FOM2)
                { // pick 1
                    for (std::size_t i = 0; i < nParams; ++i)
                    {
                        *(pParams[i]->val) = paramV1[i];
                        (pParams[i]->dVal) = inc1ParamV[i] / 1.e3;
                        FOM0 = FOM1;
                    }
                }
                else
                { // pick 2
                    for (std::size_t i = 0; i < nParams; ++i)
                    {
                        *(pParams[i]->val) = paramV2[i];
                        (pParams[i]->dVal) = inc2ParamV[i] / 1.e3;
                        FOM0 = FOM2;
                    }
                }
            }
            else
            { // no improvement
                nu *= 10.;
                if (nu > 1.e6)
                {
                    return false;
                }
            }
        }
    }
    return false;
}

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

    return makeGeneric(hpwh, targetUEF) ? 0 : 1;
}
