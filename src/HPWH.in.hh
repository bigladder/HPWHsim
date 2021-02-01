#ifndef HPWH_hh
#define HPWH_hh

#include <string>
#include <sstream>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <functional>

#include <cstdio>
#include <cstdlib>   //for exit
#include <vector>

//#define HPWH_ABRIDGED
/**<  If HPWH_ABRIDGED is defined, then some function definitions will be
 *  excluded from compiling.  This is done in order to reduce the size of the
 * final compiled code.  */

#define HPWHVRSN_MAJOR @HPWHsim_VRSN_MAJOR@
#define HPWHVRSN_MINOR @HPWHsim_VRSN_MINOR@
#define HPWHVRSN_PATCH @HPWHsim_VRSN_PATCH@
#define HPWHVRSN_META "@HPWHsim_VRSN_META@"

class HPWH {
 public:
  static const int version_major = HPWHVRSN_MAJOR;
  static const int version_minor = HPWHVRSN_MINOR;
  static const int version_patch = HPWHVRSN_PATCH;
  static const std::string version_maint;  // Initialized in source file (HPWH.cc)


  static const float DENSITYWATER_kgperL;
  static const float KWATER_WpermC;
  static const float CPWATER_kJperkgC;
  static const int CONDENSITY_SIZE = 12;  /**< this must be an integer, and only the value 12
  //change at your own risk */
  static const int MAXOUTSTRING = 200;  /**< this is the maximum length for a debuging output string */
  static const float HEATDIST_MINVALUE; /**< any amount of heat distribution less than this is reduced to 0
  //this saves on computations */
  static const float UNINITIALIZED_LOCATIONTEMP;  /**< this is used to tell the
  simulation when the location temperature has not been initialized */
  static const float ASPECTRATIO; /**< A constant to define the aspect ratio between the tank height and
								  radius (H/R). Used to find the radius and tank height from the volume and then
								  find the surface area. It is derived from the median value of 88 
								  insulated storage tanks currently available on the market from
								  Sanden, AOSmith, HTP, Rheem, and Niles,  */

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

  ///specifies the allowable preset HPWH models
  ///values may vary - names should be used
  enum MODELS {
	  // these models are used for testing purposes
	  MODELS_restankNoUA = 1,       /**< a simple resistance tank, but with no tank losses  */
	  MODELS_restankHugeUA = 2,     /**< a simple resistance tank, but with very large tank losses  */
	  MODELS_restankRealistic = 3,  /**< a more-or-less realistic resistance tank  */
	  MODELS_basicIntegrated = 4,   /**< a standard integrated HPWH  */
	  MODELS_externalTest = 5,      /**< a single compressor tank, using "external" topology  */

	  // these models are based on real tanks and measured lab data
	  // AO Smith models
	  MODELS_AOSmithPHPT60 = 102,         /**< this is the Ecotope model for the 60 gallon Voltex HPWH  */
	  MODELS_AOSmithPHPT80 = 103,         /**<  Voltex 80 gallon tank  */
	  MODELS_AOSmithHPTU50 = 104,    /**< 50 gallon AOSmith HPTU */
	  MODELS_AOSmithHPTU66 = 105,    /**< 66 gallon AOSmith HPTU */
	  MODELS_AOSmithHPTU80 = 106,    /**< 80 gallon AOSmith HPTU */
	  MODELS_AOSmithHPTU80_DR = 107,    /**< 80 gallon AOSmith HPTU */
	  MODELS_AOSmithCAHP120 = 108, /**< 12 gallon AOSmith CAHP commercial grade */

	  // GE Models
	  MODELS_GE2012 = 110,      /**<  The 2012 era GeoSpring  */
	  MODELS_GE2014STDMode = 111,    /**< 2014 GE model run in standard mode */
	  MODELS_GE2014STDMode_80 = 113,    /**< 2014 GE model run in standard mode, 80 gallon unit */
	  MODELS_GE2014 = 112,           /**< 2014 GE model run in the efficiency mode */
	  MODELS_GE2014_80 = 114,           /**< 2014 GE model run in the efficiency mode, 80 gallon unit */
	  MODELS_GE2014_80DR = 115,           /**< 2014 GE model run in the efficiency mode, 80 gallon unit */
	  MODELS_BWC2020_65 = 116,    /**<  The 2020 Bradford White 65 gallon unit  */


	  // Sanden CO2 transcritical heat pump water heaters
	  MODELS_Sanden40 = 120,        /**<  Sanden 40 gallon CO2 external heat pump  */
	  MODELS_Sanden80 = 121,        /**<  Sanden 80 gallon CO2 external heat pump  */
	  MODELS_Sanden_GS3_45HPA_US_SP = 122,  /**<  Sanden 80 gallon CO2 external heat pump used for MF  */
	  MODELS_Sanden120 = 123 ,/**<  Sanden 120 gallon CO2 external heat pump  */

	  // The new-ish Rheem
	  MODELS_RheemHB50 = 140,        /**< Rheem 2014 (?) Model */
	  MODELS_RheemHBDR2250 = 141,    /**< 50 gallon, 2250 W resistance Rheem HB Duct Ready */
	  MODELS_RheemHBDR4550 = 142,    /**< 50 gallon, 4500 W resistance Rheem HB Duct Ready */
	  MODELS_RheemHBDR2265 = 143,    /**< 65 gallon, 2250 W resistance Rheem HB Duct Ready */
	  MODELS_RheemHBDR4565 = 144,    /**< 65 gallon, 4500 W resistance Rheem HB Duct Ready */
	  MODELS_RheemHBDR2280 = 145,    /**< 80 gallon, 2250 W resistance Rheem HB Duct Ready */
	  MODELS_RheemHBDR4580 = 146,    /**< 80 gallon, 4500 W resistance Rheem HB Duct Ready */
	  
	  // The new new Rheem
	  MODELS_Rheem2020Prem40  = 151,   /**< 40 gallon, Rheem 2020 Premium */
	  MODELS_Rheem2020Prem50  = 152,   /**< 50 gallon, Rheem 2020 Premium */
	  MODELS_Rheem2020Prem65  = 153,   /**< 65 gallon, Rheem 2020 Premium */
	  MODELS_Rheem2020Prem80  = 154,   /**< 80 gallon, Rheem 2020 Premium */
	  MODELS_Rheem2020Build40 = 155,   /**< 40 gallon, Rheem 2020 Builder */
	  MODELS_Rheem2020Build50 = 156,   /**< 50 gallon, Rheem 2020 Builder */
	  MODELS_Rheem2020Build65 = 157,   /**< 65 gallon, Rheem 2020 Builder */
	  MODELS_Rheem2020Build80 = 158,   /**< 80 gallon, Rheem 2020 Builder */

	  // The new-ish Stiebel
	  MODELS_Stiebel220E = 160,      /**< Stiebel Eltron (2014 model?) */

	  // Generic water heaters, corresponding to the tiers 1, 2, and 3
	  MODELS_Generic1 = 170,         /**< Generic Tier 1 */
	  MODELS_Generic2 = 171,         /**< Generic Tier 2 */
	  MODELS_Generic3 = 172,          /**< Generic Tier 3 */
	  MODELS_UEF2generic = 173,   /**< UEF 2.0, modified GE2014STDMode case */
	  MODELS_genericCustomUEF = 174,   /**< used for creating "generic" model with custom uef*/

	  MODELS_StorageTank = 180,  /**< Generic Tank without heaters */

	  // Non-preset models
	  MODELS_CustomFile = 200,      /**< HPWH parameters were input via file */
	  MODELS_CustomResTank = 201,      /**< HPWH parameters were input via HPWHinit_resTank */

	  // Larger Colmac models in single pass configuration 
	  MODELS_ColmacCxV_5_SP  = 210,	 /**<  Colmac CxA_5 external heat pump in Single Pass Mode  */
	  MODELS_ColmacCxA_10_SP = 211,  /**<  Colmac CxA_10 external heat pump in Single Pass Mode */
	  MODELS_ColmacCxA_15_SP = 212,  /**<  Colmac CxA_15 external heat pump in Single Pass Mode */
	  MODELS_ColmacCxA_20_SP = 213,  /**<  Colmac CxA_20 external heat pump in Single Pass Mode */
	  MODELS_ColmacCxA_25_SP = 214,  /**<  Colmac CxA_25 external heat pump in Single Pass Mode */
	  MODELS_ColmacCxA_30_SP = 215,  /**<  Colmac CxA_30 external heat pump in Single Pass Mode */

	  // Larger Colmac models in multi pass configuration 
	  MODELS_ColmacCxV_5_MP  = 310,	 /**<  Colmac CxA_5 external heat pump in Multi Pass Mode  */
	  MODELS_ColmacCxA_10_MP = 311,  /**<  Colmac CxA_10 external heat pump in Multi Pass Mode */
	  MODELS_ColmacCxA_15_MP = 312,  /**<  Colmac CxA_15 external heat pump in Multi Pass Mode */
	  MODELS_ColmacCxA_20_MP = 313,  /**<  Colmac CxA_20 external heat pump in Multi Pass Mode */
	  MODELS_ColmacCxA_25_MP = 314,  /**<  Colmac CxA_25 external heat pump in Multi Pass Mode */
	  MODELS_ColmacCxA_30_MP = 315,  /**<  Colmac CxA_30 external heat pump in Multi Pass Mode */
	  
	  // Larger Nyle models in single pass configuration
	  MODELS_NyleC25A_SP = 230, /*< Nyle C25A external heat pump in Single Pass Mode  */
	  MODELS_NyleC60A_SP = 231,  /*< Nyle C60A external heat pump in Single Pass Mode  */
	  MODELS_NyleC90A_SP = 232,  /*< Nyle C90A external heat pump in Single Pass Mode  */
	  MODELS_NyleC125A_SP = 233, /*< Nyle C125A external heat pump in Single Pass Mode */
	  MODELS_NyleC185A_SP = 234, /*< Nyle C185A external heat pump in Single Pass Mode */
	  MODELS_NyleC250A_SP = 235, /*< Nyle C250A external heat pump in Single Pass Mode */	 
	  // Larger Nyle models with the cold weather package!
	  MODELS_NyleC60A_C_SP  = 241,  /*< Nyle C60A external heat pump in Single Pass Mode  */
	  MODELS_NyleC90A_C_SP  = 242,  /*< Nyle C90A external heat pump in Single Pass Mode  */
	  MODELS_NyleC125A_C_SP = 243, /*< Nyle C125A external heat pump in Single Pass Mode */
	  MODELS_NyleC185A_C_SP = 244, /*< Nyle C185A external heat pump in Single Pass Mode */
	  MODELS_NyleC250A_C_SP = 245, /*< Nyle C250A external heat pump in Single Pass Mode */

	  // Larger Nyle models in multi pass configuration
	  MODELS_NyleC25A_MP  = 330, /*< Nyle C25A external heat pump in Multi Pass Mode  */
	  MODELS_NyleC60A_MP  = 331,  /*< Nyle C60A external heat pump in Multi Pass Mode  */
	  MODELS_NyleC90A_MP  = 332,  /*< Nyle C90A external heat pump in Multi Pass Mode  */
	  MODELS_NyleC125A_MP = 333, /*< Nyle C125A external heat pump in Multi Pass Mode */
	  MODELS_NyleC185A_MP = 334, /*< Nyle C185A external heat pump in Multi Pass Mode */
	  MODELS_NyleC250A_MP = 335  /*< Nyle C250A external heat pump in Multi Pass Mode */
    };

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
    UNITS_GAL,        /**< gallons  */
    UNITS_L,          /**< liters  */
    UNITS_kJperHrC,   /**< UA, metric units  */
    UNITS_BTUperHrF,  /**< UA, imperial units  */
	UNITS_FT,		  /**< feet  */
	UNITS_M,		  /**< meters */
	UNITS_FT2,		  /**< square feet  */
	UNITS_M2,		  /**< square meters */
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

  struct NodeWeight {
    int nodeNum;
    double weight;
    NodeWeight(int n, double w) : nodeNum(n), weight(w) {};

    NodeWeight(int n) : nodeNum(n), weight(1.0) {};
  };

  struct HeatingLogic {
    std::string description;  // for debug purposes
    std::vector<NodeWeight> nodeWeights;
    double decisionPoint;
    bool isAbsolute;
    std::function<bool(double,double)> compare;
    HeatingLogic(std::string desc, std::vector<NodeWeight> n, double d, bool a=false, std::function<bool(double,double)> c=std::less<double>()) :
      description(desc), nodeWeights(n), decisionPoint(d), isAbsolute(a), compare(c) {};
  };

  HeatingLogic topThird(double d) const;
  HeatingLogic topThird_absolute(double d) const;
	HeatingLogic bottomThird(double d) const;
	HeatingLogic bottomHalf(double d) const;
	HeatingLogic bottomTwelth(double d) const;
	HeatingLogic bottomSixth(double d) const;
	HeatingLogic bottomSixth_absolute(double d) const;
	HeatingLogic secondSixth(double d) const;
	HeatingLogic thirdSixth(double d) const;
	HeatingLogic fourthSixth(double d) const;
	HeatingLogic fifthSixth(double d) const;
	HeatingLogic topSixth(double d) const;

  HeatingLogic standby(double d) const;
  HeatingLogic topNodeMaxTemp(double d) const;
  HeatingLogic bottomNodeMaxTemp(double d) const;
  HeatingLogic bottomTwelthMaxTemp(double d) const;
  HeatingLogic topThirdMaxTemp(double d) const;
  HeatingLogic bottomSixthMaxTemp(double d) const;
  HeatingLogic secondSixthMaxTemp(double d) const;
  HeatingLogic fifthSixthMaxTemp(double d) const;
  HeatingLogic topSixthMaxTemp(double d) const;

  HeatingLogic largeDraw(double d) const;
  HeatingLogic largerDraw(double d) const;


  ///this is the value that the public functions will return in case of a simulation
  ///destroying error
  static const int HPWH_ABORT = -274000;

  static std::string getVersion();
  /**< This function returns a string with the current version number */

	int HPWHinit_presets(MODELS presetNum);
	/**< This function will load in a set of parameters that are hardcoded in this function -
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
   *
   *
	 * The return value is 0 for successful initialization, HPWH_ABORT otherwise
	 */

  int HPWHinit_resTank();  /**< Default resistance tank, EF 0.95, volume 47.5 */
  int HPWHinit_resTank(double tankVol_L, double energyFactor, double upperPower_W, double lowerPower_W);
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

  int HPWHinit_genericHPWH(double tankVol_L, double energyFactor, double resUse_C);
  /**< This function will initialize a HPWH object to be a non-specific HPWH model
   * with an energy factor as specified.  Since energy
   * factor is not strongly correlated with energy use, most settings
   * are taken from the GE2015_STDMode model.
   */

	int runOneStep(double drawVolume_L, double ambientT_C,
		double externalT_C, DRMODES DRstatus, double inletVol2_L = 0., double inletT2_C = 0.,
		std::vector<double>* nodePowerExtra_W = NULL);
	/**< This function will progress the simulation forward in time by one step
	 * all calculated outputs are stored in private variables and accessed through functions
	 *
	 * The return value is 0 for successful simulation run, HPWH_ABORT otherwise
	 */
	
	/** An overloaded function that uses takes inletT_C  */
	int runOneStep(double inletT_C, double drawVolume_L, double ambientT_C, 
		double externalT_C, DRMODES DRstatus, double inletVol2_L = 0., double inletT2_C = 0.,
		std::vector<double>* nodePowerExtra_W = NULL) {
		setInletT(inletT_C);
		return runOneStep(drawVolume_L, ambientT_C,
			externalT_C, DRstatus, inletVol2_L, inletT2_C,
			nodePowerExtra_W);
	};


	int runNSteps(int N,  double *inletT_C, double *drawVolume_L,
                  double *tankAmbientT_C, double *heatSourceAmbientT_C,
                  DRMODES *DRstatus);
	/**< This function will progress the simulation forward in time by N (equal) steps
	 * The calculated values will be summed or averaged, as appropriate, and
	 * then stored in the usual variables to be accessed through functions
	 *
	 * The return value is 0 for successful simulation run, HPWH_ABORT otherwise
	 */

	/** Setters for the what are typically input variables  */
	void setInletT(double newInletT_C) { member_inletT_C = newInletT_C; };
	void setMinutesPerStep(double newMinutesPerStep) { minutesPerStep = newMinutesPerStep; };


  void setVerbosity(VERBOSITY hpwhVrb);
  /**< sets the verbosity to the specified level  */
  void setMessageCallback( void (*callbackFunc)(const std::string message, void* pContext), void* pContext);
  /**< sets the function to be used for message passing  */
  void printHeatSourceInfo();
  /**< this prints out the heat source info, nicely formatted
      specifically input/output energy/power, and runtime
      will print to cout if messageCallback pointer is unspecified
      does not use verbosity, as it is public and expected to be called only when needed  */
  void printTankTemps();
  /**< this prints out all the node temps, kind of nicely formatted
      does not use verbosity, as it is public and expected to be called only when needed  */
  int WriteCSVHeading(FILE* outFILE, const char* preamble = "", int nTCouples = 6, int options = CSVOPT_NONE) const;
  int WriteCSVRow(FILE* outFILE, const char* preamble = "", int nTCouples = 6, int options = CSVOPT_NONE) const;
  /**< a couple of function to write the outputs to a file
      they both will return 0 for success
      the preamble should be supplied with a trailing comma, as these functions do
      not add one.  Additionally, a newline is written with each call.  */



  bool isSetpointFixed() const;  /**< is the setpoint allowed to be changed */
  int setSetpoint(double newSetpoint, UNITS units = UNITS_C);/**<default units C*/
  /**< a function to change the setpoint - useful for dynamically setting it
      The return value is 0 for successful setting, HPWH_ABORT for units failure  */
  double getSetpoint(UNITS units = UNITS_C) const;
  /**< a function to check the setpoint - returns setpoint in celcius  */

  int resetTankToSetpoint();
  /**< this function resets the tank temperature profile to be completely at setpoint
      The return value is 0 for successful completion  */

  int setAirFlowFreedom(double fanFraction);
  /**< This is a simple setter for the AirFlowFreedom */

  int setDoTempDepression(bool doTempDepress);
  /**< This is a simple setter for the temperature depression option */

  int setTankSize_adjustUA(double HPWH_size, UNITS units = UNITS_L, bool forceChange = false);
  /**< This sets the tank size and adjusts the UA the HPWH currently has to have the same U value but a new A.
		A is found via getTankSurfaceArea()*/

  double getTankSurfaceArea(UNITS units = UNITS_FT2) const;
  static double getTankSurfaceArea(double vol, UNITS volUnits = UNITS_L, UNITS surfAUnits = UNITS_FT2);
  /**< Returns the tank surface area based off of real storage tanks*/
  double getTankRadius(UNITS units = UNITS_FT) const;
  static double getTankRadius(double vol, UNITS volUnits = UNITS_L, UNITS radiusUnits = UNITS_FT);
  /**< Returns the tank surface radius based off of real storage tanks*/

  bool isTankSizeFixed() const;  /**< is the tank size allowed to be changed */
  int setTankSize(double HPWH_size, UNITS units = UNITS_L, bool forceChange = false);
  /**< Defualt units L. This is a simple setter for the tank volume in L or GAL */

  double getTankSize(UNITS units = UNITS_L) const;
  /**< returns the tank volume in L or GAL  */

  int setDoInversionMixing(bool doInvMix);
  /**< This is a simple setter for the logical for running the inversion mixing method, default is true */

  int setDoConduction(bool doCondu);
  /**< This is a simple setter for doing internal conduction and nodal heatloss, default is true*/

  int setUA(double UA, UNITS units = UNITS_kJperHrC);
  /**< This is a setter for the UA, with or without units specified - default is metric, kJperHrC */

  int getUA(double& UA, UNITS units = UNITS_kJperHrC) const;
  /**< Returns the UA, with or without units specified - default is metric, kJperHrC  */

  int getFittingsUA(double& UA, UNITS units = UNITS_kJperHrC) const;
  /**< Returns the UAof just the fittings, with or without units specified - default is metric, kJperHrC  */

  int setFittingsUA(double UA, UNITS units = UNITS_kJperHrC);
  /**< This is a setter for the UA of just the fittings, with or without units specified - default is metric, kJperHrC */

  int setInletByFraction(double fractionalHeight);
  /**< This is a setter for the water inlet height which sets it as a fraction of the number of nodes from the bottom up*/
  int setInlet2ByFraction(double fractionalHeight);
  /**< This is a setter for the water inlet height which sets it as a fraction of the number of nodes from the bottom up*/
  int setNodeNumFromFractionalHeight(double fractionalHeight, int &inletNum);
  /**< This is a setter for the water inlet height, by fraction. */

  int setTimerLimitTOT(double limit_min);
  /**< Sets the timer limit in minutes for the DR_TOT call. Must be > 0 minutes and < 1440 minutes. */
  double getTimerLimitTOT_minute() const;
  /**< Returns the timer limit in minutes for the DR_TOT call. */

  int getInletHeight(int whichInlet) const;
  /**< returns the water inlet height node number */

	int getNumNodes() const;
	/**< returns the number of nodes  */
	double getTankNodeTemp(int nodeNum, UNITS units = UNITS_C) const;
	/**< returns the temperature of the water at the specified node - with specified units
      or HPWH_ABORT for incorrect node number or unit failure  */

	double getNthSimTcouple(int iTCouple, int nTCouple, UNITS units = UNITS_C) const;
  /**< returns the temperature from a set number of virtual "thermocouples" specified by nTCouple,
	  which are constructed from the node temperature array.  Specify iTCouple from 1-nTCouple, 
	  1 at the bottom using specified units
      returns HPWH_ABORT for iTCouple < 0, > nTCouple, or incorrect units  */

	int getNumHeatSources() const;
	/**< returns the number of heat sources  */
	double getNthHeatSourceEnergyInput(int N, UNITS units = UNITS_KWH) const;
	/**< returns the energy input to the Nth heat source, with the specified units
      energy used by the heat source is positive - should always be positive
      returns HPWH_ABORT for N out of bounds or incorrect units  */

	double getNthHeatSourceEnergyOutput(int N, UNITS units = UNITS_KWH) const;
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

  bool shouldDRLockOut(HEATSOURCE_TYPE hs, DRMODES DR_signal) const;
  /**< Checks the demand response signal against the different heat source types  */

  void resetTopOffTimer();
  /**< resets variables for timer associated with the DR_TOT call  */

	
  double getLocationTemp_C() const;
  int setMaxTempDepression(double maxDepression, UNITS units = UNITS_C);


 private:
  class HeatSource;

	void updateTankTemps(double draw, double inletT, double ambientT, double inletVol2_L, double inletT2_L);
	void mixTankInversions();
	/**< Mixes the any temperature inversions in the tank after all the temperature calculations  */
	bool areAllHeatSourcesOff() const;
	/**< test if all the heat sources are off  */
	void turnAllHeatSourcesOff();
	/**< disengage each heat source  */

	void addExtraHeat(std::vector<double>* nodePowerExtra_W, double tankAmbientT_C);
	/**< adds extra heat defined by the user. Where nodeExtraHeat[] is a vector of heat quantities to be added during the step.  nodeExtraHeat[ 0] would go to bottom node, 1 to next etc.  */

  double tankAvg_C(const std::vector<NodeWeight> nodeWeights) const;

	/**< functions to calculate what the temperature in a portion of the tank is  */

  void calcDerivedValues();
	/**< a helper function for the inits, calculating condentropy and the lowest node  */
  void calcSizeConstants();
  /**< a helper function to set constants for the UA and tank size*/
  void calcDerivedHeatingValues(); 
  /**< a helper for the helper, calculating condentropy and the lowest node*/


  int checkInputs();
	/**< a helper function to run a few checks on the HPWH input parameters  */

  bool HPWH::areNodeWeightsValid(HPWH::HeatingLogic logic);
	 /**< a helper for the helper, checks the node weights are valid */


  void sayMessage(const std::string message) const;
	/**< if the messagePriority is >= the hpwh verbosity,
    either pass your message out to the callback function or print it to cout
    otherwise do nothing  */
  void msg( const char* fmt, ...) const;
  void msgV( const char* fmt, va_list ap=NULL) const;

  bool simHasFailed;
	/**< did an internal error cause the simulation to fail?  */

	bool isHeating;
	/**< is the hpwh currently heating or not?  */

	bool setpointFixed;
	/**< does the HPWH allow the setpoint to vary  */

	bool tankSizeFixed;
	/**< does the HPWH have a constant tank size or can it be changed  */

  VERBOSITY hpwhVerbosity;
	/**< an enum to let the sim know how much output to say  */

  void (*messageCallback)(const std::string message, void* contextPtr);
	/**< function pointer to indicate an external message processing function  */
  void* messageCallbackContextPtr;
	/**< caller context pointer for external message processing  */



  MODELS hpwhModel;
  /**< The hpwh should know which preset initialized it, or if it was from a file */

	int numHeatSources;
	/**< how many heat sources this HPWH has  */
	HeatSource *setOfSources;
	/**< an array containing the HeatSources, in order of priority  */

	int compressorIndex;
	/**< The index of the compressor heat source (set to -1 if no compressor)*/

	int lowestElementIndex;
	/**< The index of the lowest resistance element heat source (set to -1 if no resistance elements)*/


	int VIPIndex;
	/**< The index of the VIP resistance element heat source (set to -1 if no VIP resistance elements)*/

	int numNodes;
	/**< the number of nodes in the tank - must be >= 12, in multiples of 12  */

	int nodeDensity;
	/**< the number of calculation nodes in a logical node  */

	int inletHeight;
	/**< the number of a node in the tank that the inlet water enters the tank at, must be between 0 and numNodes-1  */
  
	 int inlet2Height;
	/**< the number of a node in the tank that the 2nd inlet water enters the tank at, must be between 0 and numNodes-1  */

	double tankVolume_L;
	/**< the volume in liters of the tank  */
	double tankUA_kJperHrC;
	/**< the UA of the tank, in metric units  */
	double fittingsUA_kJperHrC;
	/**< the UA of the fittings for the tank, in metric units  */

	double volPerNode_LperNode;
	/**< the volume in liters of a single node  */
	double node_height;	
	/**< the height in meters of the one node  */
	double fracAreaTop;	
	/**< the fraction of the UA on the top and bottom of the tank, assuming it's a cylinder */
	double fracAreaSide;	
	/**< the fraction of the UA on the sides of the tank, assuming it's a cylinder  */


	double setpoint_C;
	/**< the setpoint of the tank  */
	double *tankTemps_C;
	/**< an array holding the temperature of each node - 0 is the bottom node, numNodes is the top  */
	double *nextTankTemps_C;
	/**< an array holding the future temperature of each node for the conduction calculation - 0 is the bottom node, numNodes is the top  */

	DRMODES prevDRstatus;
	/**< the DRstatus of the tank in the previous time step and at the end of runOneStep */

	double timerLimitTOT;
	/**< the time limit in minutes on the timer when the compressor and resistance elements are turned back on, used with DR_TOT. */
	double timerTOT;
	/**< the timer used for DR_TOT to turn on the compressor and resistance elements. */


  // Some outputs
	double outletTemp_C;
	/**< the temperature of the outlet water - taken from top of tank, 0 if no flow  */
	double energyRemovedFromEnvironment_kWh;
	/**< the total energy removed from the environment, to heat the water  */
	double standbyLosses_kWh;
	/**< the amount of heat lost to standby  */


  // special variables for adding abilities
	bool tankMixesOnDraw;
	/**<  whether or not the bottom third of the tank should mix during draws  */
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
  double minutesPerStep = 1;

  bool doInversionMixing;
  /**<  If and only if true will model temperature inversion mixing in the tank  */

  bool doConduction;
  /**<  If and only if true will model conduction between the internal nodes of the tank  */

};  //end of HPWH class




class HPWH::HeatSource {
 public:
  friend class HPWH;

	HeatSource(){}  /**< default constructor, does not create a useful HeatSource */
	HeatSource(HPWH *parentHPWH);
  /**< constructor assigns a pointer to the hpwh that owns this heat source  */
  HeatSource(const HeatSource &hSource);  ///copy constructor
  HeatSource& operator=(const HeatSource &hSource); ///assignment operator
  /**< the copy constructor and assignment operator basically just checks if there
      are backup/companion pointers - these can't be copied */

  void setupAsResistiveElement(int node, double Watts);
  /**< configure the heat source to be a resisive element, positioned at the
      specified node, with the specified power in watts */
  void setupExtraHeat(std::vector<double>* nodePowerExtra_W);
  /**< Configure a user defined heat source added as extra, based off using 
		nodePowerExtra_W as the total watt input and the condensity*/

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
  /**< combines shouldLockOut and shouldUnlock to one master function which locks or unlocks the heatsource. Return boolean lockedOut (true if locked, false if unlocked)*/



	bool shouldHeat() const;
  /**< queries the heat source as to whether or not it should turn on */
	bool shutsOff() const;
  /**< queries the heat source whether should shut off */

  int findParent() const;
  /**< returns the index of the heat source where this heat source is a backup.
      returns -1 if none found. */

	void addHeat_temp(double externalT_C);
	void addHeat(double externalT_C, double minutesToRun);
  /**< adds heat to the hpwh - this is the function that interprets the
      various configurations (internal/external, resistance/heat pump) to add heat */

	void setCondensity(double cnd1, double cnd2, double cnd3, double cnd4,
                     double cnd5, double cnd6, double cnd7, double cnd8,
                     double cnd9, double cnd10, double cnd11, double cnd12);
  /**< a function to set the condensity values, it pretties up the init funcs. */
	
	void linearInterp(double &ynew, double xnew, double x0, double x1, double y0, double y1);
	/**< Does a simple linear interpolation between two points to the xnew point */
	void regressedMethod(double &ynew, std::vector<double> &coefficents, double x1, double x2, double x3);
	/**< Does a calculation based on the ten term regression equation  */

	void setupDefrostMap(double derate35 = 0.8865);
	/**< configure the heat source with a default for the defrost derating */
	void defrostDerate(double &to_derate, double airT_C);
	/**< Derates the COP of a system based on the air temperature */

 private:
  //start with a few type definitions
  enum COIL_CONFIG {
    CONFIG_SUBMERGED,
    CONFIG_WRAPPED,
    CONFIG_EXTERNAL
    };

	/** the creator of the heat source, necessary to access HPWH variables */
  HPWH *hpwh;

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

	double condensity[CONDENSITY_SIZE];
	/**< The condensity function is always composed of 12 nodes.
      It represents the location within the tank where heat will be distributed,
      and it also is used to calculate the condenser temperature for inputPower/COP calcs.
      It is conceptually linked to the way condenser coils are wrapped around
      (or within) the tank, however a resistance heat source can also be simulated
      by specifying the entire condensity in one node. */
  double shrinkage;
  /**< the shrinkage is a derived value, using parameters alpha, beta,
      and the condentropy, which is derived from the condensity
      alpha and beta are not intended to be settable
      see the hpwh_init functions for calculation of shrinkage */

  struct perfPoint {
    double T_F;
	std::vector<double>  inputPower_coeffs; // c0 + c1*T + c2*T*T
	std::vector<double>  COP_coeffs; // c0 + c1*T + c2*T*T
  };

  std::vector<perfPoint> perfMap;
  /**< A map with input/COP quadratic curve coefficients at a given external temperature */

	/** a vector to hold the set of logical choices for turning this element on */
	std::vector<HeatingLogic> turnOnLogicSet;
	/** a vector to hold the set of logical choices that can cause an element to turn off */
	std::vector<HeatingLogic> shutOffLogicSet;

	struct defrostPoint {
		double T_F;
		double derate_fraction;
	};
	std::vector<defrostPoint> defrostMap;
	/**< A list of points for the defrost derate factor ordered by increasing external temperature */

	struct maxOut_minAir {
		double outT_C;
		double airT_C;
	 };
	maxOut_minAir maxOut_at_LowT;
	/**<  maximum output temperature at the minimum operating temperature of HPWH environment (minT)*/

  void addTurnOnLogic(HeatingLogic logic);
  void addShutOffLogic(HeatingLogic logic);
  /**< these are two small functions to remove some of the cruft in initiation functions */

  double minT;
  /**<  minimum operating temperature of HPWH environment */

  double maxT;
  /**<  maximum operating temperature of HPWH environment */


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


	COIL_CONFIG configuration; /**<  submerged, wrapped, external */
  HEATSOURCE_TYPE typeOfHeatSource;  /**< compressor, resistance */

	int lowestNode;
  /**< hold the number of the first non-zero condensity entry */

	EXTRAP_METHOD extrapolationMethod; /**< linear or nearest neighbor*/



  // some private functions, mostly used for heating the water with the addHeat function

 	double addHeatAboveNode(double cap_kJ, int node, double minutesToRun);
  /**< adds heat to the set of nodes that are at the same temperature, above the
      specified node number */
  double addHeatExternal(double externalT_C, double minutesToRun, double &cap_BTUperHr, double &input_BTUperHr, double &cop);
  /**<  Add heat from a source outside of the tank. Assume the condensity is where
      the water is drawn from and hot water is put at the top of the tank. */

	/**  I wrote some methods to help with the add heat interface - MJL  */
  void getCapacity(double externalT_C, double condenserTemp_C, double &input_BTUperHr, double &cap_BTUperHr, double &cop);
  void calcHeatDist(std::vector<double> &heatDistribution);

	double getCondenserTemp() const;
  /**< returns the temperature of the condensor - it's a weighted average of the
      tank temperature, using the condensity as weights */

  void sortPerformanceMap();
  /**< sorts the Performance Map by increasing external temperatures */

  /**<  A few helper functions */
  double expitFunc(double x, double offset);
  void normalize(std::vector<double> &distribution);

};  // end of HeatSource class


// a few extra functions for unit converesion
inline double dF_TO_dC(double temperature) { return (temperature*5.0/9.0); }
inline double F_TO_C(double temperature) { return ((temperature - 32.0)*5.0/9.0); }
inline double C_TO_F(double temperature) { return (((9.0/5.0)*temperature) + 32.0); }
inline double KWH_TO_BTU(double kwh) { return (3412.14 * kwh); }
inline double KWH_TO_KJ(double kwh) { return (kwh * 3600.0); }
inline double BTU_TO_KWH(double btu) { return (btu / 3412.14); }
inline double KJ_TO_KWH(double kj) { return (kj/3600.0); }
inline double BTU_TO_KJ(double btu) { return (btu * 1.055); }
inline double GAL_TO_L(double gallons) { return (gallons * 3.78541); }
inline double L_TO_GAL(double liters) { return (liters / 3.78541); }
inline double L_TO_FT3(double liters) { return (liters / 28.31685); }
inline double UAf_TO_UAc(double UAf) { return (UAf * 1.8 / 0.9478); }

inline double FT_TO_M(double feet) { return (feet / 3.2808); }
inline double FT2_TO_M2(double feet2) { return (feet2 / 10.7640); }

inline HPWH::DRMODES operator|(HPWH::DRMODES a, HPWH::DRMODES b)
{
	return static_cast<HPWH::DRMODES>(static_cast<int>(a) | static_cast<int>(b));
}

#endif
