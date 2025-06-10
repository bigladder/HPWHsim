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
    incoming water temperature to the heatpump*/

    static void linearInterp(double& ynew, double xnew, double x0, double x1, double y0, double y1);

    struct Performance
    {
        double inputPower_W;
        double outputPower_W;
        double cop;
    };

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

    /// performance polynomial to form a polynomial set of
    /// multiple points at various temperatures.
    /// Linear interpolation is applied to the collection of points.
    struct PerformancePoly
    {
        double T_F;
        std::vector<double> inputPower_coeffs;
        std::vector<double> COP_coeffs;
    };

    /// general performance function used by all models
    Performance getPerformance(double externalT_C, double condenserT_C) const;

    /// performance grid data and values to form btwxt RGI
    std::shared_ptr<std::vector<std::vector<double>>> pPerfGridValues = {};
    std::shared_ptr<std::vector<std::vector<double>>> pPerfGrid = {};
    std::shared_ptr<Btwxt::RegularGridInterpolator> pPerfRGI = {};

    std::shared_ptr<std::vector<PerformancePoly>> pPerfPolySet = {};

    /// create RGI using grid data; assign evaluate-performance function
    void makePerformanceBtwxt(const std::vector<std::vector<double>>& perfGrid_in,
                              const std::vector<std::vector<double>>& perfGridValues_in);

    /// assign evaluate-performance function using perfPolySet
    void makePerformancePolySet(const std::vector<PerformancePoly>& perfPolySet_in);

    void makeTier3_performance();
    void makeTier4_performance();

    /// internal performance-evaluation function
    std::function<Performance(double externalT_C, double condenserT_C)> evaluatePerformance;

    double inputPowerScale = 1.;
    double COP_scale = 1.;

  private:
    void makeGridFromPolySet(std::vector<std::vector<double>>& tempGrid,
                             std::vector<std::vector<double>>& tempGridValues) const;

    void convertPolySetToGrid();
};

#endif
