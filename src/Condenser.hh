#ifndef CONDENSER_hh
#define CONDENSER_hh

#include <btwxt/btwxt.h>

#include "HPWH.hh"
#include "HPWHHeatSource.hh"
#include "Tank.hh"

class HPWH::Condenser : public HPWH::HeatSource
{
  public:
    Condenser(HPWH* hpwh_in = NULL,
              std::shared_ptr<Courier::Courier> courier = std::make_shared<DefaultCourier>(),
              const std::string& name_in = "condenser");

    Condenser& operator=(const Condenser& hSource);

    HEATSOURCE_TYPE typeOfHeatSource() const override { return TYPE_compressor; }

    Description description;
    ProductInformation productInformation;

    void to(std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& p_hs) const override;
    void to(hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE& p_rshs) const;
    void to(hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP& p_rshs) const;

    void from(const std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& p_hs) override;
    void
    from(const hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE& p_rshs);
    void from(const hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP& p_rshs);

    void calcHeatDist(std::vector<double>& heatDistribution) override;

    void setupDefrostMap(double derate35 = 0.8865);
    /**< configure the heat source with a default for the defrost derating */
    void defrostDerate(double& to_derate, double airT_C) const;
    /**< Derates the COP of a system based on the air temperature */

    bool maxedOut() const override;

    bool toLockOrUnlock(double heatSourceAmbientT_C);

    bool shouldLockOut(double heatSourceAmbientT_C) const;

    bool shouldUnlock(double heatSourceAmbientT_C) const;

    double hysteresis_dC;
    /**< a hysteresis term that prevents short cycling due to heat pump self-interaction
      when the heat source is engaged, it is subtracted from lowT cutoffs and
      added to lowTreheat cutoffs */

    double maxSetpoint_C;
    /**< the maximum setpoint of the heat source can create, used for compressors predominately */

    bool useBtwxtGrid;

    // start with a few type definitions
    enum COIL_CONFIG
    {
        CONFIG_SUBMERGED,
        CONFIG_WRAPPED,
        CONFIG_EXTERNAL
    };

    bool doDefrost;
    /**<  If and only if true will derate the COP of a compressor to simulate a defrost cycle  */

    /** some compressors have a resistance element for defrost*/
    struct resistanceElementDefrost
    {
        double inputPwr_kW {0.0};
        double constTempLift_dF {0.0};
        double onBelowT_F {-999};
    };

    resistanceElementDefrost resDefrost;

    struct defrostPoint
    {
        double T_F;
        double derate_fraction;
    };
    std::vector<defrostPoint> defrostMap;
    /**< A list of points for the defrost derate factor ordered by increasing external temperature
     */

    struct maxOut_minAir
    {
        double outT_C;
        double airT_C;
    };
    maxOut_minAir maxOut_at_LowT;
    /**<  maximum output temperature at the minimum operating temperature of HPWH environment
     * (minT)*/

    struct SecondaryHeatExchanger
    {
        double coldSideTemperatureOffset_dC;
        double hotSideTemperatureOffset_dC;
        double extraPumpPower_W;
    } secondaryHeatExchanger;
    /**< adjustments for a approximating a secondary
    heat exchanger by adding extra input energy for the pump and an increaes in the water to the
    incoming waater temperature to the heatpump*/

  public:
    static void linearInterp(double& ynew, double xnew, double x0, double x1, double y0, double y1);

    struct Performance
    {
        double inputPower_W;
        double outputPower_W;
        double cop;
    };

    /// performance polynomial to form a polynomial set of
    /// multiple points at various temperatures.
    /// Linear interpolation is applied to the collection of points.
    struct PerformancePoly
    {
        double T_F;
        std::vector<double> inputPower_coeffs;
        std::vector<double> COP_coeffs;
    };
    std::vector<PerformancePoly> perfPolySet = {};

    /// performance grid data and values to form btwxt RGI
    std::vector<std::vector<double>> perfGrid = {};
    std::vector<std::vector<double>> perfGridValues = {};
    std::shared_ptr<Btwxt::RegularGridInterpolator> perfRGI = {};

    std::function<std::vector<double>(double externalT_C, double condenserT_C)>
        getPerformanceTarget;

    /// core performance-evaluation function
    std::function<Performance(double externalT_C, double condenserT_C)> evaluatePerformance;

    /// general performance function used by all models
    Performance getPerformance(double externalT_C, double condenserT_C) const;

    /// create RGI using grid and model data; assign evaluate-performance function
    void makePerformanceBtwxt()
    {
        auto is_integrated = (configuration != CONFIG_EXTERNAL);
        auto is_Mitsubishi = (hpwh->model == MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP);
        auto is_NyleMP =
            ((MODELS_NyleC60A_MP <= hpwh->model) && (hpwh->model <= MODELS_NyleC250A_C_MP));

        std::vector<Btwxt::GridAxis> grid_axes = {};
        std::size_t iAxis = 0;
        { // external T
            auto interpMethod = (is_Mitsubishi || is_NyleMP || is_integrated)
                                    ? Btwxt::InterpolationMethod::linear
                                    : Btwxt::InterpolationMethod::cubic;
            auto extrapMethod = (is_Mitsubishi || is_NyleMP) ? Btwxt::ExtrapolationMethod::constant
                                                             : Btwxt::ExtrapolationMethod::linear;
            grid_axes.push_back(Btwxt::GridAxis(perfGrid[iAxis],
                                                interpMethod,
                                                extrapMethod,
                                                {-DBL_MAX, DBL_MAX},
                                                "ExternalT",
                                                get_courier()));
            ++iAxis;
        }
        if (perfGrid.size() > 2)
        { // condenser outlet T (CWHS only)
            auto interpMethod = (is_Mitsubishi) ? Btwxt::InterpolationMethod::linear
                                                : Btwxt::InterpolationMethod::linear;
            auto extrapMethod = (is_Mitsubishi) ? Btwxt::ExtrapolationMethod::constant
                                                : Btwxt::ExtrapolationMethod::linear;
            grid_axes.push_back(Btwxt::GridAxis(perfGrid[iAxis],
                                                interpMethod,
                                                extrapMethod,
                                                {-DBL_MAX, DBL_MAX},
                                                "CondenserOutletT",
                                                get_courier()));
            ++iAxis;
        }
        { // heat-source T
            auto interpMethod = (is_Mitsubishi || is_NyleMP) ? Btwxt::InterpolationMethod::linear
                                                             : Btwxt::InterpolationMethod::cubic;
            auto extrapMethod = (is_Mitsubishi)
                                    ? Btwxt::ExtrapolationMethod::linear
                                    : ((is_NyleMP) ? Btwxt::ExtrapolationMethod::constant
                                                   : Btwxt::ExtrapolationMethod::linear);

            grid_axes.push_back(Btwxt::GridAxis(perfGrid[iAxis],
                                                interpMethod,
                                                extrapMethod,
                                                {-DBL_MAX, DBL_MAX},
                                                "HeatSourceT",
                                                get_courier()));
            ++iAxis;
        }

        perfRGI = std::make_shared<Btwxt::RegularGridInterpolator>(
            grid_axes, perfGridValues, "RegularGridInterpolator", get_courier());

        if (perfGrid.size() > 2)
        {
            getPerformanceTarget = [this](double externalT_C, double heatSourceT_C)
            {
                return std::vector<double>(
                    {externalT_C,
                     hpwh->getSetpoint() + secondaryHeatExchanger.hotSideTemperatureOffset_dC,
                     heatSourceT_C});
            };
        }
        else
        {
            getPerformanceTarget = [](double externalT_C, double heatSourceT_C) {
                return std::vector<double>({externalT_C, heatSourceT_C});
            };
        }

        evaluatePerformance = [this](double externalT_C, double heatSourceT_C)
        {
            auto target = getPerformanceTarget(externalT_C, heatSourceT_C);
            std::vector<double> result = perfRGI->get_values_at_target(target);
            Performance performance({result[0], result[1] * result[0], result[1]});
            return performance;
        };

        useBtwxtGrid = true;
    }

    /// one-dimensional quadratic expansions (three terms each) wrt condenser temperature of
    ///     input power and cop at the specified external temperature.
    void makePerformancePolySet()
    {
        getPerformanceTarget = [](double externalT_C, double heatSourceT_C) {
            return std::vector<double>({externalT_C, heatSourceT_C});
        };

        evaluatePerformance = [this](double externalT_C, double heatSourceT_C)
        {
            Performance performance = {0., 0., 0.};

            size_t i_prev = 0;
            size_t i_next = 1;

            double externalT_F = C_TO_F(externalT_C);
            double heatSourceT_F = C_TO_F(heatSourceT_C);
            for (size_t i = 0; i < perfPolySet.size(); ++i)
            {
                if (externalT_F < perfPolySet[i].T_F)
                {
                    if (i == 0)
                    {
                        i_prev = 0;
                        i_next = 1;
                    }
                    else
                    {
                        i_prev = i - 1;
                        i_next = i;
                    }
                    break;
                }
                else
                {
                    if (i == perfPolySet.size() - 1)
                    {
                        i_prev = i - 1;
                        i_next = i;
                        break;
                    }
                }
            }

            // Calculate COP and Input Power at each of the two reference temperatures
            double COP_T1 = perfPolySet[i_prev].COP_coeffs[0];
            COP_T1 += perfPolySet[i_prev].COP_coeffs[1] * heatSourceT_F;
            COP_T1 += perfPolySet[i_prev].COP_coeffs[2] * heatSourceT_F * heatSourceT_F;

            double COP_T2 = perfPolySet[i_next].COP_coeffs[0];
            COP_T2 += perfPolySet[i_next].COP_coeffs[1] * heatSourceT_F;
            COP_T2 += perfPolySet[i_next].COP_coeffs[2] * heatSourceT_F * heatSourceT_F;

            double inputPower_T1_W = perfPolySet[i_prev].inputPower_coeffs[0];
            inputPower_T1_W += perfPolySet[i_prev].inputPower_coeffs[1] * heatSourceT_F;
            inputPower_T1_W +=
                perfPolySet[i_prev].inputPower_coeffs[2] * heatSourceT_F * heatSourceT_F;

            double inputPower_T2_W = perfPolySet[i_next].inputPower_coeffs[0];
            inputPower_T2_W += perfPolySet[i_next].inputPower_coeffs[1] * heatSourceT_F;
            inputPower_T2_W +=
                perfPolySet[i_next].inputPower_coeffs[2] * heatSourceT_F * heatSourceT_F;

            // Interpolate to get COP and input power at the current ambient temperature
            Condenser::linearInterp(performance.cop,
                                    externalT_F,
                                    perfPolySet[i_prev].T_F,
                                    perfPolySet[i_next].T_F,
                                    COP_T1,
                                    COP_T2);
            linearInterp(performance.inputPower_W,
                         externalT_F,
                         perfPolySet[i_prev].T_F,
                         perfPolySet[i_next].T_F,
                         inputPower_T1_W,
                         inputPower_T2_W);
            performance.outputPower_W = performance.cop * performance.inputPower_W;
            return performance;
        };

        useBtwxtGrid = false;
    };

    void makeTier3_performance()
    {
        perfPolySet = {{50., {187.064124, 1.939747, 0.}, {5.22288834, -0.0243008, 0.}},
                       {67.5, {152.9195905, 2.476598, 0.}, {6.643934986, -0.032373288, 0.}},
                       {95., {99.263895, 3.320221, 0.}, {8.87700829, -0.0450586, 0.}}};

        makePerformancePolySet();
    }

    void makeTier4_performance()
    {
        perfPolySet = {{50., {126.9, 2.215, 0.}, {6.931, -0.03395, 0.}},
                       {67.5, {116.6, 2.467, 0.}, {8.833, -0.04431, 0.}},
                       {95., {100.4, 2.863, 0.}, {11.822, -0.06059, 0.}}};

        makePerformancePolySet();
    }

    void makeGridFromPolySet(std::vector<std::vector<double>>& tempGrid,
                             std::vector<std::vector<double>>& tempGridValues) const;

    double inputPowerScale = 1.;
    double COP_scale = 1.;

    /**< Does a simple linear interpolation between two points to the xnew point */

    /// pick the nearest temperature index in a PolySet
    int getAmbientT_index(double ambientT_C);

    double standbyPower_kW;

    bool isExternal() const;

    bool isExternalMultipass() const;

    double mpFlowRate_LPS; /**< The multipass flow rate */

    COIL_CONFIG configuration; /**<  submerged, wrapped, external */
    bool isMultipass; /**< single pass or multi-pass. Anything not obviously split system single
                                                  pass is multipass*/

    double addHeatExternal(double externalT_C, double minutesToRun, Performance& performance);
    /**<  Add heat from a source outside of the tank. Assume the condensity is where
        the water is drawn from and hot water is put at the top of the tank. */

    /// Add heat from external source using a multi-pass configuration
    double addHeatExternalMP(double externalT_C, double minutesToRun, Performance& netPerformance);

    void sortPerformancePolySet();
    /**< sorts the Performance Map by increasing external temperatures */

    void addHeat(double externalT_C, double minutesToRun);

    double getMaxSetpointT_C() const { return maxSetpoint_C; }

    void convertPolySetToGrid();
};

#endif
