#ifndef HPWHFITTER_hh
#define HPWHFITTER_hh

#include <cfloat>

#include "HPWH.hh"

/**	Optimizer for varying model parameters to match metrics, used by
 *  make_generic to implement a target UEF. The structure is fairly general, but
 *  currently limited to one figure-of-merit (UEF) and two parameters (COP coeffs).
 *  This could be expanded to include other FOMs, such as total energy in 24-h test.
 */

struct HPWH::Fitter : public Sender
{
    struct ParamInput : public Sender
    {
        ParamInput(std::shared_ptr<Courier::Courier> courier)
            : Sender("ParamInput", "paramInput", courier)
        {
        }
        enum class ParamType
        {
            none,
            PerfCoef
        };
        virtual ParamType paramType() { return ParamType::none; }
    };

    struct PerfCoefInput : public ParamInput
    {
        unsigned tempIndex;
        unsigned power;
        PerfCoefInput(unsigned tempIndex_in,
                      unsigned power_in,
                      std::shared_ptr<Courier::Courier> courier)
            : ParamInput(courier), tempIndex(tempIndex_in), power(power_in)
        {
        }
        enum class PerfCoefType
        {
            none,
            PinCoef,
            COP_Coef
        };
        ParamType paramType() override { return ParamType::PerfCoef; }
        virtual PerfCoefType perfCoefType() { return PerfCoefType::none; }
    };

    struct PinCoefInput : public PerfCoefInput
    {
        PerfCoefType perfCoefType() override { return PerfCoefType::PinCoef; }
        PinCoefInput(unsigned tempIndex, unsigned power, std::shared_ptr<Courier::Courier> courier)
            : PerfCoefInput(tempIndex, power, courier)
        {
        }
    };
    struct COP_CoefInput : public PerfCoefInput
    {
        PerfCoefType perfCoefType() override { return PerfCoefType::COP_Coef; }
        COP_CoefInput(unsigned tempIndex, unsigned power, std::shared_ptr<Courier::Courier> courier)
            : PerfCoefInput(tempIndex, power, courier)
        {
        }
    };

    struct MeritInput : public Sender
    { // base class for a figure of merit
        double targetVal;

        MeritInput(double targetVal_in, std::shared_ptr<Courier::Courier> courier)
            : Sender("MeritInput", "meritInput", courier), targetVal(targetVal_in)
        {
        }

        enum class MeritType
        {
            none,
            UEF
        };
        virtual MeritType meritType() { return MeritType::none; }
    };

    struct UEF_MeritInput : public MeritInput
    {
        double ambientT_C = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435
        UEF_MeritInput(double targetUEF,
                       double ambientT_C_in,
                       std::shared_ptr<Courier::Courier> courier)
            : MeritInput(targetUEF, courier), ambientT_C(ambientT_C_in)
        {
        }
        MeritType meritType() override { return MeritType::UEF; }
    };

    struct ParamInfo
    { // base class for parameter information

        virtual void setValue(double x) = 0;
        virtual double getValue() = 0;
    };

    struct COP_CoefInfo : public ParamInfo,
                          COP_CoefInput
    { // COP coef parameter info
        HPWH* hpwh;

        COP_CoefInfo(COP_CoefInput cop_CoefInput, HPWH* hpwh_in)
            : ParamInfo(), COP_CoefInput(cop_CoefInput), hpwh(hpwh_in)
        {
        }

        void setValue(double x) override
        { // set the HPWH member variable
            HPWH::HeatSource* heatSource;
            hpwh->getNthHeatSource(hpwh->compressorIndex, heatSource);

            auto& perfMap = heatSource->perfMap;
            if (tempIndex >= perfMap.size())
            {
                send_error("Invalid heat-source performance-map temperature index.");
            }

            auto& perfPoint = perfMap[tempIndex];
            auto& copCoeffs = perfPoint.COP_coeffs;
            if (power >= copCoeffs.size())
            {
                send_error("Invalid heat-source performance-map cop-coefficient power.");
            }
            copCoeffs[power] = x;
        }

        double getValue() override
        { // get a reference to the HPWH member variable
            HPWH::HeatSource* heatSource;
            hpwh->getNthHeatSource(hpwh->compressorIndex, heatSource);

            auto& perfMap = heatSource->perfMap;
            if (tempIndex >= perfMap.size())
            {
                send_error("Invalid heat-source performance-map temperature index.");
            }

            auto& perfPoint = perfMap[tempIndex];
            auto& copCoeffs = perfPoint.COP_coeffs;
            if (power >= copCoeffs.size())
            {
                send_error("Invalid heat-source performance-map cop-coefficient power.");
            }
            return copCoeffs[power];
        }
    };

    struct Param
    { // base class for parameter
        double dVal;
        bool hold = false;

        Param() : dVal(1.e3) {}

        virtual void setValue(double x) = 0;
        virtual double getValue() = 0;
        virtual ~Param() = default;

        virtual ParamInput::ParamType paramType() { return ParamInput::ParamType::none; }
    };

    struct COP_Coef : public Param,
                      COP_CoefInfo
    { // COP coefficient parameter
        COP_Coef(COP_CoefInput& copCoefInput, HPWH* hpwh)
            : Param(), COP_CoefInfo(copCoefInput, hpwh)
        {
            dVal = 1.e-9;
        }

        void setValue(double x) override { COP_CoefInfo::setValue(x); }
        double getValue() override { return COP_CoefInfo::getValue(); }
        ParamInput::ParamType paramType() override { return ParamInput::ParamType::PerfCoef; }
    };

    struct Merit
    { // base class for a figure of merit
        double currVal;
        double targetVal;
        double tolVal; // tolerance

        Merit() : currVal(0.), targetVal(0.), tolVal(1.e-6) {}

        virtual void eval() = 0;
        virtual void evalDiff(double& diff) = 0;

        virtual MeritInput::MeritType meritType() { return MeritInput::MeritType::none; }
    };

    struct UEF_Merit : public Merit
    { // UEF as a figure of merit
        HPWH* hpwh;
        FirstHourRating* firstHourRating;
        StandardTestOptions* standardTestOptions;
        double ambientT_C = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435

        UEF_Merit(UEF_MeritInput& uefMeritInput,
                  FirstHourRating* firstHourRating_in,
                  StandardTestOptions* standardTestOptions_in,
                  HPWH* hpwh_in)
            : Merit()
            , hpwh(hpwh_in)
            , firstHourRating(firstHourRating_in)
            , standardTestOptions(standardTestOptions_in)

        {
            targetVal = uefMeritInput.targetVal;
            ambientT_C = uefMeritInput.ambientT_C;
        }
        virtual ~UEF_Merit() = default;

        MeritInput::MeritType meritType() override { return MeritInput::MeritType::UEF; }

        void eval() override
        { // get current UEF
            static HPWH::StandardTestSummary standardTestSummary;
            hpwh->customTestOptions.overrideAmbientT = true;
            hpwh->customTestOptions.ambientT_C = ambientT_C;
            hpwh->run24hrTest(*firstHourRating, standardTestSummary, *standardTestOptions);
            currVal = standardTestSummary.UEF;
        }

        void evalDiff(double& diff) override
        { // get difference ratio
            eval();
            diff = (currVal - targetVal) / tolVal;
        }
    };

    std::vector<std::shared_ptr<Fitter::Merit>> pMerits; // could be a vector for add'l FOMs
    std::vector<std::shared_ptr<Fitter::Param>> pParams;

    Fitter(std::vector<std::shared_ptr<Fitter::Merit>> pMerits_in,
           std::vector<std::shared_ptr<Fitter::Param>> pParams_in,
           std::shared_ptr<Courier::Courier> courier)
        : Sender("Fitter", "fitter", courier), pMerits(pMerits_in), pParams(pParams_in)
    {
    }

    struct Inverter
    { // invert a 1 x 2 matrix
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

    int secant( // find x given f(x) (secant method)
        double (*pFunc)(void* pO, double& x),
        // function under investigation; note that it
        //   may CHANGE x re domain limits etc.
        void* pO,               // pointer passed to *pFunc, typ object pointer
        double f,               // f( x) value sought
        double eps,             // convergence tolerance, hi- or both sides
                                //   see also epsLo
        double& x1,             // x 1st guess,
                                //   returned with result
        double& f1,             // f( x1), if known, else pass DBL_MIN
                                //   returned: f( x1), may be != f, if no converge
        double x2,              // x 2nd guess
        double f2 /*=DBL_MIN*/, // f( x2), if known
        double epsLo /*=-1.*/); // lo-side convergence tolerance

    void leastSquares();
    void fit();
};

struct HPWH::GenericOptions
{
    std::vector<HPWH::Fitter::MeritInput*> meritInputs;
    std::vector<HPWH::Fitter::ParamInput*> paramInputs;
};
#endif
