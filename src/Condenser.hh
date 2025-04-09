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
              const std::shared_ptr<Courier::Courier> courier = std::make_shared<DefaultCourier>(),
              const std::string& name_in = "condenser");

    Condenser& operator=(const Condenser& hSource);

    HEATSOURCE_TYPE typeOfHeatSource() const override { return TYPE_compressor; }

    ProductInformation productInformation;

    void
    to(std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& rshs_ptr) const override;
    void
    to(hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE& cond_ptr) const;
    void to(hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP& atwhp_ptr) const;

    void
    from(const std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& rshs_ptr) override;
    void
    from(const hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE& cond_ptr);
    void from(const hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP& atwhp_ptr);

    void calcHeatDist(std::vector<double>& heatDistribution) override;

    std::vector<std::vector<double>> perfGrid;
    /**< The axis values defining the regular grid for the performance data.
    SP would have 3 axis, MP would have 2 axis*/

    std::vector<std::vector<double>> perfGridValues;
    /**< The values for input power and cop use matching to the grid. Should be long format with { {
     * inputPower_W }, { COP } }. */

    std::shared_ptr<Btwxt::RegularGridInterpolator> perfRGI;
    /**< The grid interpolator used for mapping performance*/

    void btwxtInterp(double& input_BTUperHr, double& cop, std::vector<double>& target);
    /**< Does a linear interpolation in btwxt to the target point*/

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

    /** specifies the extrapolation method based on Tair, from the perfmap for a heat source  */
    enum EXTRAP_METHOD
    {
        EXTRAP_LINEAR, /**< the default extrapolates linearly */
        EXTRAP_NEAREST /**< extrapolates using nearest neighbor, will just continue from closest
                          point  */
    };

    EXTRAP_METHOD extrapolationMethod; /**< linear or nearest neighbor*/

    void getCapacity(double externalT_C,
                     double condenserTemp_C,
                     double setpointTemp_C,
                     double& input_BTUperHr,
                     double& cap_BTUperHr,
                     double& cop);

    void getCapacityMP(double externalT_C,
                       double condenserTemp_C,
                       double& input_BTUperHr,
                       double& cap_BTUperHr,
                       double& cop);

    /** An overloaded function that uses uses the setpoint temperature  */
    void getCapacity(double externalT_C,
                     double condenserTemp_C,
                     double& input_BTUperHr,
                     double& cap_BTUperHr,
                     double& cop)
    {
        getCapacity(
            externalT_C, condenserTemp_C, hpwh->getSetpoint(), input_BTUperHr, cap_BTUperHr, cop);
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
    };

    SecondaryHeatExchanger secondaryHeatExchanger; /**< adjustments for a approximating a secondary
      heat exchanger by adding extra input energy for the pump and an increaes in the water to the
      incoming waater temperature to the heatpump*/

    /// Polynomials for evaluating input power and COP.
    /// Three different representations of the temperature variations are used:
    /// 1.  one-dimensional quadratic expansions (three terms each) wrt condenser temperature of
    ///     input power and cop at the specified external temperature.
    /// 2.  three-dimensional polynomial (11 terms each) wrt external, outlet, and condenser
    ///     temperatures. The variation with external temperature
    ///     is clipped at the specified temperature.
    /// 3.  two-dimensional polynomial (six terms each) wrt external and condenser
    ///     temperatures.The variation with external temperature
    ///     is clipped at the specified temperature.
    struct PerfPoint
    {
        double T_F;
        std::vector<double> inputPower_coeffs;
        std::vector<double> COP_coeffs;
    };

    /// Performance map containing one or more performance points.
    /// For case 1. above, the map typically contains multiple points, at various temperatures.
    /// Linear interpolation is applied to the collection of points.
    /// Only the first entry is used for cases 2. and 3.
    std::vector<PerfPoint> perfMap;

  public:
    static void linearInterp(double& ynew, double xnew, double x0, double x1, double y0, double y1);
    /**< Does a simple linear interpolation between two points to the xnew point */

    static void regressedMethod(
        double& ynew, const std::vector<double>& coefficents, double x1, double x2, double x3);
    /**< Does a calculation based on the ten term regression equation  */

    static void
    regressedMethodMP(double& ynew, const std::vector<double>& coefficents, double x1, double x2);
    /**< Does a calculation based on the five term regression equation for MP split systems  */

    void getCapacityFromMap(double environmentT_C,
                            double heatSourceT_C,
                            double outletT_C,
                            double& input_BTUperHr,
                            double& cop) const;

    void getCapacityFromMap(double environmentT_C,
                            double heatSourceT_C,
                            double& input_BTUperHr,
                            double& cop) const;

    void makeGridFromMap(std::vector<std::vector<double>>& tempGrid,
                         std::vector<std::vector<double>>& tempGridValues) const;

    void convertMapToGrid();

    double standbyPower_kW;

    bool isExternal() const;

    bool isExternalMultipass() const;

    double mpFlowRate_LPS; /**< The multipass flow rate */

    COIL_CONFIG configuration; /**<  submerged, wrapped, external */
    bool isMultipass; /**< single pass or multi-pass. Anything not obviously split system single
                                                  pass is multipass*/

    double addHeatExternal(double externalT_C,
                           double minutesToRun,
                           double& cap_BTUperHr,
                           double& input_BTUperHr,
                           double& cop);
    /**<  Add heat from a source outside of the tank. Assume the condensity is where
        the water is drawn from and hot water is put at the top of the tank. */

    /// Add heat from external source using a multi-pass configuration
    double addHeatExternalMP(double externalT_C,
                             double minutesToRun,
                             double& cap_BTUperHr,
                             double& input_BTUperHr,
                             double& cop);

    void sortPerformanceMap();
    /**< sorts the Performance Map by increasing external temperatures */

    void addHeat(double externalT_C, double minutesToRun);

    double getMaxSetpointT_C() const { return maxSetpoint_C; }

    void makeBtwxt();
};

#endif
