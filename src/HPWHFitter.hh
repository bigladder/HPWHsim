#ifndef HPWHFITTER_hh
#define HPWHFITTER_hh

#include "HPWH.hh"

///	@struct HPWH::Fitter HPWHFitter.h
/// Optimizer for varying model parameters to match metrics, used by
/// HPWH::makeGenericEF to modify performance coeffs to match a target UEF.
/// The structure is fairly general, but currently limited to one metric (UEF)
/// and two parameters (COP coeffs). This could be expanded to include other metrics,
/// such as total energy in 24-hr test.
struct HPWH::Fitter : public Sender
{
    ///	base class for variational parameters
    struct Parameter : public Sender
    {
        double increment; // differential increment (least squares)

        Parameter(std::shared_ptr<Courier::Courier> courier)
            : Sender("Parameter", "parameter", courier)
        {
        }
        Parameter() : increment(1.e3) {}
        virtual ~Parameter() = default;

        virtual void setValue(double x) = 0;
        virtual double getValue() = 0;

        virtual std::string show() = 0;
    };

    /// performance coefficient
    struct PerfCoef : public Parameter
    {
      private:
        HPWH* hpwh;
        unsigned tempIndex;
        unsigned exponent;

      public:
        PerfCoef(unsigned tempIndex_in,
                 unsigned exponent_in,
                 std::shared_ptr<Courier::Courier> courier,
                 HPWH* hpwh_in = nullptr)
            : Parameter(courier), hpwh(hpwh_in), tempIndex(tempIndex_in), exponent(exponent_in)
        {
        }
        PerfCoef(PerfCoef& perfCoef, HPWH* hpwh_in = nullptr)
            : PerfCoef(perfCoef.tempIndex, perfCoef.exponent, perfCoef.courier, hpwh_in)
        {
        }

        virtual std::vector<double>& getCoeffs(HPWH::HeatSource::PerfPoint& perfPoint) = 0;

        /// check validity and return reference to HPWH member variable
      private:
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

      public:
        void setValue(double x) override { *getPerfCoeff() = x; }

        double getValue() override { return *getPerfCoeff(); }

        virtual std::string getFormat() const = 0;

        std::string show() override { return fmt::format(getFormat(), tempIndex, getValue()); }
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
            increment = 1.e-5;
        }

        std::vector<double>& getCoeffs(HPWH::HeatSource::PerfPoint& perfPoint) override
        {
            return perfPoint.inputPower_coeffs;
        }

        std::string getFormat() const override { return "Pin[{}]: {}"; }
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
            increment = 1.e-9;
        }

        std::string getFormat() const override { return "COP[{}]: {}"; }

        std::vector<double>& getCoeffs(HPWH::HeatSource::PerfPoint& perfPoint) override
        {
            return perfPoint.COP_coeffs;
        }
    };

    ///	base class for metric data,
    /// i.e., a target value to match by varying parameters
    struct Metric : public Sender
    {
        double targetValue;  // value to be matched
        double currentValue; // current value
        double tolerance;    // tolerance

        Metric(double targetValue_in, std::shared_ptr<Courier::Courier> courier)
            : Sender("MetricInput", "metricInput", courier)
            , targetValue(targetValue_in)
            , currentValue(0.)
            , tolerance(1.e-6)
        {
        }

        enum class MetricType
        {
            none,
            EF
        };
        virtual MetricType metricType() { return MetricType::none; }

        virtual void evaluate() = 0;
        virtual double findError() = 0;
    };

    /// energy-factor metric, i.e., E50, UEF, E95
    struct EF_Metric : public Metric
    {
        HPWH* hpwh;
        TestOptions* testOptions;
        TestSummary testSummary;

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

        /// get current EF
        void evaluate() override
        {
            hpwh->run24hrTest(*testOptions, testSummary);
            currentValue = testSummary.EF;
        }

        /// find error ratio
        double findError() override
        {
            evaluate();
            return (currentValue - targetValue) / tolerance;
        }

        const TestSummary getTestSummary() { return testSummary; }
    };

    /// metric and parameter data retained as shared pts
    std::vector<std::shared_ptr<Fitter::Metric>> metrics;
    std::vector<std::shared_ptr<Fitter::Parameter>> parameters;

    Fitter(std::vector<std::shared_ptr<Fitter::Metric>> metrics_in,
           std::vector<std::shared_ptr<Fitter::Parameter>> params_in,
           std::shared_ptr<Courier::Courier> courier)
        : Sender("Fitter", "fitter", courier), metrics(metrics_in), parameters(params_in)
    {
    }

    std::string showParameters() const
    {
        std::string s = "";
        bool first = true;
        for (auto parameter : parameters)
        {
            if (!first)
                s.append("\n");
            s.append(parameter->show());
            first = false;
        }
        return s;
    }

    void performLeastSquaresMiminization();
    void fit();
};

#endif
