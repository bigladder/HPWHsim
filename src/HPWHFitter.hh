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
    struct Param : public Sender
    {
        double dVal;
        Param(std::shared_ptr<Courier::Courier> courier)
            : Sender("ParamInput", "paramInput", courier)
        {
        }
        Param() : dVal(1.e3) {}
        virtual ~Param() = default;

        enum class ParamType
        {
            none,
            PerfCoef
        };
        virtual ParamType paramType() { return ParamType::none; }

        virtual void setValue(double x) = 0;
        virtual double getValue() = 0;

        virtual void show() {}
    };

    struct PerfCoef : public Param
    {
        HPWH* hpwh;
        unsigned tempIndex;
        unsigned power;

        PerfCoef(unsigned tempIndex_in,
                 unsigned power_in,
                 std::shared_ptr<Courier::Courier> courier,
                 HPWH* hpwh_in = nullptr)
            : Param(courier), hpwh(hpwh_in), tempIndex(tempIndex_in), power(power_in)
        {
        }
        PerfCoef(PerfCoef& perfCoef, HPWH* hpwh_in = nullptr)
            : PerfCoef(perfCoef.tempIndex, perfCoef.power, perfCoef.courier, hpwh_in)
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

        virtual std::vector<double>& getCoeffs(HPWH::HeatSource::PerfPoint& perfPoint) = 0;
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
            auto& perfCoeffs = getCoeffs(perfPoint);
            if (power >= perfCoeffs.size())
            {
                send_error("Invalid heat-source performance-map coefficient power.");
            }
            perfCoeffs[power] = x;
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
            auto& perfCoeffs = getCoeffs(perfPoint);
            if (power >= perfCoeffs.size())
            {
                send_error("Invalid heat-source performance-map coefficient power.");
            }
            return perfCoeffs[power];
        }
    };

    struct COP_Coef : public PerfCoef
    { // COP coefficient parameter
        COP_Coef(unsigned tempIndex_in,
                 unsigned power_in,
                 std::shared_ptr<Courier::Courier> courier,
                 HPWH* hpwh_in)
            : PerfCoef(tempIndex_in, power_in, courier, hpwh_in)
        {
            dVal = 1.e-9;
        }

        COP_Coef(PerfCoef& perfCoef, HPWH* hpwh_in) : PerfCoef(perfCoef, hpwh_in) { dVal = 1.e-9; }

        Param::ParamType paramType() override { return Param::ParamType::PerfCoef; }
        PerfCoef::PerfCoefType perfCoefType() override { return PerfCoef::PerfCoefType::COP_Coef; }
        void show() override { send_info(fmt::format("COP[{}]: {}", tempIndex, getValue())); }

        std::vector<double>& getCoeffs(HPWH::HeatSource::PerfPoint& perfPoint) override
        {
            return perfPoint.COP_coeffs;
        }
    };

    struct Metric : public Sender
    { // base class for metric input
        double targetVal;
        double currVal;
        double tolVal; // tolerance

        Metric(double targetVal_in, std::shared_ptr<Courier::Courier> courier)
            : Sender("MetricInput", "metricInput", courier)
            , targetVal(targetVal_in)
            , currVal(0.)
            , tolVal(1.e-6)
        {
        }

        enum class MetricType
        {
            none,
            EF
        };
        virtual MetricType metricType() { return MetricType::none; }

        virtual void eval() = 0;
        virtual void evalDiff(double& diff) = 0;
    };

    struct EF_Metric : public Metric
    {
        HPWH* hpwh;
        TestOptions* testOptions;
        double ambientT_C = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435

        EF_Metric(double targetEF,
                  TestOptions* testOptions_in,
                  std::shared_ptr<Courier::Courier> courier,
                  HPWH* hpwh_in = nullptr)
            : Metric(targetEF, courier), hpwh(hpwh_in), testOptions(testOptions_in)
        {
        }

        EF_Metric(Metric& metric, TestOptions* testOptions_in, HPWH* hpwh_in = nullptr)
            : Metric(metric), hpwh(hpwh_in), testOptions(testOptions_in)
        {
        }

        virtual ~EF_Metric() = default;

        MetricType metricType() override { return MetricType::EF; }

        void eval() override
        { // get current EF
            static HPWH::TestSummary testSummary;
            hpwh->run24hrTest(*testOptions, testSummary);
            currVal = testSummary.EF;
        }

        void evalDiff(double& diff) override
        { // get difference ratio
            eval();
            diff = (currVal - targetVal) / tolVal;
        }
    };

    std::vector<std::shared_ptr<Fitter::Metric>> pMetrics;
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

    void leastSquares();
    void fit();
};

struct HPWH::FitOptions
{
    std::vector<HPWH::Fitter::Metric*> metrics;
    std::vector<HPWH::Fitter::Param*> params;
};
#endif
