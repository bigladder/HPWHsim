#ifndef HPWHFITTER_hh
#define HPWHFITTER_hh

#include <cfloat>

#include "HPWH.hh"

///	@struct HPWH::Fitter HPWHFitter.h
/// Optimizer for varying model parameters to match metrics, used by
/// HPWH::makeGeneric to modify performance coeffs to match a target UEF.
/// The structure is fairly general, but currently limited to one metric (UEF)
/// and two parameters (COP coeffs). This could be expanded to include other metrics,
/// such as total energy in 24-hr test.
struct HPWH::Fitter : public Sender
{
    ///	base class for variational parameters
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

    /// performance-coefficient parameter
    struct PerfCoef : public Param
    {
        HPWH* hpwh;
        unsigned tempIndex;
        unsigned exponent;

        PerfCoef(unsigned tempIndex_in,
                 unsigned exponent_in,
                 std::shared_ptr<Courier::Courier> courier,
                 HPWH* hpwh_in = nullptr)
            : Param(courier), hpwh(hpwh_in), tempIndex(tempIndex_in), exponent(exponent_in)
        {
        }
        PerfCoef(PerfCoef& perfCoef, HPWH* hpwh_in = nullptr)
            : PerfCoef(perfCoef.tempIndex, perfCoef.exponent, perfCoef.courier, hpwh_in)
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

        /// check validity and return reference to HPWH member variable
        double* getPerfCoeff()
        {
            HPWH::HeatSource* heatSource;
            hpwh->getNthHeatSource(hpwh->compressorIndex, heatSource);

            auto& perfMap = heatSource->perfMap;
            if (tempIndex >= perfMap.size())
            {
                send_error("Invalid heat-source performance-map temperature index.");
            }

            auto& perfPoint = perfMap[tempIndex];
            auto& perfCoeffs = getCoeffs(perfPoint);
            if (exponent >= perfCoeffs.size())
            {
                send_error("Invalid heat-source performance-map coefficient exponent.");
            }
            return &perfCoeffs[exponent];
        }

        void setValue(double x) override { *getPerfCoeff() = x; }

        double getValue() override { return *getPerfCoeff(); }

        std::shared_ptr<PerfCoef> make(HPWH* hpwh_in)
        {
            switch (perfCoefType())
            {
            case PerfCoefType::PinCoef:
            {
                return std::make_shared<Fitter::PinCoef>(*this, hpwh_in);
            }
            case PerfCoefType::COP_Coef:
            {
                return std::make_shared<Fitter::COP_Coef>(*this, hpwh_in);
            }
            case Fitter::PerfCoef::PerfCoefType::none:
                break;
            }
            return nullptr;
        }
    };

    /// input-power coefficient parameter
    struct PinCoef : public PerfCoef
    {
        PinCoef(unsigned tempIndex_in,
                unsigned exponent_in,
                std::shared_ptr<Courier::Courier> courier,
                HPWH* hpwh_in)
            : PerfCoef(tempIndex_in, exponent_in, courier, hpwh_in)
        {
            dVal = 1.e-9;
        }

        PinCoef(PerfCoef& perfCoef, HPWH* hpwh_in) : PerfCoef(perfCoef, hpwh_in) { dVal = 1.e-5; }

        PerfCoef::PerfCoefType perfCoefType() override { return PerfCoef::PerfCoefType::PinCoef; }

        void show() override { send_info(fmt::format("Pin[{}]: {}", tempIndex, getValue())); }

        std::vector<double>& getCoeffs(HPWH::HeatSource::PerfPoint& perfPoint) override
        {
            return perfPoint.inputPower_coeffs;
        }
    };

    ///	coefficient-of-performance coefficient parameter
    struct COP_Coef : public PerfCoef
    {
        COP_Coef(unsigned tempIndex_in,
                 unsigned exponent_in,
                 std::shared_ptr<Courier::Courier> courier,
                 HPWH* hpwh_in)
            : PerfCoef(tempIndex_in, exponent_in, courier, hpwh_in)
        {
            dVal = 1.e-9;
        }

        COP_Coef(PerfCoef& perfCoef, HPWH* hpwh_in) : PerfCoef(perfCoef, hpwh_in) { dVal = 1.e-9; }

        PerfCoef::PerfCoefType perfCoefType() override { return PerfCoef::PerfCoefType::COP_Coef; }

        void show() override { send_info(fmt::format("COP[{}]: {}", tempIndex, getValue())); }

        std::vector<double>& getCoeffs(HPWH::HeatSource::PerfPoint& perfPoint) override
        {
            return perfPoint.COP_coeffs;
        }
    };

    ///	base class for metric data,
    /// i.e., a target value to match by varying parameters
    struct Metric : public Sender
    {
        double targetVal; // value to be matched
        double currVal;   // current value
        double tolVal;    // tolerance

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

    /// energy-factor metric, i.e., E50, UEF, E95
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

    /// metric and parameter data retained as shared pts
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

/// container for passing fit information
/// to generate shared ptrs (see HPWH::makeGeneric)
struct HPWH::FitOptions
{
    std::vector<HPWH::Fitter::Metric*> metrics;
    std::vector<HPWH::Fitter::Param*> params;
};

#endif
