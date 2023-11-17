#ifndef HPWHMain_hh
#define HPWHMain_hh

#include <string>
#include <sstream>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <functional>
#include <memory>

#include <cstdio>
#include <cstdlib>   //for exit
#include <vector>

#include "HPWH.hh"

class HPWH {
public:
	static const int version_major = HPWHVRSN_MAJOR;
	static const int version_minor = HPWHVRSN_MINOR;
	static const int version_patch = HPWHVRSN_PATCH;
	static const std::string version_maint;  // Initialized in source file (HPWH.cc)

	static const float DENSITYWATER_kgperL;
	static const float KWATER_WpermC;
	static const float CPWATER_kJperkgC;
	static const int CONDENSITY_SIZE = 12;  /**<number of condensity nodes associated with each heat source */
	static const int LOGIC_NODE_SIZE = 12;  /**< number of logic nodes associated with temperature-based heating logic */
	static const int MAXOUTSTRING = 200;  /**< this is the maximum length for a debuging output string */
	static const float TOL_MINVALUE; /**< any amount of heat distribution less than this is reduced to 0 this saves on computations */
	static const float UNINITIALIZED_LOCATIONTEMP;  /**< this is used to tell the
	simulation when the location temperature has not been initialized */
	static const float ASPECTRATIO; /**< A constant to define the aspect ratio between the tank height and
									radius (H/R). Used to find the radius and tank height from the volume and then
									find the surface area. It is derived from the median value of 88
									insulated storage tanks currently available on the market from
									Sanden, AOSmith, HTP, Rheem, and Niles,  */
	static const double MAXOUTLET_R134A; /**< The max oulet temperature for compressors with the refrigerant R134a*/
	static const double MAXOUTLET_R410A; /**< The max oulet temperature for compressors with the refrigerant R410a*/
	static const double MAXOUTLET_R744; /**< The max oulet temperature for compressors with the refrigerant R744*/
	static const double MINSINGLEPASSLIFT; /**< The minimum temperature lift for single pass compressors */

	HPWH();  /**< default constructor */
	HPWH(const HPWH &hpwh);  /**< copy constructor  */
	HPWH & operator=(const HPWH &hpwh);  /**< assignment operator  */
	~HPWH(); /**< destructor just a couple dynamic arrays to destroy - could be replaced by vectors eventually?   */

	///specifies the various modes for the Demand Response (DR) abilities
	///values may vary - names should be used
	enum DRMODES {
		DR_ALLOW = 0b0000,   /**<Allow, this mode allows the water heater to run normally */
		DR_LOC   = 0b0001, /**< Lock out Compressor, this mode locks out the compressor */
		DR_LOR   = 0b0010, /**< Lock out Resistance Elements, this mode locks out the resistance elements */
		DR_TOO   = 0b0100, /**< Top Off Once, this mode ignores the dead band checks and forces the compressor and bottom resistance elements just once. */
		DR_TOT   = 0b1000 /**< Top Off Timer, this mode ignores the dead band checks and forces the compressor and bottom resistance elements
						  every x minutes, where x is defined by timer_TOT. */
	};

	enum MODELS;

	///specifies the modes for writing output
	///the specified values are used for >= comparisons, so the numerical order is relevant
	enum VERBOSITY {
		VRB_silent = 0,     /**< print no outputs  */
		VRB_reluctant = 10,  /**< print only outputs for fatal errors  */
		VRB_minuteOut = 15,    /**< print minutely output  */
		VRB_typical = 20,    /**< print some basic debugging info  */
		VRB_emetic = 30      /**< print all the things  */
	};


	enum UNITS{
		UNITS_C,          /**< celsius  */
		UNITS_F,          /**< fahrenheit  */
		UNITS_KWH,        /**< kilowatt hours  */
		UNITS_BTU,        /**< british thermal units  */
		UNITS_KJ,         /**< kilojoules  */
		UNITS_KW,		  /**< kilowatt  */
		UNITS_BTUperHr,   /**< british thermal units per Hour  */
		UNITS_GAL,        /**< gallons  */
		UNITS_L,          /**< liters  */
		UNITS_kJperHrC,   /**< UA, metric units  */
		UNITS_BTUperHrF,  /**< UA, imperial units  */
		UNITS_FT,		  /**< feet  */
		UNITS_M,		  /**< meters  */
		UNITS_FT2,		  /**< square feet  */
		UNITS_M2,		  /**< square meters  */
		UNITS_MIN,		  /**< minutes  */
		UNITS_SEC,		  /**< seconds  */
		UNITS_HR,		  /**< hours  */
		UNITS_GPM,		  /**< gallons per minute  */
		UNITS_LPS		  /**< liters per second  */
	};

	/** specifies the type of heat source  */
	enum HEATSOURCE_TYPE {
		TYPE_none,        /**< a default to check to make sure it's been set  */
		TYPE_resistance,  /**< a resistance element  */
		TYPE_compressor,   /**< a vapor cycle compressor  */
		TYPE_extra		  /**< an extra element to add user defined heat*/
	};

	/** specifies the extrapolation method based on Tair, from the perfmap for a heat source  */
	enum EXTRAP_METHOD {
		EXTRAP_LINEAR, /**< the default extrapolates linearly */
		EXTRAP_NEAREST /**< extrapolates using nearest neighbor, will just continue from closest point  */
	};

	/** specifies the unit type for outputs in the CSV file-s  */
	enum CSVOPTIONS {
		CSVOPT_NONE,
		CSVOPT_IPUNITS
	};

	struct HeatingLogic;

	struct SoCBasedHeatingLogic;
	std::shared_ptr<SoCBasedHeatingLogic> shutOffSoC(std::string desc,double targetSoC,double hystFract,double tempMinUseful_C,
		bool constMains,double mains_C);
	std::shared_ptr<SoCBasedHeatingLogic> turnOnSoC(std::string desc,double targetSoC,double hystFract,double tempMinUseful_C,
		bool constMains,double mains_C);

	struct NodeWeight;
	struct TempBasedHeatingLogic;
	std::shared_ptr<TempBasedHeatingLogic> topThird(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> topThird_absolute(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> bottomThird(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> bottomHalf(double decisionPoint) ;
	std::shared_ptr<TempBasedHeatingLogic> bottomTwelfth(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> bottomSixth(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> bottomSixth_absolute(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> secondSixth(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> thirdSixth(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> fourthSixth(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> fifthSixth(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> topSixth(double decisionPoint);

	std::shared_ptr<TempBasedHeatingLogic> standby(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> topNodeMaxTemp(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> bottomNodeMaxTemp(double decisionPoint,bool isEnteringWaterHighTempShutoff = false);
	std::shared_ptr<TempBasedHeatingLogic> bottomTwelfthMaxTemp(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> topThirdMaxTemp(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> bottomSixthMaxTemp(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> secondSixthMaxTemp(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> fifthSixthMaxTemp(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> topSixthMaxTemp(double decisionPoint);

	std::shared_ptr<TempBasedHeatingLogic> largeDraw(double decisionPoint);
	std::shared_ptr<TempBasedHeatingLogic> largerDraw(double decisionPoint);

	///this is the value that the public functions will return in case of a simulation
	///destroying error
	static const int HPWH_ABORT = -274000;

	static std::string getVersion();
	/**< This function returns a string with the current version number */

	int HPWHinit_presets(MODELS presetNum);
	/**< This function will reset all member variables to defaults and then
	 * load in a set of parameters that are hardcoded in this function -
	 * which particular set of parameters is selected by presetNum.
	 * This is similar to the way the HPWHsim currently operates, as used in SEEM,
	 * but not quite as versatile.
	 * My impression is that this could be a useful input paradigm for CSE
	 *
	 * The return value is 0 for successful initialization, HPWH_ABORT otherwise
	 */

	int HPWHinit_file(std::string configFile);
	/**< This function will load in a set of parameters from a file
	 * The file name is the input - there should be at most one set of parameters per file
	 * This is useful for testing new variations, and for the sort of variability
	 * that we typically do when creating SEEM runs
	 * Appropriate use of this function can be found in the documentation

	 * The return value is 0 for successful initialization, HPWH_ABORT otherwise
	 */

	int HPWHinit_resTank();  /**< Default resistance tank, EF 0.95, volume 47.5 */
	int HPWHinit_resTank(double tankVol_L,double energyFactor,double upperPower_W,double lowerPower_W);
	/**< This function will initialize a HPWH object to be a resistance tank.  Since
	 * resistance tanks are so simple, they can be specified with only four variables:
	 * tank volume, energy factor, and the power of the upper and lower elements.  Energy
	 * factor is converted into UA internally, although an external setter for UA is
	 * also provided in case the energy factor is unknown.
	 *
	 * Several assumptions regarding the tank configuration are assumed: the lower element
	 * is at the bottom, the upper element is at the top third.  The logics are also set
	 * to standard setting, with upper as VIP activating when the top third is too cold.
	 */

	int HPWHinit_resTankGeneric(double tankVol_L,double rValue_M2KperW,double upperPower_W,double lowerPower_W);
	/**< This function will initialize a HPWH object to be a generic resistance storage water heater,
	* with a specific R-Value defined at initalization.
	*
	* Several assumptions regarding the tank configuration are assumed: the lower element
	* is at the bottom, the upper element is at the top third. The controls are the same standard controls for
	* the HPWHinit_resTank()
	*/

	int HPWHinit_genericHPWH(double tankVol_L,double energyFactor,double resUse_C);
	/**< This function will initialize a HPWH object to be a non-specific HPWH model
	 * with an energy factor as specified.  Since energy
	 * factor is not strongly correlated with energy use, most settings
	 * are taken from the GE2015_STDMode model.
	 */

	int runOneStep(double drawVolume_L,double ambientT_C,
		double externalT_C,DRMODES DRstatus,double inletVol2_L = 0.,double inletT2_C = 0.,
		std::vector<double>* nodePowerExtra_W = NULL);
	/**< This function will progress the simulation forward in time by one step
	 * all calculated outputs are stored in private variables and accessed through functions
	 *
	 * The return value is 0 for successful simulation run, HPWH_ABORT otherwise
	 */

	 /** An overloaded function that uses takes inletT_C  */
	int runOneStep(double inletT_C,double drawVolume_L,double ambientT_C,
		double externalT_C,DRMODES DRstatus,double inletVol2_L = 0.,double inletT2_C = 0.,
		std::vector<double>* nodePowerExtra_W = NULL) {
		setInletT(inletT_C);
		return runOneStep(drawVolume_L,ambientT_C,
			externalT_C,DRstatus,inletVol2_L,inletT2_C,
			nodePowerExtra_W);
	};


	int runNSteps(int N,double *inletT_C,double *drawVolume_L,
		double *tankAmbientT_C,double *heatSourceAmbientT_C,
		DRMODES *DRstatus);
	/**< This function will progress the simulation forward in time by N (equal) steps
	 * The calculated values will be summed or averaged, as appropriate, and
	 * then stored in the usual variables to be accessed through functions
	 *
	 * The return value is 0 for successful simulation run, HPWH_ABORT otherwise
	 */

	 /** Setters for the what are typically input variables  */
	void setInletT(double newInletT_C) { member_inletT_C = newInletT_C; };
	void setMinutesPerStep(double newMinutesPerStep);

	void setVerbosity(VERBOSITY hpwhVrb);
	/**< sets the verbosity to the specified level  */
	void setMessageCallback(void (*callbackFunc)(const std::string message,void* pContext),void* pContext);
	/**< sets the function to be used for message passing  */
	void printHeatSourceInfo();
	/**< this prints out the heat source info, nicely formatted
		specifically input/output energy/power, and runtime
		will print to cout if messageCallback pointer is unspecified
		does not use verbosity, as it is public and expected to be called only when needed  */
	void printTankTemps();
	/**< this prints out all the node temps, kind of nicely formatted
		does not use verbosity, as it is public and expected to be called only when needed  */
	int WriteCSVHeading(FILE* outFILE,const char* preamble = "",int nTCouples = 6,int options = CSVOPT_NONE) const;
	int WriteCSVRow(FILE* outFILE,const char* preamble = "",int nTCouples = 6,int options = CSVOPT_NONE) const;
	/**< a couple of function to write the outputs to a file
		they both will return 0 for success
		the preamble should be supplied with a trailing comma, as these functions do
		not add one.  Additionally, a newline is written with each call.  */

	/**< Sets the tank node temps based on the provided vector of temps, which are mapped onto the 
		existing nodes, regardless of numNodes. */
	int setTankLayerTemperatures(std::vector<double> setTemps, const UNITS units = UNITS_C);
	void getTankTemps(std::vector<double> &tankTemps);

	bool isSetpointFixed() const;  /**< is the setpoint allowed to be changed */
	int setSetpoint(double newSetpoint,UNITS units = UNITS_C);/**<default units C*/
	/**< a function to change the setpoint - useful for dynamically setting it
		The return value is 0 for successful setting, HPWH_ABORT for units failure  */
	double getSetpoint(UNITS units = UNITS_C) const;
	/**< a function to check the setpoint - returns setpoint in celcius  */

	bool isNewSetpointPossible(double newSetpoint_C,double& maxAllowedSetpoint_C,std::string& why,UNITS units = UNITS_C) const;
	/**< This function returns if the new setpoint is physically possible for the compressor. If there
		  is no compressor then checks that the new setpoint is less than boiling. The setpoint can be
		  set higher than the compressor max outlet temperature if there is a  backup resistance element,
		  but the compressor will not operate above this temperature. maxAllowedSetpoint_C returns the */

	double getMaxCompressorSetpoint(UNITS units = UNITS_C) const;
	/**< a function to return the max operating temperature of the compressor which can be different than
		the value returned in isNewSetpointPossible() if there are resistance elements. */

		/** Returns State of Charge where
		   tMains = current mains (cold) water temp,
		   tMinUseful = minimum useful temp,
		   tMax = nominal maximum temp.*/
	double calcSoCFraction(double tMains_C,double tMinUseful_C,double tMax_C) const;
	double calcSoCFraction(double tMains_C,double tMinUseful_C) const {
		return calcSoCFraction(tMains_C,tMinUseful_C,getSetpoint());
	};
	/** Returns State of Charge calculated from the heating logics if this hpwh uses SoC logics. */
	double getSoCFraction() const;

	double getMinOperatingTemp(UNITS units = UNITS_C) const;
	/**< a function to return the minimum operating temperature of the compressor  */

	int resetTankToSetpoint();
	/**< this function resets the tank temperature profile to be completely at setpoint
		The return value is 0 for successful completion  */

	int setTankToTemperature(double temp_C);
	/**< helper function for testing */

	int setAirFlowFreedom(double fanFraction);
	/**< This is a simple setter for the AirFlowFreedom */

	int setDoTempDepression(bool doTempDepress);
	/**< This is a simple setter for the temperature depression option */

	int setTankSize_adjustUA(double HPWH_size,UNITS units = UNITS_L,bool forceChange = false);
	/**< This sets the tank size and adjusts the UA the HPWH currently has to have the same U value but a new A.
		  A is found via getTankSurfaceArea()*/

	double getTankSurfaceArea(UNITS units = UNITS_FT2) const;
	static double getTankSurfaceArea(double vol,UNITS volUnits = UNITS_L,UNITS surfAUnits = UNITS_FT2);
	/**< Returns the tank surface area based off of real storage tanks*/
	double getTankRadius(UNITS units = UNITS_FT) const;
	static double getTankRadius(double vol,UNITS volUnits = UNITS_L,UNITS radiusUnits = UNITS_FT);
	/**< Returns the tank surface radius based off of real storage tanks*/

	bool isTankSizeFixed() const;  /**< is the tank size allowed to be changed */
	int setTankSize(double HPWH_size,UNITS units = UNITS_L,bool forceChange = false);
	/**< Defualt units L. This is a simple setter for the tank volume in L or GAL */

	double getTankSize(UNITS units = UNITS_L) const;
	/**< returns the tank volume in L or GAL  */

	int setDoInversionMixing(bool doInvMix);
	/**< This is a simple setter for the logical for running the inversion mixing method, default is true */

	int setDoConduction(bool doCondu);
	/**< This is a simple setter for doing internal conduction and nodal heatloss, default is true*/

	int setUA(double UA,UNITS units = UNITS_kJperHrC);
	/**< This is a setter for the UA, with or without units specified - default is metric, kJperHrC */

	int getUA(double& UA,UNITS units = UNITS_kJperHrC) const;
	/**< Returns the UA, with or without units specified - default is metric, kJperHrC  */

	int getFittingsUA(double& UA,UNITS units = UNITS_kJperHrC) const;
	/**< Returns the UAof just the fittings, with or without units specified - default is metric, kJperHrC  */

	int setFittingsUA(double UA,UNITS units = UNITS_kJperHrC);
	/**< This is a setter for the UA of just the fittings, with or without units specified - default is metric, kJperHrC */

	int setInletByFraction(double fractionalHeight);
	/**< This is a setter for the water inlet height which sets it as a fraction of the number of nodes from the bottom up*/
	int setInlet2ByFraction(double fractionalHeight);
	/**< This is a setter for the water inlet height which sets it as a fraction of the number of nodes from the bottom up*/

	int setExternalInletHeightByFraction(double fractionalHeight);
	/**< This is a setter for the height at which the split system HPWH adds heated water to the storage tank,
	this sets it as a fraction of the number of nodes from the bottom up*/
	int setExternalOutletHeightByFraction(double fractionalHeight);
	/**< This is a setter for the height at which the split system HPWH takes cold water out of the storage tank,
	this sets it as a fraction of the number of nodes from the bottom up*/

	int setExternalPortHeightByFraction(double fractionalHeight,int whichPort);
	/**< sets the external heater port heights inlet height node number */

	int getExternalInletHeight() const;
	/**< Returns the node where the split system HPWH adds heated water to the storage tank*/
	int getExternalOutletHeight() const;
	/**< Returns the node where the split system HPWH takes cold water out of the storage tank*/

	int setNodeNumFromFractionalHeight(double fractionalHeight,int &inletNum);
	/**< This is a setter for the water inlet height, by fraction. */

	int setTimerLimitTOT(double limit_min);
	/**< Sets the timer limit in minutes for the DR_TOT call. Must be > 0 minutes and < 1440 minutes. */
	double getTimerLimitTOT_minute() const;
	/**< Returns the timer limit in minutes for the DR_TOT call. */

	int getInletHeight(int whichInlet) const;
	/**< returns the water inlet height node number */

	/**< resizes the tankTemp_C and nextTankTemp_C node vectors  */
	void setNumNodes(const std::size_t num_nodes);

	/**< returns the number of nodes  */
	int getNumNodes() const;

	/**< returns the index of the top node  */
	int getIndexTopNode() const;

	double getTankNodeTemp(int nodeNum,UNITS units = UNITS_C) const;
	/**< returns the temperature of the water at the specified node - with specified units
	  or HPWH_ABORT for incorrect node number or unit failure  */

	double getNthSimTcouple(int iTCouple,int nTCouple,UNITS units = UNITS_C) const;
	/**< returns the temperature from a set number of virtual "thermocouples" specified by nTCouple,
		which are constructed from the node temperature array.  Specify iTCouple from 1-nTCouple,
		1 at the bottom using specified units
		returns HPWH_ABORT for iTCouple < 0, > nTCouple, or incorrect units  */

	int getNumHeatSources() const;
	/**< returns the number of heat sources  */

	int getNumResistanceElements() const;
	/**< returns the number of resistance elements  */

	int getCompressorIndex() const;
	/**< returns the index of the compressor in the heat source array.
	Note only supports HPWHs with one compressor, if multiple will return the last index
	of a compressor */

	double getCompressorCapacity(double airTemp = 19.722,double inletTemp = 14.444,double outTemp = 57.222,
		UNITS pwrUnit = UNITS_KW,UNITS tempUnit = UNITS_C);
	/**< Returns the heating output capacity of the compressor for the current HPWH model.
	Note only supports HPWHs with one compressor, if multiple will return the last index
	of a compressor. Outlet temperatures greater than the max allowable setpoints will return an error, but
	for compressors with a fixed setpoint the */

	int setCompressorOutputCapacity(double newCapacity,double airTemp = 19.722,double inletTemp = 14.444,double outTemp = 57.222,
		UNITS pwrUnit = UNITS_KW,UNITS tempUnit = UNITS_C);
	/**< Sets the heating output capacity of the compressor at the defined air, inlet water, and outlet temperatures.
	For multi-pass models the capacity is set as the average between the inletTemp and outTemp since multi-pass models will increase
	the water temperature only a few degrees at a time (i.e. maybe 10 degF) until the tank reaches the outTemp, the capacity at
	inletTemp might not be accurate for the entire heating cycle.
	Note only supports HPWHs with one compressor, if multiple will return the last index
	of a compressor */

	int setScaleHPWHCapacityCOP(double scaleCapacity = 1.,double scaleCOP = 1.);
	/**< Scales the heatpump water heater input capacity and COP*/

	int setResistanceCapacity(double power,int which = -1,UNITS pwrUNIT = UNITS_KW);
	/**< Scale the resistance elements in the heat source list. Which heat source is chosen is changes is given by "which"
	- If which (-1) sets all the resisistance elements in the tank.
	- If which (0, 1, 2...) sets the resistance element in a low to high order.
	  So if there are 3 elements 0 is the bottom, 1 is the middle, and 2 is the top element, regardless of their order
	  in heatSources. If the elements exist on at the same node then all of the elements are set.

	The only valid values for which are between -1 and getNumResistanceElements()-1. Since which is defined as the
	by the ordered height of the resistance elements it cannot refer to a compressor.
	*/

	double getResistanceCapacity(int which = -1,UNITS pwrUNIT = UNITS_KW);
	/**< Returns the resistance elements capacity. Which heat source is chosen is changes is given by "which"
	- If which (-1) gets all the resisistance elements in the tank.
	- If which (0, 1, 2...) sets the resistance element in a low to high order.
	  So if there are 3 elements 0 is the bottom, 1 is the middle, and 2 is the top element, regardless of their order
	  in heatSources. If the elements exist on at the same node then all of the elements are set.

	The only valid values for which are between -1 and getNumResistanceElements()-1. Since which is defined as the
	by the ordered height of the resistance elements it cannot refer to a compressor.
	*/

	int getResistancePosition(int elementIndex) const;

	double getNthHeatSourceEnergyInput(int N,UNITS units = UNITS_KWH) const;
	/**< returns the energy input to the Nth heat source, with the specified units
	  energy used by the heat source is positive - should always be positive
	  returns HPWH_ABORT for N out of bounds or incorrect units  */

	double getNthHeatSourceEnergyOutput(int N,UNITS units = UNITS_KWH) const;
	/**< returns the energy output from the Nth heat source, with the specified units
	  energy put into the water is positive - should always be positive
	  returns HPWH_ABORT for N out of bounds or incorrect units  */
	double getNthHeatSourceRunTime(int N) const;
	/**< returns the run time for the Nth heat source, in minutes
	  note: they may sum to more than 1 time step for concurrently running heat sources
	  returns HPWH_ABORT for N out of bounds  */
	int isNthHeatSourceRunning(int N) const;
	/**< returns 1 if the Nth heat source is currently engaged, 0 if it is not, and
		returns HPWH_ABORT for N out of bounds  */
	HEATSOURCE_TYPE getNthHeatSourceType(int N) const;
	/**< returns the enum value for what type of heat source the Nth heat source is  */


	double getOutletTemp(UNITS units = UNITS_C) const;
	/**< returns the outlet temperature in the specified units
	  returns 0 when no draw occurs, or HPWH_ABORT for incorrect unit specifier  */
	double getCondenserWaterInletTemp(UNITS units = UNITS_C) const;
	/**< returns the condenser inlet temperature in the specified units
	returns 0 when no HP not running occurs, or HPWH_ABORT for incorrect unit specifier  */

	double getCondenserWaterOutletTemp(UNITS units = UNITS_C) const;
	/**< returns the condenser outlet temperature in the specified units
	returns 0 when no HP not running occurs, or HPWH_ABORT for incorrect unit specifier  */

	double getExternalVolumeHeated(UNITS units = UNITS_L) const;
	/**< returns the volume of water heated in an external in the specified units
	  returns 0 when no external heat source is running  */

	double getEnergyRemovedFromEnvironment(UNITS units = UNITS_KWH) const;
	/**< get the total energy removed from the environment by all heat sources in specified units
	  (not net energy - does not include standby)
	  moving heat from the space to the water is the positive direction
	  returns HPWH_ABORT for incorrect units  */

	double getStandbyLosses(UNITS units = UNITS_KWH) const;
	/**< get the amount of heat lost through the tank in specified units
	  moving heat from the water to the space is the positive direction
	  negative should occur seldom
	  returns HPWH_ABORT for incorrect units  */

	double getTankHeatContent_kJ() const;
	/**< get the heat content of the tank, relative to zero celsius
	 * returns using kilojoules */

	int getHPWHModel() const;
	/**< get the model number of the HPWHsim model number of the hpwh */

	int getCompressorCoilConfig() const;
	bool isCompressorMultipass() const;
	bool isCompressoExternalMultipass() const;

	bool hasACompressor() const;
	/**< Returns if the HPWH model has a compressor or not, could be a storage or resistance tank. */

	bool hasExternalHeatSource() const;
	/**< Returns if the HPWH model has any external heat sources or not, could be a compressor or resistance element. */
	double getExternalMPFlowRate(UNITS units = UNITS_GPM) const;
	/**< Returns the constant flow rate for an external multipass heat sources. */

	double getCompressorMinRuntime(UNITS units = UNITS_MIN) const;

	int getSizingFractions(double &aquafract,double &percentUseable) const;
	/**< returns the fraction of total tank volume from the bottom up where the aquastat is
	or the turn on logic for the compressor, and the USEable fraction of storage or 1 minus
	where the shut off logic is for the compressor. If the logic spans multiple nodes it
	returns the weighted average of the nodes */

	bool isHPWHScalable() const;
	/**< returns if the HPWH is scalable or not*/

	bool shouldDRLockOut(HEATSOURCE_TYPE hs,DRMODES DR_signal) const;
	/**< Checks the demand response signal against the different heat source types  */

	void resetTopOffTimer();
	/**< resets variables for timer associated with the DR_TOT call  */

	double getLocationTemp_C() const;
	int setMaxTempDepression(double maxDepression,UNITS units = UNITS_C);

	bool hasEnteringWaterHighTempShutOff(int heatSourceIndex);
	int setEnteringWaterHighTempShutOff(double highTemp,bool tempIsAbsolute,int heatSourceIndex,UNITS units = UNITS_C);
	/**< functions to check for and set specific high temperature shut off logics.
	HPWHs can only have one of these, which is at least typical */

	int setTargetSoCFraction(double target);

	bool canUseSoCControls();

	int switchToSoCControls(double targetSoC,double hysteresisFraction = 0.05,double tempMinUseful = 43.333,bool constantMainsT = false,
		double mainsT = 18.333,UNITS tempUnit = UNITS_C);

	bool isSoCControlled() const;

private:
	class HeatSource;

	void setAllDefaults(); /**< sets all the defaults default */

	void updateTankTemps(double draw,double inletT,double ambientT,double inletVol2_L,double inletT2_L);
	void mixTankInversions();
	/**< Mixes the any temperature inversions in the tank after all the temperature calculations  */
	void updateSoCIfNecessary();

	bool areAllHeatSourcesOff() const;
	/**< test if all the heat sources are off  */
	void turnAllHeatSourcesOff();
	/**< disengage each heat source  */

	void addHeatParent(HeatSource *heatSourcePtr,double heatSourceAmbientT_C,double minutesToRun);

	void addExtraHeat(std::vector<double> &nodePowerExtra_W,double tankAmbientT_C);
	/**< adds extra heat defined by the user, where nodeExtraHeat[] is a vector of heat quantities to be added during the step. 
	nodeExtraHeat[ 0] would go to bottom node, 1 to next etc.  */

	double tankAvg_C(const std::vector<NodeWeight> nodeWeights) const;
	/**< functions to calculate what the temperature in a portion of the tank is  */

	void mixTankNodes(int mixedAboveNode,int mixedBelowNode,double mixFactor);
	/**< function to average the nodes in a tank together bewtween the mixed abovenode and mixed below node. */

	void calcDerivedValues();
	/**< a helper function for the inits, calculating condentropy and the lowest node  */
	void calcSizeConstants();
	/**< a helper function to set constants for the UA and tank size*/
	void calcDerivedHeatingValues();
	/**< a helper for the helper, calculating condentropy and the lowest node*/
	void mapResRelativePosToHeatSources();
	/**< a helper function for the inits, creating a mapping function for the position of the resistance elements
	to their indexes in heatSources. */

	int checkInputs();
	/**< a helper function to run a few checks on the HPWH input parameters  */

	double getChargePerNode(double tCold,double tMix,double tHot) const;

	void calcAndSetSoCFraction();

	void sayMessage(const std::string message) const;
	/**< if the messagePriority is >= the hpwh verbosity,
	either pass your message out to the callback function or print it to cout
	otherwise do nothing  */
	void msg(const char* fmt,...) const;
	void msgV(const char* fmt,va_list ap=NULL) const;

	bool simHasFailed;
	/**< did an internal error cause the simulation to fail?  */

	bool isHeating;
	/**< is the hpwh currently heating or not?  */

	bool setpointFixed;
	/**< does the HPWH allow the setpoint to vary  */

	bool tankSizeFixed;
	/**< does the HPWH have a constant tank size or can it be changed  */

	bool canScale;
	/**< can the HPWH scale capactiy and COP or not  */

	VERBOSITY hpwhVerbosity;
	/**< an enum to let the sim know how much output to say  */

	void (*messageCallback)(const std::string message,void* contextPtr);
	/**< function pointer to indicate an external message processing function  */
	void* messageCallbackContextPtr;
	/**< caller context pointer for external message processing  */

	MODELS hpwhModel;
	/**< The hpwh should know which preset initialized it, or if it was from a fileget */

	// a std::vector containing the HeatSources, in order of priority
	std::vector<HeatSource> heatSources;

	int compressorIndex;
	/**< The index of the compressor heat source (set to -1 if no compressor)*/

	int lowestElementIndex;
	/**< The index of the lowest resistance element heat source (set to -1 if no resistance elements)*/

	int highestElementIndex;
	/**< The index of the highest resistance element heat source. if only one element it equals lowestElementIndex (set to -1 if no resistance elements)*/

	int VIPIndex;
	/**< The index of the VIP resistance element heat source (set to -1 if no VIP resistance elements)*/

	int inletHeight;
	/**< the number of a node in the tank that the inlet water enters the tank at, must be between 0 and numNodes-1  */

	int inlet2Height;
	/**< the number of a node in the tank that the 2nd inlet water enters the tank at, must be between 0 and numNodes-1  */

	/**< the volume in liters of the tank  */
	double tankVolume_L;

	/**< the UA of the tank, in metric units  */
	double tankUA_kJperHrC;

	/**< the UA of the fittings for the tank, in metric units  */
	double fittingsUA_kJperHrC;

	/**< the volume (L) of a single node  */
	double nodeVolume_L;

	/**< the mass of water (kg) in a single node  */
	double nodeMass_kg;

	/**< the heat capacity of the water (kJ/°C) in a single node  */
	double nodeCp_kJperC;

	/**< the height in meters of the one node  */
	double nodeHeight_m;

	double fracAreaTop;
	/**< the fraction of the UA on the top and bottom of the tank, assuming it's a cylinder */
	double fracAreaSide;
	/**< the fraction of the UA on the sides of the tank, assuming it's a cylinder  */

	double currentSoCFraction;
	/**< the current state of charge according to the logic */

	double setpoint_C;
	/**< the setpoint of the tank  */

	/**< holds the temperature of each node - 0 is the bottom node  */
	std::vector<double> tankTemps_C;

	/**< holds the future temperature of each node for the conduction calculation - 0 is the bottom node  */
	std::vector<double> nextTankTemps_C;

	DRMODES prevDRstatus;
	/**< the DRstatus of the tank in the previous time step and at the end of runOneStep */

	double timerLimitTOT;
	/**< the time limit in minutes on the timer when the compressor and resistance elements are turned back on, used with DR_TOT. */
	double timerTOT;
	/**< the timer used for DR_TOT to turn on the compressor and resistance elements. */

	bool usesSoCLogic;

	// Some outputs
	double outletTemp_C;
	/**< the temperature of the outlet water - taken from top of tank, 0 if no flow  */

	double condenserInlet_C;
	/**< the temperature of the inlet water to the condensor either an average of tank nodes or taken from the bottom, 0 if no flow or no compressor  */
	double condenserOutlet_C;
	/**< the temperature of the outlet water from the condensor either, 0 if no flow or no compressor  */
	double externalVolumeHeated_L;
	/**< the volume of water heated by an external source, 0 if no flow or no external heat source  */

	double energyRemovedFromEnvironment_kWh;
	/**< the total energy removed from the environment, to heat the water  */
	double standbyLosses_kWh;
	/**< the amount of heat lost to standby  */

  // special variables for adding abilities
	bool tankMixesOnDraw;
	/**<  whether or not the bottom fraction (defined by mixBelowFraction)
	of the tank should mix during draws  */
	double mixBelowFractionOnDraw;
	/**<  mixes the tank below this fraction on draws iff tankMixesOnDraw  */

	bool doTempDepression;
	/**<  whether the HPWH should use the alternate ambient temperature that
		gets depressed when a compressor is running
		NOTE: this only works for 1 minute steps  */
	double locationTemperature_C;
	/**<  this is the special location temperature that stands in for the the
		ambient temperature if you are doing temp. depression  */
	double maxDepression_C = 2.5;
	/** a couple variables to hold values which are typically inputs  */
	double member_inletT_C;

	double minutesPerStep = 1.;
	double secondsPerStep,hoursPerStep;

	bool doInversionMixing;
	/**<  If and only if true will model temperature inversion mixing in the tank  */

	bool doConduction;
	/**<  If and only if true will model conduction between the internal nodes of the tank  */

	struct resPoint {
		int index;
		int position;
	};
	std::vector<resPoint> resistanceHeightMap;
	/**< A map from index of an resistance element in heatSources to position in the tank, its
	is sorted by height from lowest to highest*/

	/// Generates a vector of logical nodes
	std::vector<HPWH::NodeWeight> getNodeWeightRange(double bottomFraction,double topFraction);

};  //end of HPWH class

#endif
