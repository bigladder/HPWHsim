	
/*
*	Presets
*/

#ifndef HPWHPresets_hh
#define HPWHPresets_hh

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

///specifies the allowable preset HPWH models
///values may vary - names should be used
enum HPWH::MODELS {
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

	MODELS_AOSmithHPTS50 = 1101,	 /**< 50 gallon, AOSmith HPTS */
	MODELS_AOSmithHPTS66 = 1102,	 /**< 66 gallon, AOSmith HPTS */
	MODELS_AOSmithHPTS80 = 1103,	 /**< 80 gallon, AOSmith HPTS */

	// GE Models
	MODELS_GE2012 = 110,      /**<  The 2012 era GeoSpring  */
	MODELS_GE2014STDMode = 111,    /**< 2014 GE model run in standard mode */
	MODELS_GE2014STDMode_80 = 113,    /**< 2014 GE model run in standard mode, 80 gallon unit */
	MODELS_GE2014 = 112,           /**< 2014 GE model run in the efficiency mode */
	MODELS_GE2014_80 = 114,           /**< 2014 GE model run in the efficiency mode, 80 gallon unit */
	MODELS_GE2014_80DR = 115,           /**< 2014 GE model run in the efficiency mode, 80 gallon unit */
	MODELS_BWC2020_65 = 116,    /**<  The 2020 Bradford White 65 gallon unit  */


	// SANCO2 CO2 transcritical heat pump water heaters
	//   Rebranding Sanden -> SANCO2 5-23
	MODELS_SANCO2_43 = 120,        /**<  SANCO2 43 gallon CO2 external heat pump  */
	MODELS_SANCO2_83 = 121,        /**<  SANCO2 83 gallon CO2 external heat pump  */
	MODELS_SANCO2_GS3_45HPA_US_SP = 122,  /**<  SANCO2 80 gallon CO2 external heat pump used for MF  */
	MODELS_SANCO2_119 = 123,		/**<  SANCO2 120 gallon CO2 external heat pump  */

	// Sanden synomyms for backward compatability
	//  allow unmodified code using HPWHsim to build
	MODELS_Sanden40 = HPWH::MODELS_SANCO2_43,
	MODELS_Sanden80 = HPWH::MODELS_SANCO2_83,
	MODELS_Sanden_GS3_45HPA_US_SP = HPWH::MODELS_SANCO2_GS3_45HPA_US_SP,
	MODELS_Sanden120 = HPWH::MODELS_SANCO2_119,

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

	// Rheem 120V dedicated-circuit product, no resistance elements
	MODELS_RheemPlugInDedicated40 = 1160, /**< 40 gallon, Rheem 120V dedicated-circuit */
	MODELS_RheemPlugInDedicated50 = 1161, /**< 50 gallon, Rheem 120V dedicated-circuit */

	// Rheem 120V shared-circuit products, no resistance elements. 
	MODELS_RheemPlugInShared40 = 1150,	 /**< 40 gallon, Rheem 120V shared-circuit */
	MODELS_RheemPlugInShared50 = 1151,	 /**< 50 gallon, Rheem 120V shared-circuit */
	MODELS_RheemPlugInShared65 = 1152,	 /**< 65 gallon, Rheem 120V shared-circuit */
	MODELS_RheemPlugInShared80 = 1153,	 /**< 80 gallon, Rheem 120V shared-circuit */

	// The new-ish Stiebel
	MODELS_Stiebel220E = 160,      /**< Stiebel Eltron (2014 model?) */

	// Generic water heaters, corresponding to the tiers 1, 2, and 3
	MODELS_Generic1 = 170,         /**< Generic Tier 1 */
	MODELS_Generic2 = 171,         /**< Generic Tier 2 */
	MODELS_Generic3 = 172,          /**< Generic Tier 3 */
	MODELS_UEF2generic = 173,   /**< UEF 2.0, modified GE2014STDMode case */
	MODELS_genericCustomUEF = 174,   /**< used for creating "generic" model with custom uef*/

	MODELS_AWHSTier3Generic40 = 175, /**< Generic AWHS Tier 3 50 gallons*/
	MODELS_AWHSTier3Generic50 = 176, /**< Generic AWHS Tier 3 50 gallons*/
	MODELS_AWHSTier3Generic65 = 177, /**< Generic AWHS Tier 3 65 gallons*/
	MODELS_AWHSTier3Generic80 = 178, /**< Generic AWHS Tier 3 80 gallons*/

	MODELS_StorageTank = 180,  /**< Generic Tank without heaters */
	MODELS_TamScalable_SP = 190, /** < HPWH input passed off a poor preforming SP model that has scalable input capacity and COP  */
	MODELS_Scalable_MP = 191, /** < Lower performance MP model that has scalable input capacity and COP  */

	// Non-preset models
	MODELS_CustomFile = 200,      /**< HPWH parameters were input via file */
	MODELS_CustomResTank = 201,   /**< HPWH parameters were input via HPWHinit_resTank */
	MODELS_CustomResTankGeneric = 202,   /**< HPWH parameters were input via HPWHinit_commercialResTank */

	// Larger Colmac models in single pass configuration
	MODELS_ColmacCxV_5_SP  = 210,  /**<  Colmac CxA_5 external heat pump in Single Pass Mode  */
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
	MODELS_NyleC25A_SP = 230,  /*< Nyle C25A external heat pump in Single Pass Mode  */
	MODELS_NyleC60A_SP = 231,  /*< Nyle C60A external heat pump in Single Pass Mode  */
	MODELS_NyleC90A_SP = 232,  /*< Nyle C90A external heat pump in Single Pass Mode  */
	MODELS_NyleC125A_SP = 233, /*< Nyle C125A external heat pump in Single Pass Mode */
	MODELS_NyleC185A_SP = 234, /*< Nyle C185A external heat pump in Single Pass Mode */
	MODELS_NyleC250A_SP = 235, /*< Nyle C250A external heat pump in Single Pass Mode */
	// Larger Nyle models with the cold weather package!
	MODELS_NyleC60A_C_SP  = 241,  /*< Nyle C60A external heat pump in Single Pass Mode  */
	MODELS_NyleC90A_C_SP  = 242,  /*< Nyle C90A external heat pump in Single Pass Mode  */
	MODELS_NyleC125A_C_SP = 243,  /*< Nyle C125A external heat pump in Single Pass Mode */
	MODELS_NyleC185A_C_SP = 244,  /*< Nyle C185A external heat pump in Single Pass Mode */
	MODELS_NyleC250A_C_SP = 245,  /*< Nyle C250A external heat pump in Single Pass Mode */

	// Mitsubishi Electric Trane
	MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP = 250,  /*< Mitsubishi Electric Trane QAHV external CO2 heat pump  */

	// Larger Nyle models in multi pass configuration
	//MODELS_NyleC25A_MP  = 330,  /*< Nyle C25A external heat pump in Multi Pass Mode  */
	MODELS_NyleC60A_MP  = 331,  /*< Nyle C60A external heat pump in Multi Pass Mode  */
	MODELS_NyleC90A_MP  = 332,  /*< Nyle C90A external heat pump in Multi Pass Mode  */
	MODELS_NyleC125A_MP = 333,  /*< Nyle C125A external heat pump in Multi Pass Mode */
	MODELS_NyleC185A_MP = 334,  /*< Nyle C185A external heat pump in Multi Pass Mode */
	MODELS_NyleC250A_MP = 335,  /*< Nyle C250A external heat pump in Multi Pass Mode */

	MODELS_NyleC60A_C_MP = 341,  /*< Nyle C60A external heat pump in Multi Pass Mode  */
	MODELS_NyleC90A_C_MP = 342,  /*< Nyle C90A external heat pump in Multi Pass Mode  */
	MODELS_NyleC125A_C_MP = 343,  /*< Nyle C125A external heat pump in Multi Pass Mode */
	MODELS_NyleC185A_C_MP = 344,  /*< Nyle C185A external heat pump in Multi Pass Mode */
	MODELS_NyleC250A_C_MP = 345,  /*< Nyle C250A external heat pump in Multi Pass Mode */

	// Large Rheem multi pass models
	MODELS_RHEEM_HPHD60HNU_201_MP = 350,
	MODELS_RHEEM_HPHD60VNU_201_MP = 351,
	MODELS_RHEEM_HPHD135HNU_483_MP = 352, // really bad fit to data due to inconsistency in data
	MODELS_RHEEM_HPHD135VNU_483_MP = 353  // really bad fit to data due to inconsistency in data
};

#endif