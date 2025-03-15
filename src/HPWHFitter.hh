#ifndef HPWHFITTER_hh
#define HPWHFITTER_hh

#include <cfloat>

#include "HPWH.hh"

/**	Optimizer for varying model parameters to match metrics, used by
 *  make_generic to implement a target UEF. The structure is fairly general, but
 *  currently limited to one metric (UEF) and two parameters (COP coeffs).
 *  This could be expanded to include other metrics, such as total energy in 24-h test.
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

    struct MetricInput : public Sender
    { // base class for metric input
        double targetVal;

        MetricInput(double targetVal_in, std::shared_ptr<Courier::Courier> courier)
            : Sender("MetricInput", "metricInput", courier), targetVal(targetVal_in)
        {
        }

        enum class MetricType
        {
            none,
            UEF
        };
        virtual MetricType metricType() { return MetricType::none; }
    };

    struct UEF_MetricInput : public MetricInput
    {
        double ambientT_C = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435
        UEF_MetricInput(double targetUEF,
                        double ambientT_C_in,
                        std::shared_ptr<Courier::Courier> courier)
            : MetricInput(targetUEF, courier), ambientT_C(ambientT_C_in)
        {
        }
        MetricType metricType() override { return MetricType::UEF; }
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
        virtual void show() {}
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

        void show() override { send_info(fmt::format("COP[{}]: {}", tempIndex, getValue())); }
    };

    struct Metric
    { // base class for a metric to meet
        double currVal;
        double targetVal;
        double tolVal; // tolerance

        Metric() : currVal(0.), targetVal(0.), tolVal(1.e-6) {}

        virtual void eval() = 0;
        virtual void evalDiff(double& diff) = 0;

        virtual MetricInput::MetricType metricType() { return MetricInput::MetricType::none; }
    };

    struct UEF_Metric : public Metric
    { // UEF as a metric
        HPWH* hpwh;
        FirstHourRating* firstHourRating;
        StandardTestOptions* standardTestOptions;
        double ambientT_C = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435

        UEF_Metric(UEF_MetricInput& uefMetricInput,
                   FirstHourRating* firstHourRating_in,
                   StandardTestOptions* standardTestOptions_in,
                   HPWH* hpwh_in)
            : Metric()
            , hpwh(hpwh_in)
            , firstHourRating(firstHourRating_in)
            , standardTestOptions(standardTestOptions_in)

        {
            targetVal = uefMetricInput.targetVal;
            ambientT_C = uefMetricInput.ambientT_C;
        }
        virtual ~UEF_Metric() = default;

        MetricInput::MetricType metricType() override { return MetricInput::MetricType::UEF; }

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

    std::vector<std::shared_ptr<Fitter::Metric>> pMetrics; // could be a vector for add'l FOMs
    std::vector<std::shared_ptr<Fitter::Param>> pParams;

    Fitter(std::vector<std::shared_ptr<Fitter::Metric>> pMetrics_in,
           std::vector<std::shared_ptr<Fitter::Param>> pParams_in,
           std::shared_ptr<Courier::Courier> courier)
        : Sender("Fitter", "fitter", courier), pMetrics(pMetrics_in), pParams(pParams_in)
    {
    }

    void showParams()
    {
        for (auto param : pParams)
            param->show();
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

            if (fabs(det) < thresh)
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

struct HPWH::FitOptions
{
    std::vector<HPWH::Fitter::MetricInput*> metricInputs;
    std::vector<HPWH::Fitter::ParamInput*> paramInputs;
};
#endif
