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

    HeatSource(const HeatSource& heatSource); /// copy constructor

    virtual ~HeatSource() = default;
    HeatSource& operator=(const HeatSource& hSource); /// assignment operator

    void to(hpwh_data_model::rsintegratedwaterheater_ns::HeatSourceConfiguration& config) const;
    void from(const hpwh_data_model::rsintegratedwaterheater_ns::HeatSourceConfiguration& config);

    virtual HEATSOURCE_TYPE typeOfHeatSource() const = 0;
    virtual void to(std::unique_ptr<HeatSourceTemplate>& rshs_ptr) const = 0;
    virtual void from(const std::unique_ptr<HeatSourceTemplate>& rshs_ptr) = 0;
    virtual void calcHeatDist(std::vector<double>& heatDistribution);

    bool isACompressor() const { return typeOfHeatSource() == TYPE_compressor; }
    /**< returns if the heat source uses a compressor or not */
    bool isAResistance() const { return typeOfHeatSource() == TYPE_resistance; }

    bool isEngaged() const;
    /**< return whether or not the heat source is engaged */
    void engageHeatSource(DRMODES DRstatus = DR_ALLOW);
    /**< turn heat source on, i.e. set isEngaged to TRUE */
    void disengageHeatSource();
    /**< turn heat source off, i.e. set isEngaged to FALSE */

    virtual bool maxedOut() const { return false; }
    /**< queries the heat source as to if it shouldn't produce hotter water and the tank isn't at
     * setpoint. */

    bool lockedOut;
    /**< is the condenser locked out	 */

    bool isLockedOut() const { return lockedOut; };

    void lockOutHeatSource() { lockedOut = true; }

    void unlockHeatSource() { lockedOut = false; }

    bool shouldHeat() const;
    /**< queries the heat source as to whether or not it should turn on */
    bool shutsOff() const;
    /**< queries the heat source whether should shut off */

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

  private:
    /** the creator of the heat source, necessary to access HPWH variables */
    HPWH* hpwh;

    // these are the heat source state/output variables
    bool isOn;
    /**< is the heat source running or not	 */

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

    /** a vector to hold the set of logical choices for turning this element on */
    std::vector<std::shared_ptr<HeatingLogic>> turnOnLogicSet;
    /** a vector to hold the set of logical choices that can cause an element to turn off */
    std::vector<std::shared_ptr<HeatingLogic>> shutOffLogicSet;
    /** a single logic that checks the bottom point is below a temperature so the system doesn't
     * short cycle*/
    std::shared_ptr<HeatingLogic> standbyLogic;

    void addTurnOnLogic(std::shared_ptr<HeatingLogic> logic);
    void addShutOffLogic(std::shared_ptr<HeatingLogic> logic);
    /**< these are two small functions to remove some of the cruft in initiation functions */
    void clearAllTurnOnLogic();
    void clearAllShutOffLogic();
    void clearAllLogic();
    /**< these are two small functions to remove some of the cruft in initiation functions */

    double minT;
    /**<  minimum operating temperature of HPWH environment */

    double maxT;
    /**<  maximum operating temperature of HPWH environment */

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

    int lowestNode;
    /**< hold the number of the first non-zero condensity entry */

    double getTankTemp() const;
    /**< returns the tank temperature weighted by the condensity for this heat source */

  protected:
    double heat(double cap_kJ, double maxSetpointT_C);

  public:
}; // end of HeatSource class

#endif
