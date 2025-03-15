#ifndef HPWH_hh
#define HPWH_hh

#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <functional>
#include <memory>

#include <cstdio>
#include <cstdlib> //for exit
#include <vector>
#include <unordered_map>

#include <courier/courier.h>

namespace Btwxt
{
class RegularGridInterpolator;
}

// #define HPWH_ABRIDGED
/**<  Definition of HPWH_ABRIDGED excludes some functions to reduce the size of the
 * compiled library.  */

#include "HPWHversion.hh"
#include "courier/helpers.h"

class HPWH : public Courier::Sender
{
  public:
    class DefaultCourier : public Courier::DefaultCourier
    {
      protected:
        void write_message(const std::string& message_type, const std::string& message) override
        {
            static bool first_occurrence = true;
            std::cout << fmt::format("  [{}] {}", message_type, message) << std::endl;
            if (first_occurrence)
            {
                std::cout << "  Generated using HPWH::DefaultCourier. Consider deriving your own "
                             "Courier class!"
                          << std::endl;
                first_occurrence = false;
            }
        }
    };

    static const int version_major = HPWHVRSN_MAJOR;
    static const int version_minor = HPWHVRSN_MINOR;
    static const int version_patch = HPWHVRSN_PATCH;
    static const std::string version_maint;

    static const int CONDENSITY_SIZE =
        12; /**<number of condensity nodes associated with each heat source */
    static const int LOGIC_SIZE =
        12; /**< number of logic nodes associated with temperature-based heating logic */
    static const int MAXOUTSTRING =
        200; /**< this is the maximum length for a debuging output string */

    static const double DENSITYWATER_kgperL; /// mass density of water
    static const double KWATER_WpermC;       /// thermal conductivity of water
    static const double CPWATER_kJperkgC;    /// specific heat capcity of water
    static const double TOL_MINVALUE; /**< any amount of heat distribution less than this is reduced
                                         to 0 this saves on computations */

    static const float UNINITIALIZED_LOCATIONTEMP; /**< this is used to tell the
   simulation when the location temperature has not been initialized */
    static const float
        ASPECTRATIO; /**< A constant to define the aspect ratio between the tank height and
                     radius (H/R). Used to find the radius and tank height from the volume and then
                     find the surface area. It is derived from the median value of 88
                     insulated storage tanks currently available on the market from
                     Sanden, AOSmith, HTP, Rheem, and Niles,  */
    static const double
        MAXOUTLET_R134A; /**< The max oulet temperature for compressors with the refrigerant R134a*/
    static const double
        MAXOUTLET_R410A; /**< The max oulet temperature for compressors with the refrigerant R410a*/
    static const double
        MAXOUTLET_R744; /**< The max oulet temperature for compressors with the refrigerant R744*/
    static const double
        MINSINGLEPASSLIFT; /**< The minimum temperature lift for single pass compressors */

    HPWH(const std::shared_ptr<Courier::Courier>& courier = std::make_shared<DefaultCourier>(),
         const std::string& name_in = "hpwh"); /**< default constructor */
    HPWH(const HPWH& hpwh);                    /**< copy constructor  */
    HPWH& operator=(const HPWH& hpwh);         /**< assignment operator  */
    ~HPWH(); /**< destructor just a couple dynamic arrays to destroy - could be replaced by vectors
                eventually?   */

    void set_courier(std::shared_ptr<Courier::Courier> courier_in)
    {
        courier = std::move(courier_in);
    }
    std::shared_ptr<Courier::Courier> get_courier() { return courier; };

    /// specifies the various modes for the Demand Response (DR) abilities
    /// values may vary - names should be used
    enum DRMODES
    {
        DR_ALLOW = 0b0000, /**<Allow, this mode allows the water heater to run normally */
        DR_LOC = 0b0001,   /**< Lock out Compressor, this mode locks out the compressor */
        DR_LOR = 0b0010,   /**< Lock out Resistance Elements, this mode locks out the resistance
                              elements */
        DR_TOO = 0b0100,   /**< Top Off Once, this mode ignores the dead band checks and forces the
                              compressor and bottom resistance elements just once. */
        DR_TOT = 0b1000    /**< Top Off Timer, this mode ignores the dead band checks and forces the
                           compressor and bottom resistance elements    every x minutes, where x is
                           defined by timer_TOT. */
    };

    /// specifies the allowable preset HPWH models
    /// values may vary - names should be used
    enum MODELS
    {
        // these models are used for testing purposes
        MODELS_restankNoUA = 1,   /**< a simple resistance tank, but with no tank losses  */
        MODELS_restankHugeUA = 2, /**< a simple resistance tank, but with very large tank losses  */
        MODELS_restankRealistic = 3, /**< a more-or-less realistic resistance tank  */
        MODELS_basicIntegrated = 4,  /**< a standard integrated HPWH  */
        MODELS_externalTest = 5,     /**< a single compressor tank, using "external" topology  */

        // these models are based on real tanks and measured lab data
        // AO Smith models
        MODELS_AOSmithPHPT60 = 102, /**< this is the Ecotope model for the 60 gallon Voltex HPWH  */
        MODELS_AOSmithPHPT80 = 103, /**<  Voltex 80 gallon tank  */
        MODELS_AOSmithHPTU50 = 104, /**< 50 gallon AOSmith HPTU */
        MODELS_AOSmithHPTU66 = 105, /**< 66 gallon AOSmith HPTU */
        MODELS_AOSmithHPTU80 = 106, /**< 80 gallon AOSmith HPTU */
        MODELS_AOSmithHPTU80_DR = 107, /**< 80 gallon AOSmith HPTU */
        MODELS_AOSmithCAHP120 = 108,   /**< 12 gallon AOSmith CAHP commercial grade */

        MODELS_AOSmithHPTS40 = 1100, /**< 40 gallon, AOSmith HPTS */
        MODELS_AOSmithHPTS50 = 1101, /**< 50 gallon, AOSmith HPTS */
        MODELS_AOSmithHPTS66 = 1102, /**< 66 gallon, AOSmith HPTS */
        MODELS_AOSmithHPTS80 = 1103, /**< 80 gallon, AOSmith HPTS */

        // GE Models
        MODELS_GE2012 = 110,           /**<  The 2012 era GeoSpring  */
        MODELS_GE2014STDMode = 111,    /**< 2014 GE model run in standard mode */
        MODELS_GE2014STDMode_80 = 113, /**< 2014 GE model run in standard mode, 80 gallon unit */
        MODELS_GE2014 = 112,           /**< 2014 GE model run in the efficiency mode */
        MODELS_GE2014_80 = 114,   /**< 2014 GE model run in the efficiency mode, 80 gallon unit */
        MODELS_GE2014_80DR = 115, /**< 2014 GE model run in the efficiency mode, 80 gallon unit */
        MODELS_BWC2020_65 = 116,  /**<  The 2020 Bradford White 65 gallon unit  */

        // SANCO2 CO2 transcritical heat pump water heaters
        //   Rebranding Sanden -> SANCO2 5-23
        MODELS_SANCO2_43 = 120, /**<  SANCO2 43 gallon CO2 external heat pump  */
        MODELS_SANCO2_83 = 121, /**<  SANCO2 83 gallon CO2 external heat pump  */
        MODELS_SANCO2_GS3_45HPA_US_SP =
            122,                 /**<  SANCO2 80 gallon CO2 external heat pump used for MF  */
        MODELS_SANCO2_119 = 123, /**<  SANCO2 120 gallon CO2 external heat pump  */

        // Sanden synomyms for backward compatability
        //  allow unmodified code using HPWHsim to build
        MODELS_Sanden40 = MODELS_SANCO2_43,
        MODELS_Sanden80 = MODELS_SANCO2_83,
        MODELS_Sanden_GS3_45HPA_US_SP = MODELS_SANCO2_GS3_45HPA_US_SP,
        MODELS_Sanden120 = MODELS_SANCO2_119,

        // The new-ish Rheem
        MODELS_RheemHB50 = 140,     /**< Rheem 2014 (?) Model */
        MODELS_RheemHBDR2250 = 141, /**< 50 gallon, 2250 W resistance Rheem HB Duct Ready */
        MODELS_RheemHBDR4550 = 142, /**< 50 gallon, 4500 W resistance Rheem HB Duct Ready */
        MODELS_RheemHBDR2265 = 143, /**< 65 gallon, 2250 W resistance Rheem HB Duct Ready */
        MODELS_RheemHBDR4565 = 144, /**< 65 gallon, 4500 W resistance Rheem HB Duct Ready */
        MODELS_RheemHBDR2280 = 145, /**< 80 gallon, 2250 W resistance Rheem HB Duct Ready */
        MODELS_RheemHBDR4580 = 146, /**< 80 gallon, 4500 W resistance Rheem HB Duct Ready */

        // The new new Rheem
        MODELS_Rheem2020Prem40 = 151,  /**< 40 gallon, Rheem 2020 Premium */
        MODELS_Rheem2020Prem50 = 152,  /**< 50 gallon, Rheem 2020 Premium */
        MODELS_Rheem2020Prem65 = 153,  /**< 65 gallon, Rheem 2020 Premium */
        MODELS_Rheem2020Prem80 = 154,  /**< 80 gallon, Rheem 2020 Premium */
        MODELS_Rheem2020Build40 = 155, /**< 40 gallon, Rheem 2020 Builder */
        MODELS_Rheem2020Build50 = 156, /**< 50 gallon, Rheem 2020 Builder */
        MODELS_Rheem2020Build65 = 157, /**< 65 gallon, Rheem 2020 Builder */
        MODELS_Rheem2020Build80 = 158, /**< 80 gallon, Rheem 2020 Builder */

        // Rheem 120V dedicated-circuit product, no resistance elements
        MODELS_RheemPlugInDedicated40 = 1160, /**< 40 gallon, Rheem 120V dedicated-circuit */
        MODELS_RheemPlugInDedicated50 = 1161, /**< 50 gallon, Rheem 120V dedicated-circuit */

        // Rheem 120V shared-circuit products, no resistance elements.
        MODELS_RheemPlugInShared40 = 1150, /**< 40 gallon, Rheem 120V shared-circuit */
        MODELS_RheemPlugInShared50 = 1151, /**< 50 gallon, Rheem 120V shared-circuit */
        MODELS_RheemPlugInShared65 = 1152, /**< 65 gallon, Rheem 120V shared-circuit */
        MODELS_RheemPlugInShared80 = 1153, /**< 80 gallon, Rheem 120V shared-circuit */

        // The new-ish Stiebel
        MODELS_Stiebel220E = 160, /**< Stiebel Eltron (2014 model?) */

        // Generic water heaters, corresponding to the tiers 1, 2, and 3
        MODELS_Generic1 = 170,         /**< Generic Tier 1 */
        MODELS_Generic2 = 171,         /**< Generic Tier 2 */
        MODELS_Generic3 = 172,         /**< Generic Tier 3 */
        MODELS_UEF2generic = 173,      /**< UEF 2.0, modified GE2014STDMode case */
        MODELS_genericCustomUEF = 174, /**< used for creating "generic" model with custom uef*/

        MODELS_AWHSTier3Generic40 = 175, /**< Generic AWHS Tier 3 50 gallons*/
        MODELS_AWHSTier3Generic50 = 176, /**< Generic AWHS Tier 3 50 gallons*/
        MODELS_AWHSTier3Generic65 = 177, /**< Generic AWHS Tier 3 65 gallons*/
        MODELS_AWHSTier3Generic80 = 178, /**< Generic AWHS Tier 3 80 gallons*/

        MODELS_GenericUEF217 = 179,

        MODELS_AWHSTier4Generic40 = 1175, /**< Generic AWHS Tier 4 40 gallons*/
        MODELS_AWHSTier4Generic50 = 1176, /**< Generic AWHS Tier 4 50 gallons*/
        MODELS_AWHSTier4Generic65 = 1177, /**< Generic AWHS Tier 4 65 gallons*/
        MODELS_AWHSTier4Generic80 = 1178, /**< Generic AWHS Tier 4 80 gallons*/

        MODELS_StorageTank = 180, /**< Generic Tank without heaters */

        MODELS_TamScalable_SP = 190, /** < HPWH input passed off a poor preforming SP model that
                                        has scalable input capacity and COP  */
        MODELS_TamScalable_SP_2X = 191,
        MODELS_TamScalable_SP_Half = 192,

        MODELS_Scalable_MP =
            193, /** < Lower performance MP model that has scalable input capacity and COP  */

        // Non-preset models
        MODELS_CustomFile = 200,    /**< HPWH parameters were input via file */
        MODELS_CustomResTank = 201, /**< HPWH parameters were input via HPWHinit_resTank */
        MODELS_CustomResTankGeneric =
            202, /**< HPWH parameters were input via HPWHinit_commercialResTank */

        // Larger Colmac models in single pass configuration
        MODELS_ColmacCxV_5_SP = 210,  /**<  Colmac CxA_5 external heat pump in Single Pass Mode  */
        MODELS_ColmacCxA_10_SP = 211, /**<  Colmac CxA_10 external heat pump in Single Pass Mode */
        MODELS_ColmacCxA_15_SP = 212, /**<  Colmac CxA_15 external heat pump in Single Pass Mode */
        MODELS_ColmacCxA_20_SP = 213, /**<  Colmac CxA_20 external heat pump in Single Pass Mode */
        MODELS_ColmacCxA_25_SP = 214, /**<  Colmac CxA_25 external heat pump in Single Pass Mode */
        MODELS_ColmacCxA_30_SP = 215, /**<  Colmac CxA_30 external heat pump in Single Pass Mode */

        // Larger Colmac models in multi pass configuration
        MODELS_ColmacCxV_5_MP = 310,  /**<  Colmac CxA_5 external heat pump in Multi Pass Mode  */
        MODELS_ColmacCxA_10_MP = 311, /**<  Colmac CxA_10 external heat pump in Multi Pass Mode */
        MODELS_ColmacCxA_15_MP = 312, /**<  Colmac CxA_15 external heat pump in Multi Pass Mode */
        MODELS_ColmacCxA_20_MP = 313, /**<  Colmac CxA_20 external heat pump in Multi Pass Mode */
        MODELS_ColmacCxA_25_MP = 314, /**<  Colmac CxA_25 external heat pump in Multi Pass Mode */
        MODELS_ColmacCxA_30_MP = 315, /**<  Colmac CxA_30 external heat pump in Multi Pass Mode */

        // Larger Nyle models in single pass configuration
        MODELS_NyleC25A_SP = 230,  /*< Nyle C25A external heat pump in Single Pass Mode  */
        MODELS_NyleC60A_SP = 231,  /*< Nyle C60A external heat pump in Single Pass Mode  */
        MODELS_NyleC90A_SP = 232,  /*< Nyle C90A external heat pump in Single Pass Mode  */
        MODELS_NyleC125A_SP = 233, /*< Nyle C125A external heat pump in Single Pass Mode */
        MODELS_NyleC185A_SP = 234, /*< Nyle C185A external heat pump in Single Pass Mode */
        MODELS_NyleC250A_SP = 235, /*< Nyle C250A external heat pump in Single Pass Mode */
        // Larger Nyle models with the cold weather package!
        MODELS_NyleC60A_C_SP = 241,  /*< Nyle C60A external heat pump in Single Pass Mode  */
        MODELS_NyleC90A_C_SP = 242,  /*< Nyle C90A external heat pump in Single Pass Mode  */
        MODELS_NyleC125A_C_SP = 243, /*< Nyle C125A external heat pump in Single Pass Mode */
        MODELS_NyleC185A_C_SP = 244, /*< Nyle C185A external heat pump in Single Pass Mode */
        MODELS_NyleC250A_C_SP = 245, /*< Nyle C250A external heat pump in Single Pass Mode */

        // Mitsubishi Electric Trane
        MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP =
            250, /*< Mitsubishi Electric Trane QAHV external CO2 heat pump  */

        // Larger Nyle models in multi pass configuration
        // MODELS_NyleC25A_MP  = 330,  /*< Nyle C25A external heat pump in Multi Pass Mode  */
        MODELS_NyleC60A_MP = 331,  /*< Nyle C60A external heat pump in Multi Pass Mode  */
        MODELS_NyleC90A_MP = 332,  /*< Nyle C90A external heat pump in Multi Pass Mode  */
        MODELS_NyleC125A_MP = 333, /*< Nyle C125A external heat pump in Multi Pass Mode */
        MODELS_NyleC185A_MP = 334, /*< Nyle C185A external heat pump in Multi Pass Mode */
        MODELS_NyleC250A_MP = 335, /*< Nyle C250A external heat pump in Multi Pass Mode */

        MODELS_NyleC60A_C_MP = 341,  /*< Nyle C60A external heat pump in Multi Pass Mode  */
        MODELS_NyleC90A_C_MP = 342,  /*< Nyle C90A external heat pump in Multi Pass Mode  */
        MODELS_NyleC125A_C_MP = 343, /*< Nyle C125A external heat pump in Multi Pass Mode */
        MODELS_NyleC185A_C_MP = 344, /*< Nyle C185A external heat pump in Multi Pass Mode */
        MODELS_NyleC250A_C_MP = 345, /*< Nyle C250A external heat pump in Multi Pass Mode */

        // Large Rheem multi pass models
        MODELS_RHEEM_HPHD60HNU_201_MP = 350,
        MODELS_RHEEM_HPHD60VNU_201_MP = 351,
        MODELS_RHEEM_HPHD135HNU_483_MP = 352, // really bad fit to data due to inconsistency in data
        MODELS_RHEEM_HPHD135VNU_483_MP = 353, // really bad fit to data due to inconsistency in data

        MODELS_AquaThermAire = 400, // heat exchanger model

        MODELS_BradfordWhiteAeroThermRE2H50 = 500,
        MODELS_BradfordWhiteAeroThermRE2H65 = 501,
        MODELS_BradfordWhiteAeroThermRE2H80 = 502,

        MODELS_LG_APHWC50 = 600,
        MODELS_LG_APHWC80 = 601
    };

    /// specifies the modes for writing output
    /// the specified values are used for >= comparisons, so the numerical order is relevant
    enum VERBOSITY
    {
        VRB_silent = 0,     /**< print no outputs  */
        VRB_reluctant = 10, /**< print only outputs for fatal errors  */
        VRB_minuteOut = 15, /**< print minutely output  */
        VRB_typical = 20,   /**< print some basic debugging info  */
        VRB_emetic = 30     /**< print all the things  */
    };

    enum UNITS
    {
        UNITS_C,         /**< celsius  */
        UNITS_F,         /**< fahrenheit  */
        UNITS_KWH,       /**< kilowatt hours  */
        UNITS_BTU,       /**< british thermal units  */
        UNITS_KJ,        /**< kilojoules  */
        UNITS_KW,        /**< kilowatt  */
        UNITS_BTUperHr,  /**< british thermal units per Hour  */
        UNITS_GAL,       /**< gallons  */
        UNITS_L,         /**< liters  */
        UNITS_kJperHrC,  /**< UA, metric units  */
        UNITS_BTUperHrF, /**< UA, imperial units  */
        UNITS_FT,        /**< feet  */
        UNITS_M,         /**< meters  */
        UNITS_FT2,       /**< square feet  */
        UNITS_M2,        /**< square meters  */
        UNITS_MIN,       /**< minutes  */
        UNITS_SEC,       /**< seconds  */
        UNITS_HR,        /**< hours  */
        UNITS_GPM,       /**< gallons per minute  */
        UNITS_LPS        /**< liters per second  */
    };

    /** specifies the type of heat source  */
    enum HEATSOURCE_TYPE
    {
        TYPE_none,       /**< a default to check to make sure it's been set  */
        TYPE_resistance, /**< a resistance element  */
        TYPE_compressor  /**< a vapor cycle compressor  */
    };

    /** specifies the extrapolation method based on Tair, from the perfmap for a heat source  */
    enum EXTRAP_METHOD
    {
        EXTRAP_LINEAR, /**< the default extrapolates linearly */
        EXTRAP_NEAREST /**< extrapolates using nearest neighbor, will just continue from closest
                          point  */
    };

    /** specifies the unit type for outputs in the CSV file-s  */
    enum CSVOPTIONS
    {
        CSVOPT_NONE = 0,
        CSVOPT_IPUNITS = 1 << 0,
        CSVOPT_IS_DRAWING = 1 << 1
    };

    struct NodeWeight
    {
        int nodeNum;
        double weight;
        NodeWeight(int n, double w) : nodeNum(n), weight(w) {};

        NodeWeight(int n) : nodeNum(n), weight(1.0) {};
    };

    struct HeatingLogic
    {
      public:
        std::string description;
        std::function<bool(double, double)> compare;

        HeatingLogic(std::string desc,
                     double decisionPoint_in,
                     HPWH* hpwh_in,
                     std::function<bool(double, double)> c,
                     bool isHTS)
            : description(desc)
            , compare(c)
            , decisionPoint(decisionPoint_in)
            , hpwh(hpwh_in)
            , isEnteringWaterHighTempShutoff(isHTS) {};

        virtual ~HeatingLogic() = default;

        /**< checks that the input is all valid. */
        virtual bool isValid() = 0;
        /**< gets the value for comparing the tank value to, i.e. the target SoC */
        virtual double getComparisonValue() = 0;
        /**< gets the calculated value from the tank, i.e. SoC or tank average of node weights*/
        virtual double getTankValue() = 0;
        /**< function to calculate where the average node for a logic set is. */
        virtual double nodeWeightAvgFract() = 0;
        /**< gets the fraction of a node that has to be heated up to met the turnoff condition*/
        virtual double getFractToMeetComparisonExternal() = 0;

        virtual void setDecisionPoint(double value) = 0;
        double getDecisionPoint() { return decisionPoint; }
        bool getIsEnteringWaterHighTempShutoff() { return isEnteringWaterHighTempShutoff; }

      protected:
        double decisionPoint;
        HPWH* hpwh;
        bool isEnteringWaterHighTempShutoff;
    };

    struct SoCBasedHeatingLogic : HeatingLogic
    {
      public:
        SoCBasedHeatingLogic(std::string desc,
                             double decisionPoint,
                             HPWH* hpwh,
                             double hF = -0.05,
                             double tM_C = 43.333,
                             bool constMains = false,
                             double mains_C = 18.333,
                             std::function<bool(double, double)> c = std::less<double>())
            : HeatingLogic(desc, decisionPoint, hpwh, c, false)
            , tempMinUseful_C(tM_C)
            , hysteresisFraction(hF)
            , useCostantMains(constMains)
            , constantMains_C(mains_C) {};
        bool isValid();

        double getComparisonValue();
        double getTankValue();
        double nodeWeightAvgFract();
        double getFractToMeetComparisonExternal();
        double getMainsT_C();
        double getTempMinUseful_C();
        void setDecisionPoint(double value);
        void setConstantMainsTemperature(double mains_C);

      private:
        double tempMinUseful_C;
        double hysteresisFraction;
        bool useCostantMains;
        double constantMains_C;
    };

    struct TempBasedHeatingLogic : HeatingLogic
    {
      public:
        TempBasedHeatingLogic(std::string desc,
                              std::vector<NodeWeight> n,
                              double decisionPoint,
                              HPWH* hpwh,
                              bool a = false,
                              std::function<bool(double, double)> c = std::less<double>(),
                              bool isHTS = false)
            : HeatingLogic(desc, decisionPoint, hpwh, c, isHTS), isAbsolute(a), nodeWeights(n) {};

        bool isValid();

        double getComparisonValue();
        double getTankValue();
        double nodeWeightAvgFract();
        double getFractToMeetComparisonExternal();

        void setDecisionPoint(double value);
        void setDecisionPoint(double value, bool absolute);

      private:
        bool areNodeWeightsValid();

        bool isAbsolute;
        std::vector<NodeWeight> nodeWeights;
    };

    std::shared_ptr<HPWH::SoCBasedHeatingLogic> shutOffSoC(std::string desc,
                                                           double targetSoC,
                                                           double hystFract,
                                                           double tempMinUseful_C,
                                                           bool constMains,
                                                           double mains_C);
    std::shared_ptr<HPWH::SoCBasedHeatingLogic> turnOnSoC(std::string desc,
                                                          double targetSoC,
                                                          double hystFract,
                                                          double tempMinUseful_C,
                                                          bool constMains,
                                                          double mains_C);

    std::shared_ptr<TempBasedHeatingLogic>
    wholeTank(double decisionPoint, const UNITS units = UNITS_C, const bool absolute = false);
    std::shared_ptr<TempBasedHeatingLogic> topThird(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> topThird_absolute(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic>
    secondThird(double decisionPoint, const UNITS units = UNITS_C, const bool absolute = false);
    std::shared_ptr<TempBasedHeatingLogic> bottomThird(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> bottomHalf(double decisionPoint);
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
    std::shared_ptr<TempBasedHeatingLogic>
    bottomNodeMaxTemp(double decisionPoint, bool isEnteringWaterHighTempShutoff = false);
    std::shared_ptr<TempBasedHeatingLogic> bottomTwelfthMaxTemp(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> topThirdMaxTemp(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> bottomSixthMaxTemp(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> secondSixthMaxTemp(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> fifthSixthMaxTemp(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> topSixthMaxTemp(double decisionPoint);

    std::shared_ptr<TempBasedHeatingLogic> largeDraw(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> largerDraw(double decisionPoint);

    static std::string getVersion();
    /**< This function returns a string with the current version number */

    void initResistanceTank(); /**< Default resistance tank, EF 0.95, volume 47.5 */
    void initResistanceTank(double tankVol_L,
                            double energyFactor,
                            double upperPower_W,
                            double lowerPower_W);
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

    void initResistanceTankGeneric(double tankVol_L,
                                   double rValue_M2KperW,
                                   double upperPower_W,
                                   double lowerPower_W);
    /**< This function will initialize a HPWH object to be a generic resistance storage water
     * heater, with a specific R-Value defined at initalization.
     *
     * Several assumptions regarding the tank configuration are assumed: the lower element
     * is at the bottom, the upper element is at the top third. The controls are the same standard
     * controls for the HPWHinit_resTank()
     */

    void initGeneric(double tankVol_L, double energyFactor, double resUse_C);
    /**< This function will initialize a HPWH object to be a non-specific HPWH model
     * with an energy factor as specified.  Since energy
     * factor is not strongly correlated with energy use, most settings
     * are taken from the GE2015_STDMode model.
     */

    static bool mapNameToPreset(const std::string& modelName, MODELS& model);

    void initPreset(MODELS presetNum);
    /**< This function will reset all member variables to defaults and then
     * load in a set of parameters that are hardcoded in this function -
     * which particular set of parameters is selected by presetNum.
     * This is similar to the way the HPWHsim currently operates, as used in SEEM,
     * but not quite as versatile.
     * My impression is that this could be a useful input paradigm for CSE
     */

    void initPreset(const std::string& modelName);

#ifndef HPWH_ABRIDGED
    void initFromFile(std::string modelName);
    /**< Loads a HPWH model from a file
     * The file name is the input - there should be at most one set of parameters per file
     * This is useful for testing new variations, and for the sort of variability
     * that we typically do when creating SEEM runs
     * Appropriate use of this function can be found in the documentation
     */
#endif

    void runOneStep(double drawVolume_L,
                    double ambientT_C,
                    double externalT_C,
                    DRMODES DRstatus,
                    double inletVol2_L = 0.,
                    double inletT2_C = 0.,
                    std::vector<double>* extraHeatDist_W = NULL);
    /**< This function will progress the simulation forward in time by one step
     * all calculated outputs are stored in private variables and accessed through functions
     */

    /** An overloaded function that uses takes inletT_C  */
    void runOneStep(double inletT_C,
                    double drawVolume_L,
                    double ambientT_C,
                    double externalT_C,
                    DRMODES DRstatus,
                    double inletVol2_L = 0.,
                    double inletT2_C = 0.,
                    std::vector<double>* extraHeatDist_W = NULL)
    {
        setInletT(inletT_C);
        runOneStep(drawVolume_L,
                   ambientT_C,
                   externalT_C,
                   DRstatus,
                   inletVol2_L,
                   inletT2_C,
                   extraHeatDist_W);
    };

    void runNSteps(int N,
                   double* inletT_C,
                   double* drawVolume_L,
                   double* tankAmbientT_C,
                   double* heatSourceAmbientT_C,
                   DRMODES* DRstatus);
    /**< This function will progress the simulation forward in time by N (equal) steps
     * The calculated values will be summed or averaged, as appropriate, and
     * then stored in the usual variables to be accessed through functions
     */

    /** Setters for the what are typically input variables  */
    void setInletT(double newInletT_C)
    {
        member_inletT_C = newInletT_C;
        haveInletT = true;
    };
    void setMinutesPerStep(double newMinutesPerStep);

    int WriteCSVHeading(std::ofstream& outFILE,
                        const char* preamble = "",
                        int nTCouples = 6,
                        int options = CSVOPT_NONE) const;
    int WriteCSVRow(std::ofstream& outFILE,
                    const char* preamble = "",
                    int nTCouples = 6,
                    int options = CSVOPT_NONE) const;
    /**< a couple of function to write the outputs to a file
        they both will return 0 for success
        the preamble should be supplied with a trailing comma, as these functions do
        not add one.  Additionally, a newline is written with each call.  */

    /**< Sets the tank node temps based on the provided vector of temps, which are mapped onto the
        existing nodes, regardless of numNodes. */
    void setTankLayerTemperatures(std::vector<double> setTemps, const UNITS units = UNITS_C);

    bool isSetpointFixed() const; /**< is the setpoint allowed to be changed */

    /// attempt to change the setpoint
    void setSetpoint(double newSetpoint, UNITS units = UNITS_C); /**<default units C*/

    double getSetpoint(UNITS units = UNITS_C) const;
    /**< a function to check the setpoint - returns setpoint in celcius  */

    bool isNewSetpointPossible(double newSetpoint_C,
                               double& maxAllowedSetpoint_C,
                               std::string& why,
                               UNITS units = UNITS_C) const;
    /**< This function returns if the new setpoint is physically possible for the compressor. If
       there is no compressor then checks that the new setpoint is less than boiling. The setpoint
       can be set higher than the compressor max outlet temperature if there is a  backup resistance
       element, but the compressor will not operate above this temperature. maxAllowedSetpoint_C
       returns the */

    double getMaxCompressorSetpoint(UNITS units = UNITS_C) const;
    /**< a function to return the max operating temperature of the compressor which can be different
       than the value returned in isNewSetpointPossible() if there are resistance elements. */

    /** Returns State of Charge where
       tMains = current mains (cold) water temp,
       tMinUseful = minimum useful temp,
       tMax = nominal maximum temp.*/
    double calcSoCFraction(double tMains_C, double tMinUseful_C, double tMax_C) const;
    double calcSoCFraction(double tMains_C, double tMinUseful_C) const
    {
        return calcSoCFraction(tMains_C, tMinUseful_C, getSetpoint());
    };
    /** Returns State of Charge calculated from the heating logics if this hpwh uses SoC logics. */
    double getSoCFraction() const;

    double getMinOperatingTemp(UNITS units = UNITS_C) const;
    /**< a function to return the minimum operating temperature of the compressor  */

    void resetTankToSetpoint();
    /**< this function resets the tank temperature profile to be completely at setpoint  */

    void setTankToTemperature(double temp_C);
    /**< helper function for testing */

    void setAirFlowFreedom(double fanFraction);
    /**< This is a simple setter for the AirFlowFreedom */

    void setDoTempDepression(bool doTempDepress);
    /**< This is a simple setter for the temperature depression option */

    void setTankSize_adjustUA(double HPWH_size, UNITS units = UNITS_L, bool forceChange = false);
    /**< This sets the tank size and adjusts the UA the HPWH currently has to have the same U value
       but a new A. A is found via getTankSurfaceArea()*/

    double getTankSurfaceArea(UNITS units = UNITS_FT2) const;
    static double
    getTankSurfaceArea(double vol, UNITS volUnits = UNITS_L, UNITS surfAUnits = UNITS_FT2);
    /**< Returns the tank surface area based off of real storage tanks*/
    double getTankRadius(UNITS units = UNITS_FT) const;
    static double getTankRadius(double vol, UNITS volUnits = UNITS_L, UNITS radiusUnits = UNITS_FT);
    /**< Returns the tank surface radius based off of real storage tanks*/

    bool isTankSizeFixed() const; /**< is the tank size allowed to be changed */
    void setTankSize(double HPWH_size, UNITS units = UNITS_L, bool forceChange = false);
    /**< Defualt units L. This is a simple setter for the tank volume in L or GAL */

    double getTankSize(UNITS units = UNITS_L) const;
    /**< returns the tank volume in L or GAL  */

    void setDoInversionMixing(bool doInversionMixing_in);
    /**< This is a simple setter for the logical for running the inversion mixing method, default is
     * true */

    void setDoConduction(bool doConduction_in);
    /**< This is a simple setter for doing internal conduction and nodal heatloss, default is true*/

    void setUA(double UA, UNITS units = UNITS_kJperHrC);
    /**< This is a setter for the UA, with or without units specified - default is metric, kJperHrC
     */

    void getUA(double& UA, UNITS units = UNITS_kJperHrC) const;
    /**< Returns the UA, with or without units specified - default is metric, kJperHrC  */

    void getFittingsUA(double& UA, UNITS units = UNITS_kJperHrC) const;
    /**< Returns the UAof just the fittings, with or without units specified - default is metric,
     * kJperHrC  */

    void setFittingsUA(double UA, UNITS units = UNITS_kJperHrC);
    /**< This is a setter for the UA of just the fittings, with or without units specified - default
     * is metric, kJperHrC */

    void setInletByFraction(double fractionalHeight);
    /**< This is a setter for the water inlet height which sets it as a fraction of the number of
     * nodes from the bottom up*/
    void setInlet2ByFraction(double fractionalHeight);
    /**< This is a setter for the water inlet height which sets it as a fraction of the number of
     * nodes from the bottom up*/

    void setExternalInletHeightByFraction(double fractionalHeight);
    /**< This is a setter for the height at which the split system HPWH adds heated water to the
    storage tank, this sets it as a fraction of the number of nodes from the bottom up*/
    void setExternalOutletHeightByFraction(double fractionalHeight);
    /**< This is a setter for the height at which the split system HPWH takes cold water out of the
    storage tank, this sets it as a fraction of the number of nodes from the bottom up*/

    void setExternalPortHeightByFraction(double fractionalHeight, int whichPort);
    /**< sets the external heater port heights inlet height node number */

    int getExternalInletHeight() const;
    /**< Returns the node where the split system HPWH adds heated water to the storage tank*/
    int getExternalOutletHeight() const;
    /**< Returns the node where the split system HPWH takes cold water out of the storage tank*/

    void setNodeNumFromFractionalHeight(double fractionalHeight, int& inletNum);
    /**< This is a setter for the water inlet height, by fraction. */

    void setTimerLimitTOT(double limit_min);
    /**< Sets the timer limit in minutes for the DR_TOT call. Must be > 0 minutes and < 1440
     * minutes. */
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

    int getNumHeatSources() const;
    /**< returns the number of heat sources  */

    int getNumResistanceElements() const;
    /**< returns the number of resistance elements  */

    int getCompressorIndex() const;
    /**< returns the index of the compressor in the heat source array.
    Note only supports HPWHs with one compressor, if multiple will return the last index
    of a compressor */

    double getCompressorCapacity(double airTemp = 19.722,
                                 double inletTemp = 14.444,
                                 double outTemp = 57.222,
                                 UNITS pwrUnit = UNITS_KW,
                                 UNITS tempUnit = UNITS_C);
    /**< Returns the heating output capacity of the compressor for the current HPWH model.
    Note only supports HPWHs with one compressor, if multiple will return the last index
    of a compressor. Outlet temperatures greater than the max allowable setpoints will return an
    error, but for compressors with a fixed setpoint the */

    void setCompressorOutputCapacity(double newCapacity,
                                     double airTemp = 19.722,
                                     double inletTemp = 14.444,
                                     double outTemp = 57.222,
                                     UNITS pwrUnit = UNITS_KW,
                                     UNITS tempUnit = UNITS_C);
    /**< Sets the heating output capacity of the compressor at the defined air, inlet water, and
    outlet temperatures. For multi-pass models the capacity is set as the average between the
    inletTemp and outTemp since multi-pass models will increase the water temperature only a few
    degrees at a time (i.e. maybe 10 degF) until the tank reaches the outTemp, the capacity at
    inletTemp might not be accurate for the entire heating cycle.
    Note only supports HPWHs with one compressor, if multiple will return the last index
    of a compressor */

    void setScaleCapacityCOP(double scaleCapacity = 1., double scaleCOP = 1.);
    /**< Scales the input capacity and COP*/

    void setResistanceCapacity(double power, int which = -1, UNITS pwrUNIT = UNITS_KW);
    /**< Scale the resistance elements in the heat source list. Which heat source is chosen is
    changes is given by "which"
    - If which (-1) sets all the resisistance elements in the tank.
    - If which (0, 1, 2...) sets the resistance element in a low to high order.
      So if there are 3 elements 0 is the bottom, 1 is the middle, and 2 is the top element,
    regardless of their order in heatSources. If the elements exist on at the same node then all of
    the elements are set.

    The only valid values for which are between -1 and getNumResistanceElements()-1. Since which is
    defined as the by the ordered height of the resistance elements it cannot refer to a compressor.
    */

    double getResistanceCapacity(int which = -1, UNITS pwrUNIT = UNITS_KW);
    /**< Returns the resistance elements capacity. Which heat source is chosen is changes is given
    by "which"
    - If which (-1) gets all the resisistance elements in the tank.
    - If which (0, 1, 2...) sets the resistance element in a low to high order.
      So if there are 3 elements 0 is the bottom, 1 is the middle, and 2 is the top element,
    regardless of their order in heatSources. If the elements exist on at the same node then all of
    the elements are set.

    The only valid values for which are between -1 and getNumResistanceElements()-1. Since which is
    defined as the by the ordered height of the resistance elements it cannot refer to a compressor.
    */

    int getResistancePosition(int elementIndex) const;

    double getNthHeatSourceEnergyInput(int N, UNITS units = UNITS_KWH) const;
    /**< returns the energy input to the Nth heat source, with the specified units
      energy used by the heat source is positive - should always be positive */

    double getNthHeatSourceEnergyOutput(int N, UNITS units = UNITS_KWH) const;
    /**< returns the energy output from the Nth heat source, with the specified units
      energy put into the water is positive - should always be positive  */

    double getNthHeatSourceRunTime(int N) const;
    /**< returns the run time for the Nth heat source, in minutes
      note: they may sum to more than 1 time step for concurrently running heat sources  */
    int isNthHeatSourceRunning(int N) const;
    /**< returns 1 if the Nth heat source is currently engaged, 0 if it is not  */
    HEATSOURCE_TYPE getNthHeatSourceType(int N) const;
    /**< returns the enum value for what type of heat source the Nth heat source is  */

    class HeatSource;

    /// get a pointer to the Nth heat source
    bool getNthHeatSource(int N, HPWH::HeatSource*& heatSource);

    double getExternalVolumeHeated(UNITS units = UNITS_L) const;
    /**< returns the volume of water heated in an external in the specified units
      returns 0 when no external heat source is running  */

    double getEnergyRemovedFromEnvironment(UNITS units = UNITS_KWH) const;
    /**< get the total energy removed from the environment by all heat sources in specified units
      (not net energy - does not include standby)
      moving heat from the space to the water is the positive direction */

    double getStandbyLosses(UNITS units = UNITS_KWH) const;
    /**< get the amount of heat lost through the tank in specified units
      moving heat from the water to the space is the positive direction
      negative should occur seldom */

    double getTankHeatContent_kJ() const;
    /**< get the heat content of the tank, relative to zero celsius
     * returns using kilojoules */

    int getModel() const;
    /**< get the model number */

    int getCompressorCoilConfig() const;

    /// returns 1 if compressor is multipass, 0 if compressor is not multipass, ABORT if no
    /// compressor
    int isCompressorMultipass() const;

    /// returns 1 if compressor is external multipass, 0 if compressor is not external multipass
    int isCompressorExternalMultipass() const;

    bool hasACompressor() const;
    /// Returns if the HPWH model has a compressor or not, could be a storage or resistance tank.

    /// returns 1 if compressor running, 0 if compressor not running, ABORT if no compressor
    int isCompressorRunning() const;

    bool hasExternalHeatSource(int& heatSourceIndex) const;
    /**< Returns if the HPWH model has any external heat sources or not, could be a compressor or
     * resistance element. */
    double getExternalMPFlowRate(UNITS units = UNITS_GPM) const;
    /**< Returns the constant flow rate for an external multipass heat sources. */

    double getCompressorMinRuntime(UNITS units = UNITS_MIN) const;

    void getSizingFractions(double& aquafract, double& percentUseable) const;
    /**< returns the fraction of total tank volume from the bottom up where the aquastat is
    or the turn on logic for the compressor, and the USEable fraction of storage or 1 minus
    where the shut off logic is for the compressor. If the logic spans multiple nodes it
    returns the weighted average of the nodes */

    bool isHPWHScalable() const;
    /**< returns if the HPWH is scalable or not*/

    bool shouldDRLockOut(HEATSOURCE_TYPE hs, DRMODES DR_signal) const;
    /**< Checks the demand response signal against the different heat source types  */

    void resetTopOffTimer();
    /**< resets variables for timer associated with the DR_TOT call  */

    double getLocationTemp_C() const;

    void getTankTemps(std::vector<double>& tankTemps);

    double getOutletTemp(UNITS units = UNITS_C) const;
    /**< returns the outlet temperature in the specified units
      returns 0 when no draw occurs */

    double getCondenserWaterInletTemp(UNITS units = UNITS_C) const;
    /**< returns the condenser inlet temperature in the specified units
    returns 0 when no HP not running occurs,  */

    double getCondenserWaterOutletTemp(UNITS units = UNITS_C) const;
    /**< returns the condenser outlet temperature in the specified units
    returns 0 when no HP not running occurs */

    double getTankNodeTemp(int nodeNum, UNITS units = UNITS_C) const;
    /**< returns the temperature of the water at the specified node - with specified units */

    double getNthSimTcouple(int iTCouple, int nTCouple, UNITS units = UNITS_C) const;
    /**< returns the temperature from a set number of virtual "thermocouples" specified by nTCouple,
        which are constructed from the node temperature array.  Specify iTCouple from 1-nTCouple,
        1 at the bottom using specified units */

    /// returns the tank temperature averaged uniformly
    double getAverageTankTemp_C() const;

    /// returns the tank temperature averaged over a distribution
    double getAverageTankTemp_C(const std::vector<double>& dist) const;

    /// returns the tank temperature averaged using weighted logic nodes
    double getAverageTankTemp_C(const std::vector<NodeWeight>& nodeWeights) const;

    void setMaxTempDepression(double maxDepression, UNITS units = UNITS_C);

    bool hasEnteringWaterHighTempShutOff(int heatSourceIndex);
    void setEnteringWaterHighTempShutOff(double highTemp,
                                         bool tempIsAbsolute,
                                         int heatSourceIndex,
                                         UNITS units = UNITS_C);
    /**< functions to check for and set specific high temperature shut off logics.
    HPWHs can only have one of these, which is at least typical */

    void setTargetSoCFraction(double target);

    bool canUseSoCControls();

    void switchToSoCControls(double targetSoC,
                             double hysteresisFraction = 0.05,
                             double tempMinUseful = 43.333,
                             bool constantMainsT = false,
                             double mainsT = 18.333,
                             UNITS tempUnit = UNITS_C);

    bool isSoCControlled() const;

    /// Checks whether energy is balanced during a simulation step.
    bool isEnergyBalanced(const double drawVol_L,
                          const double prevHeatContent_kJ,
                          const double fracEnergyTolerance = 0.001);

    /// Overloaded version of above that allows specification of inlet temperature.
    bool isEnergyBalanced(const double drawVol_L,
                          double inletT_C_in,
                          const double prevHeatContent_kJ,
                          const double fracEnergyTolerance)
    {
        setInletT(inletT_C_in);
        return isEnergyBalanced(drawVol_L, prevHeatContent_kJ, fracEnergyTolerance);
    }

    /// Addition of heat from a normal heat sources; return excess heat, if needed, to prevent
    /// exceeding maximum or setpoint
    double addHeatAboveNode(double qAdd_kJ, const int nodeNum, const double maxT_C);

    /// Addition of extra heat handled separately from normal heat sources
    void addExtraHeatAboveNode(double qAdd_kJ, const int nodeNum);

    /// first-hour rating designations to determine draw pattern for 24-hr test
    struct FirstHourRating
    {
        enum class Desig
        {
            VerySmall,
            Low,
            Medium,
            High
        } desig;

        static inline std::unordered_map<Desig, std::string> sDesigMap = {
            {Desig::VerySmall, "Very Small"},
            {Desig::Low, "Low"},
            {Desig::Medium, "Medium"},
            {Desig::High, "High"}};

        double drawVolume_L;
    };

    /// collection of information derived from standard test
    struct StandardTestSummary
    {
        // first recovery values
        double recoveryEfficiency = 0.; // eta_r
        double recoveryDeliveredEnergy_kJ = 0.;
        double recoveryStoredEnergy_kJ = 0.;
        double recoveryUsedEnergy_kJ = 0.; // Q_r

        //
        double standbyPeriodTime_h = 0; // tau_stby,1

        double standbyStartTankT_C = 0.; // T_su,0
        double standbyEndTankT_C = 0.;   // T_su,f

        double standbyStartEnergy_kJ = 0.; // Q_su,0
        double standbyEndEnergy_kJ = 0.;   // Q_su,f
        double standbyUsedEnergy_kJ = 0.;  // Q_stby

        double standbyHourlyLossEnergy_kJperh = 0.; // Q_hr
        double standbyLossCoefficient_kJperhC = 0.; // UA

        double noDrawTotalTime_h = 0;        // tau_stby,2
        double noDrawAverageAmbientT_C = 0.; // <T_a,stby,2>

        // 24-hr values
        double removedVolume_L = 0.;
        double waterHeatingEnergy_kJ = 0.; // Q_HW
        double avgOutletT_C = 0.;          // <Tdel,i>
        double avgInletT_C = 0.;           // <Tin,i>

        double usedFossilFuelEnergy_kJ = 0.;               // Q_f
        double usedElectricalEnergy_kJ = 0.;               // Q_e
        double usedEnergy_kJ = 0.;                         // Q
        double consumedHeatingEnergy_kJ = 0.;              // Q_d
        double standardWaterHeatingEnergy_kJ = 0.;         // Q_HW,T
        double adjustedConsumedWaterHeatingEnergy_kJ = 0.; // Q_da
        double modifiedConsumedWaterHeatingEnergy_kJ = 0.; // Q_dm
        double UEF = 0.;

        // (calculated) annual totals
        double annualConsumedElectricalEnergy_kJ = 0.; // E_annual,e
        double annualConsumedEnergy_kJ = 0.;           // E_annual

        bool qualifies = false;
    };

    struct StandardTestOptions
    {
        bool saveOutput = false;
        std::string sOutputDirectory = "";
        std::string sOutputFilename = "";
        std::ostream* outputStream = NULL;
        bool changeSetpoint = false;
        std::ofstream outputFile;
        int nTestTCouples = 6;
        double setpointT_C = 51.7;
    };

    /// perform a draw/heat cycle to prepare for test
    void prepForTest(StandardTestOptions& standardTestOptions);

    /// determine first-hour rating
    void findFirstHourRating(FirstHourRating& firstHourRating,
                             StandardTestOptions& standardTestOptions);

    /// run 24-hr draw pattern and compute metrics
    void run24hrTest(const FirstHourRating firstHourRating,
                     StandardTestSummary& standardTestSummary,
                     StandardTestOptions& standardTestOptions);

    /// specific information for a single draw
    struct Draw
    {
        double startTime_min;
        double volume_L;
        double flowRate_Lper_min;

        Draw(const double startTime_min_in,
             const double volume_L_in,
             const double flowRate_Lper_min_in)
            : startTime_min(startTime_min_in)
            , volume_L(volume_L_in)
            , flowRate_Lper_min(flowRate_Lper_min_in)
        {
        }
    };

    /// sequence of draws in pattern
    typedef std::vector<Draw> DrawPattern;

    static std::unordered_map<FirstHourRating::Desig, std::size_t> firstDrawClusterSizes;

    /// collection of standard draw patterns
    static std::unordered_map<FirstHourRating::Desig, DrawPattern> drawPatterns;

    /// fields for test output to csv
    struct OutputData
    {
        int time_min;
        double ambientT_C;
        double setpointT_C;
        double inletT_C;
        double drawVolume_L;
        DRMODES drMode;
        std::vector<double> h_srcIn_kWh;
        std::vector<double> h_srcOut_kWh;
        std::vector<double> thermocoupleT_C;
        double outletT_C;
    };

    int writeRowAsCSV(std::ofstream& outFILE,
                      OutputData& outputData,
                      const CSVOPTIONS& options = CSVOPTIONS::CSVOPT_NONE) const;

    void measureMetrics(FirstHourRating& firstHourRating,
                        StandardTestOptions& standardTestOptions,
                        StandardTestSummary& standardTestSummary);

    struct CustomTestOptions
    {
        bool overrideFirstHourRating = false;
        FirstHourRating::Desig desig = FirstHourRating::Desig::VerySmall;

        bool overrideAmbientT = false;
        double ambientT_C = 19.7;

    } customTestOptions;

    struct Fitter;
    struct FitOptions;
    void makeGeneric(const FitOptions& genericOptions, StandardTestOptions& standardTestOptions);
    void makeGenericUEF(double targetEF, double ambientT_C = 19.7);
    void makeGenericE50(double targetEF) { return makeGenericUEF(targetEF, 10.); }
    void makeGenericE95(double targetEF) { return makeGenericUEF(targetEF, 35.); }
    void makeGenericE50_UEF_E95(double targetEF50, double targetUEF, double targetEF95)
    {
        makeGenericE50(targetEF50);
        makeGenericUEF(targetUEF);
        makeGenericE95(targetEF95);
    }

  private:
    void setAllDefaults(); /**< sets all the defaults default */

    void updateTankTemps(
        double draw, double inletT_C, double ambientT_C, double inletVol2_L, double inletT2_L);
    void mixTankInversions();
    /**< Mixes the any temperature inversions in the tank after all the temperature calculations  */
    void updateSoCIfNecessary();

    bool areAllHeatSourcesOff() const;
    /**< test if all the heat sources are off  */
    void turnAllHeatSourcesOff();
    /**< disengage each heat source  */

    void addHeatParent(HeatSource* heatSourcePtr, double heatSourceAmbientT_C, double minutesToRun);

    /// adds extra heat to the set of nodes that are at the same temperature, above the
    ///	specified node number
    void modifyHeatDistribution(std::vector<double>& heatDistribution);
    void addExtraHeat(std::vector<double>& extraHeatDist_W);

    ///  "extra" heat added during a simulation step
    double extraEnergyInput_kWh;

    /// shift temperatures of tank nodes with indices in the range [mixBottomNode, mixBelowNode)
    /// by a factor mixFactor towards their average temperature
    void mixTankNodes(int mixBottomNode, int mixBelowNode, double mixFactor);

    void calcDerivedValues();
    /**< a helper function for the inits, calculating condentropy and the lowest node  */
    void calcSizeConstants();
    /**< a helper function to set constants for the UA and tank size*/
    void calcDerivedHeatingValues();
    /**< a helper for the helper, calculating condentropy and the lowest node*/
    void mapResRelativePosToHeatSources();
    /**< a helper function for the inits, creating a mapping function for the position of the
    resistance elements to their indexes in heatSources. */

    void checkInputs();
    /**< a helper function to run a few checks on the HPWH input parameters  */

    double getChargePerNode(double tCold, double tMix, double tHot) const;

    void calcAndSetSoCFraction();

    bool isHeating;
    /**< is the hpwh currently heating or not?  */

    bool setpointFixed;
    /**< does the HPWH allow the setpoint to vary  */

    bool tankSizeFixed;
    /**< does the HPWH have a constant tank size or can it be changed  */

    bool canScale;
    /**< can the HPWH scale capactiy and COP or not  */

    MODELS model;
    /**< The model id */

    /// a std::vector containing the HeatSources, in order of priority
    std::vector<HeatSource> heatSources;

    HeatSource* addHeatSource(const std::string& name_in);

    int compressorIndex;
    /**< The index of the compressor heat source (set to -1 if no compressor)*/

    int lowestElementIndex;
    /**< The index of the lowest resistance element heat source (set to -1 if no resistance
     * elements)*/

    int highestElementIndex;
    /**< The index of the highest resistance element heat source. if only one element it equals
     * lowestElementIndex (set to -1 if no resistance elements)*/

    int VIPIndex;
    /**< The index of the VIP resistance element heat source (set to -1 if no VIP resistance
     * elements)*/

    int inletHeight;
    /**< the number of a node in the tank that the inlet water enters the tank at, must be between 0
     * and numNodes-1  */

    int inlet2Height;
    /**< the number of a node in the tank that the 2nd inlet water enters the tank at, must be
     * between 0 and numNodes-1  */

    /**< the volume in liters of the tank  */
    double tankVolume_L;

    /**< the UA of the tank, in metric units  */
    double tankUA_kJperHrC;

    /**< the UA of the fittings for the tank, in metric units  */
    double fittingsUA_kJperHrC;

    /**< the volume (L) of a single node  */
    double nodeVolume_L;

    /**< heat capacity (kJ/degC) of the fluid (water, except for heat-exchange models) in a single
     * node  */
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

    /**< holds the future temperature of each node for the conduction calculation - 0 is the bottom
     * node  */
    std::vector<double> nextTankTemps_C;

    DRMODES prevDRstatus;
    /**< the DRstatus of the tank in the previous time step and at the end of runOneStep */

    double timerLimitTOT;
    /**< the time limit in minutes on the timer when the compressor and resistance elements are
     * turned back on, used with DR_TOT. */
    double timerTOT;
    /**< the timer used for DR_TOT to turn on the compressor and resistance elements. */

    bool usesSoCLogic;

    // Some outputs
    double outletTemp_C;
    /**< the temperature of the outlet water - taken from top of tank, 0 if no flow  */

    double condenserInlet_C;
    /**< the temperature of the inlet water to the condensor either an average of tank nodes or
     * taken from the bottom, 0 if no flow or no compressor  */
    double condenserOutlet_C;
    /**< the temperature of the outlet water from the condensor either, 0 if no flow or no
     * compressor  */
    double externalVolumeHeated_L;
    /**< the volume of water heated by an external source, 0 if no flow or no external heat source
     */

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
    bool haveInletT; /// needed for SoC-based heating logic

    double minutesPerStep = 1.;
    double secondsPerStep, hoursPerStep;

    bool doInversionMixing;
    /**<  If and only if true will model temperature inversion mixing in the tank  */

    bool doConduction;
    /**<  If and only if true will model conduction between the internal nodes of the tank  */

    struct resPoint
    {
        int index;
        int position;
    };
    std::vector<resPoint> resistanceHeightMap;
    /**< A map from index of an resistance element in heatSources to position in the tank, its
    is sorted by height from lowest to highest*/

    /// Generates a vector of logical nodes
    std::vector<HPWH::NodeWeight> getNodeWeightRange(double bottomFraction, double topFraction);

    /// False: water is drawn from the tank itself; True: tank provides heat exchange only
    bool hasHeatExchanger;

    /// Coefficient (0-1) of effectiveness for heat exchange between tank and water line (used by
    /// heat-exchange models only).
    double heatExchangerEffectiveness;

    /// Coefficient (0-1) of effectiveness for heat exchange between a single tank node and water
    /// line (derived from heatExchangerEffectiveness).
    double nodeHeatExchangerEffectiveness;

}; // end of HPWH class

class HPWH::HeatSource : public Courier::Sender
{
  public:
    friend class HPWH;

    HeatSource() {} /**< default constructor, does not create a useful HeatSource */
    HeatSource(
        const std::string& name_in,
        HPWH* hpwh_in,
        const std::shared_ptr<Courier::Courier> courier = std::make_shared<DefaultCourier>());
    /**< constructor assigns a pointer to the hpwh that owns this heat source  */
    HeatSource(const HeatSource& hSource); /// copy constructor

    ~HeatSource() {}

    HeatSource& operator=(const HeatSource& hSource); /// assignment operator
    /**< the copy constructor and assignment operator basically just checks if there
        are backup/companion pointers - these can't be copied */

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

    int findParent() const;
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

    struct perfPoint
    {
        double T_F;
        std::vector<double> inputPower_coeffs; // c0 + c1*T + c2*T*T
        std::vector<double> COP_coeffs;        // c0 + c1*T + c2*T*T
    };

    std::vector<perfPoint> perfMap;
    /**< A map with input/COP quadratic curve coefficients at a given external temperature */

  private:
    // start with a few type definitions
    enum COIL_CONFIG
    {
        CONFIG_SUBMERGED,
        CONFIG_WRAPPED,
        CONFIG_EXTERNAL
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
    std::shared_ptr<TempBasedHeatingLogic> standbyLogic;

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

constexpr double BTUperKWH =
    3412.14163312794;               // https://www.rapidtables.com/convert/energy/kWh_to_BTU.html
constexpr double FperC = 9. / 5.;   // degF / degC
constexpr double offsetF = 32.;     // degF offset
constexpr double sec_per_min = 60.; // s / min
constexpr double min_per_hr = 60.;  // min / hr
constexpr double sec_per_hr = sec_per_min * min_per_hr; // s / hr
constexpr double L_per_gal = 3.78541;                   // liters / gal
constexpr double ft_per_m = 3.2808;                     // ft / m
constexpr double ft2_per_m2 = ft_per_m * ft_per_m;      // ft^2 / m^2
constexpr double BTUm2C_per_kWhft2F =
    BTUperKWH / ft2_per_m2 / FperC; // BTU m^2 degC / kW h ft^2 degC

// a few extra functions for unit conversion
inline double dF_TO_dC(double temperature) { return (temperature / FperC); }
inline double F_TO_C(double temperature) { return ((temperature - offsetF) / FperC); }
inline double C_TO_F(double temperature) { return ((FperC * temperature) + offsetF); }
inline double KWH_TO_BTU(double kwh) { return (BTUperKWH * kwh); }
inline double KWH_TO_KJ(double kwh) { return (kwh * sec_per_hr); }
inline double BTU_TO_KWH(double btu) { return (btu / BTUperKWH); }
inline double BTUperH_TO_KW(double btu) { return (btu / BTUperKWH); }
inline double KW_TO_BTUperH(double kw) { return (kw * BTUperKWH); }
inline double W_TO_BTUperH(double w) { return (w * BTUperKWH / 1000.); }
inline double KJ_TO_KWH(double kj) { return (kj / sec_per_hr); }
inline double BTU_TO_KJ(double btu) { return (btu * sec_per_hr / BTUperKWH); }
inline double GAL_TO_L(double gallons) { return (gallons * L_per_gal); }
inline double L_TO_GAL(double liters) { return (liters / L_per_gal); }
inline double L_TO_FT3(double liters) { return (liters / 28.31685); }
inline double UAf_TO_UAc(double UAf) { return (UAf * 1.8 / 0.9478); }
inline double GPM_TO_LPS(double gpm) { return (gpm * L_per_gal / sec_per_min); }
inline double LPS_TO_GPM(double lps) { return (lps * sec_per_min / L_per_gal); }

inline double FT_TO_M(double feet) { return (feet / ft_per_m); }
inline double FT2_TO_M2(double feet2) { return (feet2 / ft2_per_m2); }

inline double MIN_TO_SEC(double minute) { return minute * sec_per_min; }
inline double MIN_TO_HR(double minute) { return minute / min_per_hr; }

inline double HM_TO_MIN(const double hours, const double minutes)
{
    return min_per_hr * hours + minutes;
}

inline HPWH::DRMODES operator|(HPWH::DRMODES a, HPWH::DRMODES b)
{
    return static_cast<HPWH::DRMODES>(static_cast<int>(a) | static_cast<int>(b));
}

template <typename T>
inline bool aboutEqual(T a, T b)
{
    return fabs(a - b) < HPWH::TOL_MINVALUE;
}

/// Generate an absolute or relative temperature in degC.
inline double convertTempToC(const double T_F_or_C, const HPWH::UNITS units, const bool absolute)
{
    return (units == HPWH::UNITS_C) ? T_F_or_C : (absolute ? F_TO_C(T_F_or_C) : dF_TO_dC(T_F_or_C));
}

// resampling utility functions
double
getResampledValue(const std::vector<double>& values, double beginFraction, double endFraction);
bool resample(std::vector<double>& values, const std::vector<double>& sampleValues);
inline bool resampleIntensive(std::vector<double>& values, const std::vector<double>& sampleValues)
{
    return resample(values, sampleValues);
}
bool resampleExtensive(std::vector<double>& values, const std::vector<double>& sampleValues);

///  helper functions
double expitFunc(double x, double offset);
void normalize(std::vector<double>& distribution);
int findLowestNode(const std::vector<double>& nodeDist, const int numTankNodes);
double findShrinkageT_C(const std::vector<double>& nodeDist);
void calcThermalDist(std::vector<double>& thermalDist,
                     const double shrinkageT_C,
                     const int lowestNode,
                     const std::vector<double>& nodeTemp_C,
                     const double setpointT_C);
void scaleVector(std::vector<double>& coeffs, const double scaleFactor);

#endif
