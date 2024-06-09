#ifndef HPWHHEATSOURCE_hh
#define HPWHHEATSOURCE_hh

#include "HPWH.hh"

class HPWH::HeatSource : public Sender
{
  public:
    friend class HPWH;

    static const int CONDENSITY_SIZE = 12;

    HeatSource(HPWH* hpwh_in = NULL,
               const std::shared_ptr<Courier::Courier> courier = std::make_shared<DefaultCourier>(),
               const std::string& name_in = "heatsource");

    HeatSource(const HeatSource& heatSource);         /// copy constructor
    HeatSource& operator=(const HeatSource& hSource); /// assignment operator

    void from(hpwh_data_model::rsintegratedwaterheater_ns::HeatSourceConfiguration&
                  heatsourceconfiguration);
    void from(hpwh_data_model::rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE&
                  rscondenserwaterheatsource);
    void from(hpwh_data_model::rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE&
                  rsresistancewaterheatsource);

    void to(hpwh_data_model::rsintegratedwaterheater_ns::HeatSourceConfiguration&
                heatsourceconfiguration) const;
    void to(hpwh_data_model::rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE&
                rscondenserwaterheatsource) const;
    void to(hpwh_data_model::rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE&
                rsresistancewaterheatsource) const;

    void setConstantElementPower(double power_W);
    void setupAsResistiveElement(int node, double Watts, int condensitySize = CONDENSITY_SIZE);
    /**< configure the heat source to be a resisive element, positioned at the
        specified node, with the specified power in watts */

    bool isEngaged() const;
    /**< return whether or not the heat source is engaged */
    void engageHeatSource(DRMODES DRstatus = DR_ALLOW);
    /**< turn heat source on, i.e. set isEngaged to TRUE */
    void disengageHeatSource();
    /**< turn heat source off, i.e. set isEngaged to FALSE */

    bool isLockedOut() const;
    /**< return whether or not the heat source is locked out */
    void lockOutHeatSource();
    /**< lockout heat source, i.e. set isLockedOut to TRUE */
    void unlockHeatSource();
    /**< unlock heat source, i.e. set isLockedOut to FALSE */

    bool shouldLockOut(double heatSourceAmbientT_C) const;
    /**< queries the heat source as to whether it should lock out */
    bool shouldUnlock(double heatSourceAmbientT_C) const;
    /**< queries the heat source as to whether it should unlock */

    bool toLockOrUnlock(double heatSourceAmbientT_C);
    /**< combines shouldLockOut and shouldUnlock to one master function which locks or unlocks the
     * heatsource. Return boolean lockedOut (true if locked, false if unlocked)*/

    bool shouldHeat() const;
    /**< queries the heat source as to whether or not it should turn on */
    bool shutsOff() const;
    /**< queries the heat source whether should shut off */

    bool maxedOut() const;
    /**< queries the heat source as to if it shouldn't produce hotter water and the tank isn't at
     * setpoint. */

    [[nodiscard]] int findParent() const;
    /**< returns the index of the heat source where this heat source is a backup.
        returns -1 if none found. */

    double fractToMeetComparisonExternal() const;
    /**< calculates the distance the current state is from the shutOff logic for external
     * configurations*/

    void addHeat(double externalT_C, double minutesToRun);
    /**< adds heat to the hpwh - this is the function that interprets the
        various configurations (internal/external, resistance/heat pump) to add heat */

    /// Assign new condensity values from supplied vector. Note the input vector is currently
    /// resampled to preserve a condensity vector of size CONDENSITY_SIZE.
    void setCondensity(const std::vector<double>& condensity_in);

    int getCondensitySize() const;

    void linearInterp(double& ynew, double xnew, double x0, double x1, double y0, double y1);
    /**< Does a simple linear interpolation between two points to the xnew point */

    void regressedMethod(
        double& ynew, std::vector<double>& coefficents, double x1, double x2, double x3);
    /**< Does a calculation based on the ten term regression equation  */

    void regressedMethodMP(double& ynew, std::vector<double>& coefficents, double x1, double x2);
    /**< Does a calculation based on the five term regression equation for MP split systems  */

    void btwxtInterp(double& input_BTUperHr, double& cop, std::vector<double>& target);
    /**< Does a linear interpolation in btwxt to the target point*/

    void setupDefrostMap(double derate35 = 0.8865);
    /**< configure the heat source with a default for the defrost derating */
    void defrostDerate(double& to_derate, double airT_C);
    /**< Derates the COP of a system based on the air temperature */

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

  private:
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

    /** the creator of the heat source, necessary to access HPWH variables */
    HPWH* hpwh;

    // these are the heat source state/output variables
    bool isOn;
    /**< is the heat source running or not	 */

    bool lockedOut;
    /**< is the heat source locked out	 */

    bool doDefrost;
    /**<  If and only if true will derate the COP of a compressor to simulate a defrost cycle  */

    // some outputs
    double runtime_min;
    /**< this is the percentage of the step that the heat source was running */
    double energyInput_kWh;
    /**< the energy used by the heat source */
    double energyOutput_kWh;
    /**< the energy put into the water by the heat source */

    // these are the heat source property variables
    bool isVIP;
    /**< is this heat source a high priority heat source? (e.g. upper resisitor) */
    HeatSource* backupHeatSource;
    /**< a pointer to the heat source which serves as backup to this one
      should be NULL if no backup exists */
    HeatSource* companionHeatSource;
    /**< a pointer to the heat source which will run concurrently with this one
      it still will only turn on if shutsOff is false */

    HeatSource* followedByHeatSource;
    /**< a pointer to the heat source which will attempt to run after this one */

    //	condensity is represented as a std::vector of size CONDENSITY_SIZE.
    //  It represents the location within the tank where heat will be distributed,
    //  and it also is used to calculate the condenser temperature for inputPower/COP calcs.
    //  It is conceptually linked to the way condenser coils are wrapped around
    //  (or within) the tank, however a resistance heat source can also be simulated
    //  by specifying the entire condensity in one node. */
    std::vector<double> condensity;

    double Tshrinkage_C;
    /**< Tshrinkage_C is a derived from the condentropy (conditional entropy),
        using the condensity and fixed parameters Talpha_C and Tbeta_C.
        Talpha_C and Tbeta_C are not intended to be settable
        see the hpwh_init functions for calculation of shrinkage */

    std::vector<std::vector<double>> perfGrid;
    /**< The axis values defining the regular grid for the performance data.
    SP would have 3 axis, MP would have 2 axis*/

    std::vector<std::vector<double>> perfGridValues;
    /**< The values for input power and cop use matching to the grid. Should be long format with { {
     * inputPower_W }, { COP } }. */

    std::shared_ptr<Btwxt::RegularGridInterpolator> perfRGI;
    /**< The grid interpolator used for mapping performance*/

    bool useBtwxtGrid;

    /** a vector to hold the set of logical choices for turning this element on */
    std::vector<std::shared_ptr<HeatingLogic>> turnOnLogicSet;
    /** a vector to hold the set of logical choices that can cause an element to turn off */
    std::vector<std::shared_ptr<HeatingLogic>> shutOffLogicSet;
    /** a single logic that checks the bottom point is below a temperature so the system doesn't
     * short cycle*/
    std::shared_ptr<HeatingLogic> standbyLogic;

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
        double coldSideTemperatureOffest_dC;
        double hotSideTemperatureOffset_dC;
        double extraPumpPower_W;
    };

    SecondaryHeatExchanger secondaryHeatExchanger; /**< adjustments for a approximating a secondary
      heat exchanger by adding extra input energy for the pump and an increaes in the water to the
      incoming waater temperature to the heatpump*/

    void addTurnOnLogic(std::shared_ptr<HeatingLogic> logic);
    void addShutOffLogic(std::shared_ptr<HeatingLogic> logic);
    /**< these are two small functions to remove some of the cruft in initiation functions */
    void clearAllTurnOnLogic();
    void clearAllShutOffLogic();
    void clearAllLogic();
    /**< these are two small functions to remove some of the cruft in initiation functions */

    void changeResistanceWatts(double watts);
    /**< function to change the resistance wattage */

    bool isACompressor() const;
    /**< returns if the heat source uses a compressor or not */
    bool isAResistance() const;
    /**< returns if the heat source uses a resistance element or not */
    bool isExternalMultipass() const;

    double minT;
    /**<  minimum operating temperature of HPWH environment */

    double maxT;
    /**<  maximum operating temperature of HPWH environment */

    double maxSetpoint_C;
    /**< the maximum setpoint of the heat source can create, used for compressors predominately */

    double hysteresis_dC;
    /**< a hysteresis term that prevents short cycling due to heat pump self-interaction
      when the heat source is engaged, it is subtracted from lowT cutoffs and
      added to lowTreheat cutoffs */

    bool depressesTemperature;
    /**<  heat pumps can depress the temperature of their space in certain instances -
      whether or not this occurs is a bool in HPWH, but a heat source must
      know if it is capable of contributing to this effect or not
      NOTE: this only works for 1 minute steps
      ALSO:  this is set according the the heat source type, not user-specified */

    double airflowFreedom;
    /**< airflowFreedom is the fraction of full flow.  This is used to de-rate compressor
        cop (not capacity) for cases where the air flow is restricted - typically ducting */

    int externalInletHeight; /**<The node height at which the external multipass or single pass HPWH
                                 adds heated water to the storage tank, defaults to top for single
                                 pass. */
    int externalOutletHeight; /**<The node height at which the external multipass or single pass
                                 HPWH adds takes cold water out of the storage tank, defaults to
                                 bottom for single pass.  */

    double mpFlowRate_LPS; /**< The multipass flow rate */

    COIL_CONFIG configuration;        /**<  submerged, wrapped, external */
    HEATSOURCE_TYPE typeOfHeatSource; /**< compressor, resistance, extra, none */
    bool isMultipass; /**< single pass or multi-pass. Anything not obviously split system single
                                         pass is multipass*/

    double standbyPower_kW;
    int lowestNode;
    /**< hold the number of the first non-zero condensity entry */

    EXTRAP_METHOD extrapolationMethod; /**< linear or nearest neighbor*/

    // some private functions, mostly used for heating the water with the addHeat function

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

    /**  I wrote some methods to help with the add heat interface - MJL  */
    void getCapacity(double externalT_C,
                     double condenserTemp_C,
                     double setpointTemp_C,
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
    /** An equivalent getCapcity function just for multipass external (or split) HPWHs  */
    void getCapacityMP(double externalT_C,
                       double condenserTemp_C,
                       double& input_BTUperHr,
                       double& cap_BTUperHr,
                       double& cop);

    void calcHeatDist(std::vector<double>& heatDistribution);

    double getTankTemp() const;
    /**< returns the tank temperature weighted by the condensity for this heat source */

    void sortPerformanceMap();
    /**< sorts the Performance Map by increasing external temperatures */

}; // end of HeatSource class

class Condenser : public HPWH::HeatSource
{
};

class Resistance : public HPWH::HeatSource
{
};
#endif
