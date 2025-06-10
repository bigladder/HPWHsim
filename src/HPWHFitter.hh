#ifndef HPWHFITTER_hh
#define HPWHFITTER_hh

#include "HPWH.hh"
#include "Condenser.hh"

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
        double* data_ptr;

        explicit Parameter(std::shared_ptr<Courier::Courier> courier)
            : Sender("Parameter", "parameter", courier), increment(1.e-12), data_ptr(nullptr)
        {
        }
        virtual ~Parameter() = default;

        virtual std::string show() = 0;
    };

    /// performance coefficient
    struct PerformanceCoefficient : public Parameter
    {
      private:
        HPWH::Condenser* condenser;
        unsigned temperatureIndex;
        unsigned exponent;

      public:
        PerformanceCoefficient(unsigned temperatureIndex_in,
                               unsigned exponent_in,
                               std::shared_ptr<Courier::Courier> courier,
                               HPWH::Condenser* condenser_in = nullptr)
            : Parameter(courier)
            , condenser(condenser_in)
            , temperatureIndex(temperatureIndex_in)
            , exponent(exponent_in)
        {
        }

        PerformanceCoefficient(PerformanceCoefficient& performanceCoefficient,
                               HPWH::Condenser* condenser_in = nullptr)
            : PerformanceCoefficient(performanceCoefficient.temperatureIndex,
                                     performanceCoefficient.exponent,
                                     performanceCoefficient.courier,
                                     condenser_in)
        {
        }

        std::string show() override
        {
            return fmt::format(getFormat(), temperatureIndex, *data_ptr);
        }

      protected:
        virtual std::vector<double>&
        getCoefficients(HPWH::Condenser::PerformancePoly& perfPoly) = 0;

        /// check validity and retain pointer to HPWH member variable
        void assign()
        {
            if (condenser->useBtwxtGrid)
            {
                send_error("Invalid performance representation.");
            }
            if (temperatureIndex >= (*(condenser->pPerfPolySet)).size())
            {
                send_error("Invalid heat-source performance-map temperature index.");
            }

            auto& perfPoly = (*(condenser->pPerfPolySet))[temperatureIndex];
            auto& perfCoeffs = getCoefficients(perfPoly);
            if (exponent >= perfCoeffs.size())
            {
                send_error("Invalid heat-source performance-map coefficient exponent.");
            }
            data_ptr = &perfCoeffs[exponent];
        }

      private:
        [[nodiscard]] virtual std::string getFormat() const = 0;
    };

    /// input-power coefficient parameter
    struct InputPowerCoefficient : public PerformanceCoefficient
    {
        InputPowerCoefficient(unsigned temperatureIndex_in,
                              unsigned exponent_in,
                              std::shared_ptr<Courier::Courier> courier,
                              HPWH::Condenser* condenser_in)
            : PerformanceCoefficient(temperatureIndex_in, exponent_in, courier, condenser_in)
        {
            assign();
            increment = 1.e-5;
        }

      private:
        [[nodiscard]] std::string getFormat() const override { return "Pin[{}]: {}"; }

        std::vector<double>& getCoefficients(HPWH::Condenser::PerformancePoly& perfPoly) override
        {
            return perfPoly.inputPower_coeffs;
        }
    };

    ///	coefficient-of-performance coefficient parameter
    struct COP_Coefficient : public PerformanceCoefficient
    {
        COP_Coefficient(unsigned temperatureIndex_in,
                        unsigned exponent_in,
                        std::shared_ptr<Courier::Courier> courier,
                        HPWH::Condenser* condenser_in)
            : PerformanceCoefficient(temperatureIndex_in, exponent_in, courier, condenser_in)
        {
            assign();
            increment = 1.e-9;
        }

      private:
        [[nodiscard]] std::string getFormat() const override { return "COP[{}]: {}"; }

        std::vector<double>& getCoefficients(HPWH::Condenser::PerformancePoly& perfPoly) override
        {
            return perfPoly.COP_coeffs;
        }
    };

    ///	base class for metric data,
    /// i.e., a target value to match by varying parameters
    struct Metric : public Sender
    {
        double targetValue; // value to be matched
        double tolerance;   // tolerance

        Metric(double targetValue_in, std::shared_ptr<Courier::Courier> courier)
            : Sender("MetricInput", "metricInput", courier)
            , targetValue(targetValue_in)
            , tolerance(1.e-6)
        {
        }

        virtual double evaluate() = 0;
        virtual double findError() = 0;
    };

    /// energy-factor metric, i.e., E50, UEF, E95
    struct EF_Metric : public Metric
    {
        HPWH* hpwh;
        TestConfiguration testConfiguration;
        FirstHourRating::Designation designation;
        TestSummary testSummary;

        EF_Metric(double targetEF,
                  TestConfiguration testConfiguration_in,
                  FirstHourRating::Designation designation_in,
                  std::shared_ptr<Courier::Courier> courier,
                  HPWH* hpwh_in = nullptr)
            : Metric(targetEF, courier)
            , hpwh(hpwh_in)
            , testConfiguration(testConfiguration_in)
            , designation(designation_in)
        {
        }

        virtual ~EF_Metric() = default;

        /// get current EF
        double evaluate() override
        {
            testSummary = hpwh->run24hrTest(testConfiguration, designation, false);
            return testSummary.EF;
        }

        /// find error ratio
        double findError() override { return (evaluate() - targetValue) / tolerance; }

        TestSummary getTestSummary() const { return testSummary; }
    };

    /// metric and parameter data retained as shared pts
  private:
    std::vector<std::shared_ptr<Fitter::Metric>> metrics;
    std::vector<std::shared_ptr<Fitter::Parameter>> parameters;

  public:
    Fitter(std::vector<std::shared_ptr<Fitter::Metric>> metrics_in,
           std::vector<std::shared_ptr<Fitter::Parameter>> parameters_in,
           std::shared_ptr<Courier::Courier> courier)
        : Sender("Fitter", "fitter", courier)
        , metrics(std::move(metrics_in))
        , parameters(std::move(parameters_in))
    {
    }

    // evaluate the current metric using provided single-parameter value
    double getMetricSingleParameter(double x);

    [[nodiscard]] std::string showParameters() const
    {
        std::string s;
        bool first = true;
        for (const auto& parameter : parameters)
        {
            if (!first)
                s.append("\n");
            s.append(parameter->show());
            first = false;
        }
        return s;
    }

    void performLeastSquaresMinimization();
    void fit();
};

#endif
