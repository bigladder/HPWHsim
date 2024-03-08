#ifndef HPWH_hh
#define HPWH_hh

#include <string>
#include <sstream>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <functional>
#include <memory>

#include <cstdio>
#include <cstdlib> //for exit
#include <vector>
#include <unordered_map>

namespace Btwxt
{
class RegularGridInterpolator;
}

// #define HPWH_ABRIDGED
/**<  Definition of HPWH_ABRIDGED excludes some functions to reduce the size of the
 * compiled library.  */

#include "HPWHversion.hh"

class HPWH
{
  public:
    static const int version_major = HPWHVRSN_MAJOR;
    static const int version_minor = HPWHVRSN_MINOR;
    static const int version_patch = HPWHVRSN_PATCH;
    static const std::string version_maint;

    /// number of condensity nodes associated with each heat source
    static const int CONDENSITY_SIZE = 12;

    /// number of logic nodes associated with temperature-based heating logic
    static const int LOGIC_SIZE = 12;

    /// maximum length for a debuging output string
    static const int MAXOUTSTRING = 200;

    static const double DENSITYWATER_kgperL; /// mass density of water
    static const double KWATER_WpermC;       /// thermal conductivity of water
    static const double CPWATER_kJperkgC;    /// specific heat capcity of water
    static const double TOL_MINVALUE;        /// tolerance

    /// tells the simulation when the location temperature has not been initialized
    static const float UNINITIALIZED_LOCATIONTEMP;

    static const float
        ASPECTRATIO; /**< A constant to define the aspect ratio between the tank height and
                     radius (H/R). Used to find the radius and tank height from the volume and then
                     find the surface area. It is derived from the median value of 88
                     insulated storage tanks currently available on the market from
                     Sanden, AOSmith, HTP, Rheem, and Niles,  */

    /// max oulet temperature for compressors with the refrigerant R134a
    static const double MAXOUTLET_R134A;

    /// max oulet temperature for compressors with the refrigerant R410a
    static const double MAXOUTLET_R410A;

    /// max oulet temperature for compressors with the refrigerant R744*/
    static const double MAXOUTLET_R744;

    /// minimum temperature lift for single-pass compressors
    static const double MINSINGLEPASSLIFT;

    HPWH();                            /// default constructor
    HPWH(const HPWH& hpwh);            /// copy constructor
    HPWH& operator=(const HPWH& hpwh); /// assignment operator
    ~HPWH();                           /// destructor

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

    /// unit-conversion utilities
    struct Units
    {
        struct PairHash
        {
            template <class T>
            std::size_t operator()(const std::pair<T, T>& p) const
            {
                auto h1 = static_cast<std::size_t>(p.first);
                auto h2 = static_cast<std::size_t>(p.second);
                return h1 ^ h2;
            }
        };

        template <typename T>
        using ConversionMap =
            std::unordered_map<std::pair<T, T>, std::function<double(const double)>, PairHash>;

        template <typename T>
        static ConversionMap<T> conversionMap;

        template <typename T>
        static double convert(const double x, const T fromUnits, const T toUnits)
        {
            return conversionMap<T>[{fromUnits, toUnits}](x);
        }

        template <typename T>
        static double invert(const double x, const T fromUnits, const T toUnits)
        {
            return conversionMap<T>[{toUnits, fromUnits}](x);
        }

        template <typename T>
        static std::vector<double>
        convert(const std::vector<double> xV, const T fromUnits, const T toUnits)
        {
            auto conversionFnc = conversionMap<T>[{fromUnits, toUnits}];
            std::vector<double> xV_out;
            for (auto& x : xV)
            {
                double y = conversionFnc(x);
                xV_out.push_back(y);
            }
            return xV_out;
        }

        template <typename T>
        static std::vector<double>
        invert(const std::vector<double> xV, const T fromUnits, const T toUnits)
        {
            auto conversionFnc = conversionMap<T>[{toUnits, fromUnits}];
            std::vector<double> xV_out;
            for (auto& x : xV)
            {
                double y = conversionFnc(x);
                xV_out.push_back(y);
            }
            return xV_out;
        }

        /* time units */
        enum class Time
        {
            h,   // hours
            min, // minutes
            s    // seconds
        };

        /* temperature units */
        enum class Temp
        {
            C, // celsius
            F  // fahrenheit
        };

        /* temperature-difference units */
        enum class TempDiff
        {
            C, // celsius
            F  // fahrenheit
        };

        /* energy units */
        enum class Energy
        {
            kJ,  // kilojoules
            kWh, // kilowatt hours
            Btu  // british thermal units
        };

        /* power units */
        enum class Power
        {
            kW,        // kilowatts
            Btu_per_h, // BTU per hour
            W,         // watts
            kJ_per_h,  // kilojoules per hour
        };

        /* length units */
        enum class Length
        {
            m, // meters
            ft // feet
        };

        /* area units */
        enum class Area
        {
            m2, // square meters
            ft2 // square feet
        };

        /* volume units */
        enum class Volume
        {
            L,   // liters
            gal, // gallons
            ft3  // cubic feet
        };

        /* flow-rate units */
        enum class FlowRate
        {
            L_per_s,    // liters per second
            gal_per_min // gallons per minute
        };

        /* UA units */
        enum class UA
        {
            kJ_per_hC, // kilojoules per hour degree celsius
            Btu_per_hF // british thermal units per hour degree fahrenheit
        };
    };

    /// specifies the type of heat source
    enum HEATSOURCE_TYPE
    {
        TYPE_none,       /**< a default to check to make sure it's been set  */
        TYPE_resistance, /**< a resistance element  */
        TYPE_compressor  /**< a vapor cycle compressor  */
    };

    /// specifies the extrapolation method based on Tair, from the perfmap for a heat source
    enum EXTRAP_METHOD
    {
        EXTRAP_LINEAR, /**< the default extrapolates linearly */
        EXTRAP_NEAREST /**< extrapolates using nearest neighbor, will just continue from closest
                          point  */
    };

    /// specifies the unit type for CSV file outputs
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

        /// check that the input is valid.
        virtual bool isValid() = 0;

        /// get the value for comparing the tank value to, i.e. the target SoC
        virtual double getComparisonValue() = 0;

        /// get the calculated value from the tank, i.e. SoC or tank average of node weights
        virtual double getTankValue() = 0;

        /// calculate where the average node for a logic set is.
        virtual double nodeWeightAvgFract() = 0;

        /// gets the fraction of a node that has to be heated up to met the turnoff condition
        virtual double getFractToMeetComparisonExternal() = 0;

        virtual int setDecisionPoint(double value) = 0;
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
        int setDecisionPoint(double value);
        int setConstantMainsTemperature(double mains_C);

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

        int setDecisionPoint(double value);
        int setDecisionPoint(double value, bool absolute);

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

    std::shared_ptr<TempBasedHeatingLogic> wholeTank(double decisionPoint,
                                                     const bool absolute = false);
    std::shared_ptr<TempBasedHeatingLogic> topThird(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> topThird_absolute(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> secondThird(double decisionPoint,
                                                       const bool absolute = false);
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

    /// return value to indicate a simulation-destroying error
    static const int HPWH_ABORT;

    /// returns a string with the current version number
    static std::string getVersion();

    /// get the model enum
    int getModel() const { return model; }

    int initResistanceTank(); /**< Default resistance tank, EF 0.95, volume 47.5 */
    int initResistanceTank(const double tankVol,
                           const double energyFactor,
                           const double upperPower,
                           const double lowerPower,
                           const Units::Volume unitsVolume = Units::Volume::L,
                           const Units::Power unitsPower = Units::Power::kW);
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

    int initResistanceTankGeneric(double tankVol,
                                  double rValue,
                                  double upperPower,
                                  double lowerPower,
                                  const Units::Volume unitsVolume = Units::Volume::L,
                                  const Units::Area unitsArea = Units::Area::m2,
                                  const Units::Temp unitsTemp = Units::Temp::C,
                                  const Units::Power unitsPower = Units::Power::kW);
    /**< This function will initialize a HPWH object to be a generic resistance storage water
     * heater, with a specific R-Value defined at initalization.
     *
     * Several assumptions regarding the tank configuration are assumed: the lower element
     * is at the bottom, the upper element is at the top third. The controls are the same standard
     * controls for the HPWHinit_resTank()
     */

    int initGeneric(double tankVol,
                    double energyFactor,
                    double resUseT,
                    const Units::Volume unitsVolume = Units::Volume::L,
                    const Units::Temp unitsTemp = Units::Temp::C);
    /**< This function will initialize a HPWH object to be a non-specific HPWH model
     * with an energy factor as specified.  Since energy
     * factor is not strongly correlated with energy use, most settings
     * are taken from the GE2015_STDMode model.
     */

    static bool mapNameToPreset(const std::string& modelName, MODELS& model);

    int initPreset(MODELS presetNum);
    /**< This function will reset all member variables to defaults and then
     * load in a set of parameters that are hardcoded in this function -
     * which particular set of parameters is selected by presetNum.
     * This is similar to the way the HPWHsim currently operates, as used in SEEM,
     * but not quite as versatile.
     * My impression is that this could be a useful input paradigm for CSE
     *
     * The return value is 0 for successful initialization, HPWH_ABORT otherwise
     */

    int initPreset(const std::string& modelName);

#ifndef HPWH_ABRIDGED
    int initFromFile(std::string configFile);
    /**< Loads a HPWH model from a file
     * The file name is the input - there should be at most one set of parameters per file
     * This is useful for testing new variations, and for the sort of variability
     * that we typically do when creating SEEM runs
     * Appropriate use of this function can be found in the documentation
     * The return value is 0 for successful initialization, HPWH_ABORT otherwise
     */
#endif

    int runOneStep(double drawVolume_L,
                   double ambientT_C,
                   double externalT_C,
                   DRMODES DRstatus,
                   double inletVol2_L = 0.,
                   double inletT2_C = 0.,
                   std::vector<double>* extraHeatDist_W = NULL);
    /**< This function will progress the simulation forward in time by one step
     * all calculated outputs are stored in private variables and accessed through functions
     *
     * The return value is 0 for successful simulation run, HPWH_ABORT otherwise
     */

    /** An overloaded function that takes inletT_C  */
    int runOneStep(double inletT_C_in,
                   double drawVolume_L,
                   double ambientT_C,
                   double externalT_C,
                   DRMODES DRstatus,
                   double inletVol2_L = 0.,
                   double inletT2_C = 0.,
                   std::vector<double>* extraHeatDist_W = NULL)
    {
        setInletT_C(inletT_C_in);
        return runOneStep(drawVolume_L,
                          ambientT_C,
                          externalT_C,
                          DRstatus,
                          inletVol2_L,
                          inletT2_C,
                          extraHeatDist_W);
    };

    int runNSteps(int N,
                  double* inletT_C,
                  double* drawVolume_L,
                  double* tankAmbientT_C,
                  double* heatSourceAmbientT_C,
                  DRMODES* DRstatus);
    /**< This function will progress the simulation forward in time by N (equal) steps
     * The calculated values will be summed or averaged, as appropriate, and
     * then stored in the usual variables to be accessed through functions
     *
     * The return value is 0 for successful simulation run, HPWH_ABORT otherwise
     */

    /** Setters for the what are typically input variables  */
    void setMinutesPerStep(double newMinutesPerStep);

    /// sets the verbosity to the specified level
    void setVerbosity(VERBOSITY hpwhVrb);

    /// sets the function to be used for message passing
    void setMessageCallback(void (*callbackFunc)(const std::string message, void* pContext),
                            void* pContext);

    void printHeatSourceInfo();
    /**< this prints out the heat source info, nicely formatted
        specifically input/output energy/power, and runtime
        will print to cout if messageCallback pointer is unspecified
        does not use verbosity, as it is public and expected to be called only when needed  */

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

    ///////////////////////////////////////////////
    /* The following functions return energies (kJ) */

    /// total input energy supplied to all heat sources in the previous time step
    double getInputEnergy_kJ() const;

    /// total energy transferred from all heat sources to the tank in the previous time step
    double getOutputEnergy_kJ() const;

    /// total energy removed from the environment by all heat source in the previous time step
    double getEnergyRemovedFromEnvironment_kJ() const;

    /// energy released from the tank to the environment in the previous time step
    double getStandbyLosses_kJ() const { return standbyLosses_kJ; }

    /// heat content of the tank (relative to 0 degC)
    double getTankHeatContent_kJ() const;

    /// total heat content of the tank (relative to 0 degC), other terms can be incorporated
    double getHeatContent_kJ() const;

    ///////////////////////////////////////////////
    /* The following functions return energies in user-specified units */

    double getInputEnergy(const Units::Energy units = Units::Energy::kWh) const;

    double getOutputEnergy(const Units::Energy units = Units::Energy::kWh) const;

    double getTankHeatContent(const Units::Energy units = Units::Energy::kWh) const;

    double getHeatContent(const Units::Energy units = Units::Energy::kWh) const;

    double getEnergyRemovedFromEnvironment(const Units::Energy units = Units::Energy::kWh) const;

    double getStandbyLosses(const Units::Energy units = Units::Energy::kWh) const;

    ///////////////////////////////////////////////
    /* The following functions return, assign, or display temperatures (in degC) */

    double getSetpointT_C() const { return setpointT_C; }

    double getLocationT_C() const { return locationT_C; }

    double getOutletT_C() const { return outletT_C; }

    double getCondenserInletT_C() const { return condenserInletT_C; }

    double getCondenserOutletT_C() const { return condenserOutletT_C; }

    double getMaxCompressorSetpointT_C() const;

    double getMinOperatingT_C() const;

    double getTankNodeT_C(const int nodeNum) const;

    double getNthThermocoupleT_C(const int iTCouple, const int nTCouple) const;

    void getTankTs_C(std::vector<double>& tankTemps) const;

    void printTankTs_C();

    int setTankT_C(double tankT_C); /// assign uniform tank temperature

    void setInletT_C(double inletT_C_in) { inletT_C = inletT_C_in; }

    bool canApplySetpointT_C(double newSetpointT_C, double& maxSetpointT_C, std::string& why) const;

    int setSetpointT_C(const double setpointT_C_in);

    /// change the setpoint T, if possible
    int setSetpointT(const double setpointT, const Units::Temp units = Units::Temp::C);

    /// assign tank node temperatures by mapping the provided vector of temperatures
    int setTankTs_C(std::vector<double> tankTs_C_in);

    ///////////////////////////////////////////////
    /* The following functions return temperatures in user-specified units */

    double getSetpointT(const Units::Temp units = Units::Temp::C) const;

    double getMinOperatingT(const Units::Temp units = Units::Temp::C) const;

    double getTankNodeT(const int nodeNum, const Units::Temp units = Units::Temp::C) const;

    double getNthThermocoupleT(const int iTCouple,
                               const int nTCouple,
                               const Units::Temp units = Units::Temp::C) const;

    double getOutletT(const Units::Temp units = Units::Temp::C) const;

    double getCondenserInletT(const Units::Temp units = Units::Temp::C) const;

    double getCondenserOutletT(const Units::Temp units = Units::Temp::C) const;

    double getMaxCompressorSetpointT(const Units::Temp units = Units::Temp::C) const;

    int setMaxDepressionT(double maxDepression, const Units::Temp units = Units::Temp::C);

    int setTankTs(std::vector<double> tankTs_in, const Units::Temp units = Units::Temp::C);

    /// return whether specified new setpoint is physically possible for the compressor. If
    /// there is no compressor then checks that the new setpoint is less than boiling. The setpoint
    /// can be set higher than the compressor max outlet temperature if there is a backup
    /// resistance element, but the compressor will not operate above this temperature.
    bool canApplySetpointT(const double newSetpointT,
                           double& maxSetpointT,
                           std::string& why,
                           Units::Temp units = Units::Temp::C) const;

    ///////////////////////////////////////////////
    int resetTankToSetpoint(); /// reset tank to setpoint temperature

    bool isSetpointFixed() const; /// can the setpoint temperature be changed

    int setDoTempDepression(bool doTempDepress); /// set the temperature-depression option

    /** Returns State of Charge where
        mainsT_C = current mains (cold) water temp,
        minUsefulT_C = minimum useful temp,
        maxT_C = nominal maximum temp.*/

    double calcSoCFraction(double mainsT_C, double minUsefulT_C, double maxT_C) const;
    double calcSoCFraction(double mainsT_C, double minUsefulT_C) const
    {
        return calcSoCFraction(mainsT_C, minUsefulT_C, getSetpointT());
    };

    /// return State of Charge calculated from the SoC heating logics if used.
    double getSoCFraction() const;

    /// change the AirFlowFreedom value
    int setAirFlowFreedom(double fanFraction);

    /// set the tank size, adjusting the UA to maintain the same U
    int setTankSizeWithSameU(double tankSize,
                             Units::Volume units = Units::Volume::L,
                             bool forceChange = false);

    double getTankSurfaceArea(const Units::Area units = Units::Area::ft2) const;

    static double getTankSurfaceArea(double vol,
                                     Units::Volume volUnits = Units::Volume::L,
                                     Units::Area surfAUnits = Units::Area::ft2);

    /// return the tank surface area based on real storage tanks
    double getTankRadius(const Units::Length units = Units::Length::ft) const;

    /// returns the tank surface radius based off of real storage tanks
    static double getTankRadius(double vol,
                                const Units::Volume volUnits = Units::Volume::L,
                                const Units::Length radiusUnits = Units::Length::ft);

    /// report whether the tank size can be changed
    bool isTankSizeFixed() const;

    /// set the tank volume
    int setTankSize(const double volume,
                    const Units::Volume units = Units::Volume::L,
                    bool forceChange = false);

    /// returns the tank volume
    double getTankSize(const Units::Volume units = Units::Volume::L) const;

    /// set whether to run the inversion mixing method, default is true
    int setDoInversionMixing(bool doInvMix);

    /// set whether to perform inter-nodal conduction
    int setDoConduction(bool doCondu);

    /// set the UA with specified units
    int setUA(const double UA, const Units::UA units = Units::UA::kJ_per_hC);

    /// returns the UA with specified units
    int getUA(double& UA, const Units::UA units = Units::UA::kJ_per_hC) const;

    /// returns the fittings UA with specified units
    int getFittingsUA(double& UA, const Units::UA units = Units::UA::kJ_per_hC) const;

    /// set the fittings UA with specified units
    int setFittingsUA(const double UA, const Units::UA units = Units::UA::kJ_per_hC);

    /// set the water inlet height as a fraction from the bottom up
    int setInletByFraction(double fractionalHeight);

    /// set the second water inlet height as a fraction from the bottom up
    int setInlet2ByFraction(double fractionalHeight);

    /// set the external water inlet height as a fraction from the bottom up (split systems)
    int setExternalInletHeightByFraction(double fractionalHeight);

    /// set the external water outlet height as a fraction from the bottom up (split systems)
    int setExternalOutletHeightByFraction(double fractionalHeight);

    /// set an external heater port height as a fraction from the bottom up (split systems)
    int setExternalPortHeightByFraction(double fractionalHeight, int whichPort);

    /// return the index of the node for the external water inlet (split systems)
    int getExternalInletNodeIndex() const;

    /// returns the index of the node for the external water outlet (split systems)
    int getExternalOutletNodeIndex() const;

    /// get the node index for a specified fractional height
    int getFractionalHeightNodeIndex(double fractionalHeight, int& nodeIndex);

    /// set the timer limit (min) for the DR_TOT call (0 min, 1440 min)
    int setTimerLimitTOT(double limit_min);

    /// return the timer limit (min) for the DR_TOT call
    double getTimerLimitTOT_minute() const;

    /// return the node index of the specified water inlet
    int getInletNodeIndex(int whichInlet) const;

    /// resizes the tankTs_C and nextTankTs_C node vectors
    void setNumNodes(const std::size_t num_nodes);

    /// returns the number of nodes
    int getNumNodes() const;

    /// returns the index of the top node
    int getTopNodeIndex() const;

    /// returns the number of heat sources
    int getNumHeatSources() const;

    /// returns the number of resistance elements
    int getNumResistanceElements() const;

    /// returns the index of the last compressor in the heat source array.
    int getCompressorIndex() const;

    double getCompressorCapacity(double airTemp = 19.722,
                                 double inletTemp = 14.444,
                                 double outTemp = 57.222,
                                 Units::Power pwrUnit = Units::Power::kW,
                                 Units::Temp tempUnit = Units::Temp::C);
    /**< Returns the heating output capacity of the compressor for the current HPWH model.
    Note only supports HPWHs with one compressor, if multiple will return the last index
    of a compressor. Outlet temperatures greater than the max allowable setpoints will return an
    error, but for compressors with a fixed setpoint the */

    int setCompressorOutputCapacity(double newCapacity,
                                    double airTemp = 19.722,
                                    double inletTemp = 14.444,
                                    double outTemp = 57.222,
                                    Units::Power pwrUnit = Units::Power::kW,
                                    Units::Temp tempUnit = Units::Temp::C);
    /**< Sets the heating output capacity of the compressor at the defined air, inlet water, and
    outlet temperatures. For multi-pass models the capacity is set as the average between the
    inletTemp and outTemp since multi-pass models will increase the water temperature only a few
    degrees at a time (i.e. maybe 10 degF) until the tank reaches the outTemp, the capacity at
    inletTemp might not be accurate for the entire heating cycle.
    Note only supports HPWHs with one compressor, if multiple will return the last index
    of a compressor */

    /// scale the input capacity and COP
    int setScaleCapacityCOP(double scaleCapacity = 1., double scaleCOP = 1.);

    int
    setResistanceCapacity(double power, int which = -1, Units::Power pwrUNIT = Units::Power::kW);
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

    double getResistanceCapacity(int which = -1, Units::Power pwrUNIT = Units::Power::kW);
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

    /// check whether a valid heat source with index n exists
    bool isHeatSourceIndexValid(const int n) const;

    /// returns the energy input to the Nth heat source, with the specified units
    double getNthHeatSourceEnergyInput(int N, Units::Energy units = Units::Energy::kWh) const;

    /// returns energy transferred from the Nth heat source to the water, with the specified units
    double getNthHeatSourceEnergyOutput(int N, Units::Energy units = Units::Energy::kWh) const;

    /// returns energy removed from the environment into the heat source, with the specified units
    double
    getNthHeatSourceEnergyRemovedFromEnvironment(int N,
                                                 Units::Energy units = Units::Energy::kWh) const;

    /// return the run time for the Nth heat source (min)
    /// note: they may sum to more than 1 time step for concurrently running heat sources
    /// returns HPWH_ABORT for N out of bounds  */
    double getNthHeatSourceRunTime(int N) const;

    /// return 1 if the Nth heat source is currently engaged, 0 if it is not, or
    /// HPWH_ABORT for N out of bounds
    int isNthHeatSourceRunning(int N) const;

    /// returns the enum value for what type of heat source the Nth heat source is/
    HEATSOURCE_TYPE getNthHeatSourceType(int N) const;

    /// return the volume of water heated in an external in the specified units
    /// returns 0 when no external heat source is running
    double getExternalVolumeHeated(Units::Volume units = Units::Volume::L) const;

    /// return model identifier
    int getHPWHModel() const;

    int getCompressorCoilConfig() const;
    bool isCompressorMultipass() const;
    bool isCompressorExternalMultipass() const;

    bool hasCompressor() const;

    /// report whether the HPWH model has a compressor (false if storage or resistance tank)
    bool hasACompressor() const;

    /// returns 1 if compressor running, 0 if compressor not running, ABORT if no compressor
    int isCompressorRunning() const { return isNthHeatSourceRunning(getCompressorIndex()); }

    /// report whether the HPWH has any external heat sources, (compressor or resistance element)
    bool hasExternalHeatSource() const;

    /// return the constant flow rate for an external multipass heat source
    double getExternalMPFlowRate(const Units::FlowRate units = Units::FlowRate::gal_per_min) const;

    double getCompressorMinimumRuntime(const Units::Time units = Units::Time::min) const;

    int getSizingFractions(double& aquafract, double& percentUseable) const;
    /**< returns the fraction of total tank volume from the bottom up where the aquastat is
    or the turn on logic for the compressor, and the USEable fraction of storage or 1 minus
    where the shut off logic is for the compressor. If the logic spans multiple nodes it
    returns the weighted average of the nodes */

    /// report whether the HPWH is scalable
    bool isHPWHScalable() const;

    /// check the demand-response signal against the different heat source types
    bool shouldDRLockOut(HEATSOURCE_TYPE hs, DRMODES DR_signal) const;

    /// reset variables for timer associated with the DR_TOT call
    void resetTopOffTimer();

    bool hasEnteringWaterHighTempShutOff(int heatSourceIndex);

    /// check for and set specific high temperature shut off logics.
    /// HPWHs can only have one of these, which is at least typical
    int setEnteringWaterHighTempShutOff(double highTemp,
                                        bool tempIsAbsolute,
                                        int heatSourceIndex,
                                        Units::Temp units = Units::Temp::C);

    int setTargetSoCFraction(double target);

    bool canUseSoCControls();

    int switchToSoCControls(double targetSoC,
                            double hysteresisFraction = 0.05,
                            double tempMinUseful = 43.333,
                            bool constantMainsT = false,
                            double mainsT = 18.333,
                            Units::Temp tempUnit = Units::Temp::C);

    bool isSoCControlled() const;

    /// checks whether energy is balanced follwoing a simulation step
    bool isEnergyBalanced(const double drawVol_L,
                          const double prevHeatContent_kJ,
                          const double fracEnergyTolerance = 0.001);

    /// overloaded version of above that allows specification of inlet temperature
    bool isEnergyBalanced(const double drawVol_L,
                          double inletT_C_in,
                          const double prevHeatContent_kJ,
                          const double fracEnergyTolerance)
    {
        setInletT_C(inletT_C_in);
        return isEnergyBalanced(drawVol_L, prevHeatContent_kJ, fracEnergyTolerance);
    }

    /// ad heat from a normal heat source; return excess heat, if needed, to prevent
    /// exceeding maximum or setpoint
    double addHeatAboveNode(double qAdd_kJ, const int nodeNum, const double maxT_C);

    /// add "extra" heat handled separately from normal heat sources
    void addExtraHeatAboveNode(double qAdd_kJ, const int nodeNum);

    struct PerfPointStore
    {
        double T_C;
        std::vector<double> inputPower_coeffs;
        std::vector<double> COP_coeffs;
    };

    struct PerfPoint
    {
        double T_C;
        std::vector<double> inputPower_coeffs_kW; // c0 + c1*T + c2*T*T
        std::vector<double> COP_coeffs;           // c0 + c1*T + c2*T*T

        PerfPoint(const double T_in = 0.,
                  const std::vector<double>& inputPower_coeffs_in = {},
                  std::vector<double> COP_coeffs_in = {},
                  const Units::Temp unitsTemp = Units::Temp::C,
                  const Units::Power unitsPower = Units::Power::kW);

        PerfPoint(const PerfPointStore& perfPointStore,
                  const Units::Temp unitsTemp = Units::Temp::C,
                  const Units::Power unitsPower = Units::Power::kW)
            : PerfPoint(Units::convert(perfPointStore.T_C, Units::Temp::C, unitsTemp),
                        perfPointStore.inputPower_coeffs,
                        perfPointStore.COP_coeffs,
                        unitsTemp,
                        unitsPower)
        {
        }
    };

    /// A map with input/COP quadratic curve coefficients at a given external temperature
    typedef std::vector<PerfPoint> PerfMap;

    static const std::vector<int> powers3;
    static const std::vector<std::pair<int, int>> powers6;
    static const std::vector<std::tuple<int, int, int>> powers11;

  private:
    class HeatSource;

    /// sets all the values to defaults
    void setAllDefaults();

    void updateTankTemps(
        double draw, double inletT_C, double ambientT_C, double inletVol2_L, double inletT2_L);

    /// mix to remove any temperature inversions in the tank
    void mixTankInversions();

    void updateSoCIfNecessary();

    /// report whether all the heat sources are off
    bool areAllHeatSourcesOff() const;

    /// disengage all heat sources
    void turnAllHeatSourcesOff();

    void addHeatParent(HeatSource* heatSourcePtr, double heatSourceAmbientT_C, double minutesToRun);

    /// adds extra heat to the set of nodes that are at the same temperature, above the
    ///	specified node number
    void modifyHeatDistribution(std::vector<double>& heatDistribution);
    void addExtraHeat(std::vector<double>& extraHeatDist_W);

    ///  "extra" heat (kJ) added during a simulation step
    double extraEnergyInput_kJ;

    /// calculate the tank temperature using logic-node weighting
    double tankAvg_C(const std::vector<NodeWeight> nodeWeights) const;

    /// shift temperatures of tank nodes with indices in the range [mixBottomNode, mixBelowNode)
    /// by a factor mixFactor towards their average temperature
    void mixTankNodes(int mixBottomNode, int mixBelowNode, double mixFactor);

    /// calculate condentropy and the lowest node
    void calcDerivedValues();

    /// set constants for the UA and tank size
    void calcSizeConstants();

    /// calculate condentropy and the lowest node
    void calcDerivedHeatingValues();

    /// map positions of the resistance elements to their indices in heatSources
    void mapResRelativePosToHeatSources();

    /// run checks on the HPWH input parameters
    int checkInputs();

    double getChargePerNode(double coldT, double mixT, double hotT) const;

    void calcAndSetSoCFraction();

    void sayMessage(const std::string message) const;
    /**< if the messagePriority is >= the hpwh verbosity,
    either pass your message out to the callback function or print it to cout
    otherwise do nothing  */
    void msg(const char* fmt, ...) const;
    void msgV(const char* fmt, va_list ap = NULL) const;

    /// did an internal error cause the simulation to fail?
    bool simHasFailed;

    /// is the hpwh currently heating or not?
    bool isHeating;

    /// does the HPWH allow the setpoint to vary
    bool setpointFixed;

    /// does the HPWH have a constant tank size or can it be changed
    bool tankSizeFixed;

    /// can the HPWH scale capactiy and COP or not
    bool canScale;

    /// an enum to let the sim know how much output to say
    VERBOSITY hpwhVerbosity;

    /// function pointer to indicate an external message processing function
    void (*messageCallback)(const std::string message, void* contextPtr);

    /// caller context pointer for external message processing
    void* messageCallbackContextPtr;

    /// The hpwh should know which preset initialized it, or if it was from a fileget
    MODELS model;

    /// contains the HeatSources, in order of priority
    std::vector<HeatSource> heatSources;

    /// index of the compressor heat source (set to -1 if no compressor)
    int compressorIndex;

    /// The index of the lowest resistance element heat source
    /// (-1 if no resistance elements)
    int lowestElementIndex;

    /// index of the highest resistance element heat source. if only one element it equals
    /// lowestElementIndex (-1 if no resistance elements)
    int highestElementIndex;

    /// index of the VIP resistance element heat source
    /// (-1 if no VIP resistance elements)
    int VIPIndex;

    /// tank node index of the inlet [0, numNodes-1]
    int inletNodeIndex;

    /// tank node index of the second inlet [0, numNodes-1]
    int inlet2NodeIndex;

    /// the volume (L) of the tank
    double tankVolume_L;

    /// the UA of the tank, in metric units
    double tankUA_kJperhC;

    /// the UA of the fittings for the tank, in metric units
    double fittingsUA_kJperhC;

    /// the volume (L) of a single node
    double nodeVolume_L;

    /// heat capacity (kJ/degC) of the fluid (water, except for heat-exchange models)
    /// in a single node
    double nodeCp_kJperC;

    /// the height in meters of the one node
    double nodeHeight_m;

    /// fraction of the UA on the top and bottom of the tank (cylinder assumed)
    double fracAreaTop;

    /// fraction of the UA on the sides of the tank (cylinder assumed)
    double fracAreaSide;

    /// the current state of charge according to the logic
    double currentSoCFraction;

    /// setpoint temperature of the tank
    double setpointT_C;

    /// node temperatures - 0 is the bottom node
    std::vector<double> tankTs_C;

    /// future temperature of each node for the conduction calculation
    /// 0 is the bottom node
    std::vector<double> nextTankTs_C;

    /// the temperature of the inlet water; typically an input parameter
    double inletT_C;

    /// the temperature of the outlet water - taken from top of tank, 0 if no flow
    double outletT_C;

    /// the temperature of the inlet water to the condensor either an average of tank nodes or
    /// taken from the bottom, 0 if no flow or no compressor
    double condenserInletT_C;

    /// the temperature of the outlet water from the condenser either
    /// 0 if no flow or no compressor
    double condenserOutletT_C;

    /// the volume of water heated by an external source, 0 if no flow or no external heat
    /// source
    double externalVolumeHeated_L;

    /// the amount of heat lost to standby
    double standbyLosses_kJ;

    // special variables for adding abilities

    /// whether the bottom fraction (defined by mixBelowFraction)
    /// of the tank should mix during draws
    bool tankMixesOnDraw;

    ///  mixes the tank below this fraction on draws iff tankMixesOnDraw
    double mixBelowFractionOnDraw;

    /// whether the HPWH should use the alternate ambient temperature that
    /// gets depressed when a compressor is running
    /// NOTE: this only works for 1 minute steps
    bool doTempDepression;

    /// special location temperature that stands in for the
    /// ambient temperature if you are doing temp. depression
    double locationT_C;

    double maxDepressionT_C = 2.5;

    /// the DRstatus of the tank in the previous time step and at the end of runOneStep
    DRMODES prevDRstatus;

    /// the time limit in minutes on the timer when the compressor and resistance elements are
    /// turned back on, used with DR_TOT.
    double timerLimitTOT;

    /// the timer used for DR_TOT to turn on the compressor and resistance elements.
    double timerTOT;

    bool usesSoCLogic;
    /// values which are typically inputs
    double minutesPerStep = 1., secondsPerStep, hoursPerStep;

    /// iff true will model temperature inversion mixing in the tank
    bool doInversionMixing;

    /// iff true will model conduction between the internal nodes of the tank
    bool doConduction;

    struct resPoint
    {
        int index;
        int position;
    };

    /// map of resistance element index to position in the tank,
    /// sorted by height from lowest to highest
    std::vector<resPoint> resistanceHeightMap;

    /// Generates a vector of logical nodes
    std::vector<HPWH::NodeWeight> getNodeWeightRange(double bottomFraction, double topFraction);

    /// true if tank provides heat exchange only
    bool hasHeatExchanger;

    /// coefficient (0-1) of effectiveness for heat exchange between tank and water line
    /// (heat-exchange models only).
    double heatExchangerEffectiveness;

    /// cefficient (0-1) of effectiveness for heat exchange between a single tank node and water
    /// line (derived from heatExchangerEffectiveness).
    double nodeHeatExchangerEffectiveness;

}; // end of HPWH class

class HPWH::HeatSource
{
  public:
    friend class HPWH;

    HeatSource(HPWH* hpwh_in = nullptr);

    HeatSource(const HeatSource& hSource);            /// copy constructor
    HeatSource& operator=(const HeatSource& hSource); /// assignment operator

    /// configure the heat source to be a resisive element, positioned at the
    /// specified node, with the specified power in watts
    void setupAsResistiveElement(const int node,
                                 const double power,
                                 const Units::Power units = Units::Power::kW,
                                 const int condensitySize = CONDENSITY_SIZE);

    bool isEngaged() const;                             /// true if on
    void engageHeatSource(DRMODES DRstatus = DR_ALLOW); /// turn on
    void disengageHeatSource();                         /// turn off

    bool isLockedOut() const; /// true if locked out
    void lockOutHeatSource(); /// lock out
    void unlockHeatSource();  /// unlock

    bool shouldLockOut(double heatSourceAmbientT_C) const; /// true if should lock out
    bool shouldUnlock(double heatSourceAmbientT_C) const;  /// true if should unlock

    /// lock or unlock, as needed; return resulting state (true: locked, false: unlocked)
    bool toLockOrUnlock(double heatSourceAmbientT_C);

    bool shouldHeat() const; /// return whether to turn on

    bool shouldShutOff() const; /// return whether to turn off

    /// return whether either maximum or setpoint temperature is exceeded
    bool maxedOut() const;

    int findParent() const;
    /**< returns the index of the heat source where this heat source is a backup.
        returns -1 if none found. */

    /// return a measure of the current state from the shutoff condition (external
    /// configurations)
    double fractToMeetComparisonExternal() const;

    /// add heat
    void addHeat(double externalT_C, double minutesToRun);

    /// Assign new condensity values from supplied vector. Note the input vector is currently
    /// resampled to preserve a condensity vector of size CONDENSITY_SIZE.
    void setCondensity(const std::vector<double>& condensity_in);

    int getCondensitySize() const;

    /// perform btwxt linear interpolation to the target point
    void btwxtInterp(double& inputPower, double& cop, std::vector<double>& target);

    /// configure a map for defrost derating
    void setupDefrostMap(double derate35 = 0.8865);

    /// perform cop derating based on the air temperature
    void defrostDerate(double& to_derate, double airT_C);

  private:
    // start with a few type definitions
    enum COIL_CONFIG
    {
        CONFIG_SUBMERGED,
        CONFIG_WRAPPED,
        CONFIG_EXTERNAL
    };

    /// owner of the heat source, necessary to access HPWH variables
    HPWH* hpwh;

    HEATSOURCE_TYPE typeOfHeatSource; /// compressor, resistance, extra, none
    COIL_CONFIG configuration;        /// submerged, wrapped, external

    // state/output variables

    bool isOn; /// whether heat source is running

    bool lockedOut; /// whether the heat source is locked out

    /// iff true will derate the COP of a compressor to simulate a defrost cycle
    bool doDefrost;

    /// tracks the time that heat source was running
    double runtime_min;

    /// energy supplied to the heat source in the previous time step
    double energyInput_kJ;

    /// energy (kJ) transferred to the tank in the previous time step
    double energyOutput_kJ;

    /// energy removed from the environment in the previous time step
    double energyRemovedFromEnvironment_kJ;

    // property variables

    /// is this heat source a high priority heat source? (e.g., upper resistor)
    bool isVIP;

    /// pointer to heat source that serves as backup to this one (NULL if none).
    HeatSource* backupHeatSource;

    /// pointer to the heat source that runs concurrently with this one
    /// it still will only turn on if shouldShutOff is false.
    HeatSource* companionHeatSource;

    /// pointer to the heat source which will attempt to run after this one
    HeatSource* followedByHeatSource;

    /// condensity represents the location within the tank where heat will be distributed,
    /// and it also is used to calculate the condenser temperature for inputPower/COP calcs.
    /// It is conceptually linked to the way condenser coils are wrapped around
    /// (or within) the tank, however a resistance heat source can also be simulated
    /// by specifying the entire condensity in one node.
    std::vector<double> condensity;

    /// shrinkageT_C is a derived from the condentropy (conditional entropy),
    /// using the condensity and fixed parameters Talpha_C and Tbeta_C.
    /// Talpha_C and Tbeta_C are not intended to be settable
    /// see the hpwh_init functions for calculation of shrinkage.
    double shrinkageT_C;

    PerfMap perfMap;

    /// The axis values defining the regular grid for the performance data.
    /// SP would have 3 axis, MP would have 2 axis
    std::vector<std::vector<double>> perfGrid;

    /// The values for input power and cop use matching to the grid. Should be long format with
    /// { { * inputPower_W }, { COP } }.
    std::vector<std::vector<double>> perfGridValues;

    /// The grid interpolator used for mapping performance
    std::shared_ptr<Btwxt::RegularGridInterpolator> perfRGI;

    bool useBtwxtGrid;

    EXTRAP_METHOD extrapolationMethod; /**< linear or nearest neighbor*/

    /// a vector to hold the set of logical choices for turning this element on
    std::vector<std::shared_ptr<HeatingLogic>> turnOnLogicSet;

    /// a vector to hold the set of logical choices that can cause an element to turn off
    std::vector<std::shared_ptr<HeatingLogic>> shutOffLogicSet;

    /// a single logic that checks the bottom point is below a temperature so the system doesn't
    /// short cycle
    std::shared_ptr<TempBasedHeatingLogic> standbyLogic;

    /// some compressors have a resistance element for defrost
    struct ResistanceDefrost
    {
        double inputPwr_kW {0.};
        double constLiftT_C {0.};
        double onBelowT_C {-999};

        ResistanceDefrost(const double inputPwr_in = 0.,
                          const double constLiftT_in = 0.,
                          const double onBelowT_in = 0.,
                          const Units::Temp unitsTemp_in = Units::Temp::C,
                          const Units::Power unitsPower_in = Units::Power::kW)
        {
            Units::TempDiff unitsTempDiff_in =
                (unitsTemp_in == Units::Temp::C) ? Units::TempDiff::C : Units::TempDiff::F;
            inputPwr_kW = Units::convert(inputPwr_in, unitsPower_in, Units::Power::kW);
            constLiftT_C = Units::convert(constLiftT_in, unitsTempDiff_in, Units::TempDiff::C);
            onBelowT_C = Units::convert(onBelowT_in, unitsTemp_in, Units::Temp::C);
        }
    } resDefrost;

    struct defrostPoint
    {
        double T_C;
        double derate_fraction;
    };

    /// A list of points for the defrost derate factor ordered by increasing external
    /// temperature
    std::vector<defrostPoint> defrostMap;

    ///  maximum output temperature at the minimum operating temperature (minT_C) of HPWH
    ///  environment
    struct maxOut_minAir
    {
        double outT_C;
        double airT_C;
    } maxOut_at_LowT;

    /// approximate a secondary
    /// heat exchanger by adding extra input energy for the pump and an increase in the water to
    /// the incoming waater temperature to the heatpump
    struct SecondaryHeatExchanger
    {
        double coldSideOffsetT_C;
        double hotSideOffsetT_C;
        double extraPumpPower_W;
    } secondaryHeatExchanger;

    void addTurnOnLogic(std::shared_ptr<HeatingLogic> logic);
    void addShutOffLogic(std::shared_ptr<HeatingLogic> logic);

    /// small functions to remove some of the cruft in initiation functions
    void clearAllTurnOnLogic();
    void clearAllShutOffLogic();
    void clearAllLogic();

    /// change the resistance wattage
    void changeResistancePower(const double power, const Units::Power units = Units::Power::kW);

    /// returns if the heat source uses a compressor or not
    bool isACompressor() const;

    /// returns whether the heat source uses a resistance element or not
    bool isAResistance() const;

    bool isExternalMultipass() const;

    ///  minimum operating temperature
    double minT_C;

    ///  maximum operating temperature
    double maxT_C;

    /// maximum setpoint of the heat source can create, used for compressors predominately
    double maxSetpointT_C;

    /// a hysteresis term that prevents short cycling due to heat pump self-interaction
    /// when the heat source is engaged, it is subtracted from lowT cutoffs and
    /// added to lowTreheat cutoffs
    double hysteresisOffsetT_C;

    ///  heat pumps can depress the temperature of their space in certain instances -
    /// whether or not this occurs is a bool in HPWH, but a heat source must
    ///  know if it is capable of contributing to this effect or not
    ///  NOTE: this only works for 1 minute steps
    ///  ALSO:  this is set according the the heat source type, not user-specified
    bool depressesTemperature;

    /// airflowFreedom is the fraction of full flow. This is used to de-rate compressor
    /// cop (not capacity) for cases where the air flow is restricted - typically ducting
    double airflowFreedom;

    /// The node height at which the external HPWH adds heated water to the storage tank.
    /// Defaults to top for single pass.
    int externalInletNodeIndex;

    /// The node height at which the external HPWH takes cold water out of the storage tank.
    /// Defaults to bottom for single pass.
    int externalOutletNodeIndex;

    /// number of the first non-zero condensity entry
    int lowestNode;

    /// The multipass flow rate
    double mpFlowRate_LPS;

    /// single pass or multi-pass. Anything not obviously split system single
    /// pass is multipass
    bool isMultipass;

    // private functions, mostly used for heating the water with the addHeat function

    ///  Add heat from a source outside of the tank. Assume the condensity is where
    /// the water is drawn from and hot water is put at the top of the tank.
    double addHeatExternal(
        double externalT_C, double minutesToRun, double& cap_kW, double& input_kW, double& cop);

    /// Add heat from external source using a multi-pass configuration
    double addHeatExternalMP(
        double externalT_C, double minutesToRun, double& cap_kW, double& input_kW, double& cop);

    /// some methods to help with the add heat interface - MJL
    void getCapacity(double externalT_C,
                     double condenserT_C,
                     double setpointT_C,
                     double& input_kW,
                     double& cap_kW,
                     double& cop);

    /// An overloaded function that uses uses the setpoint temperature
    void getCapacity(
        double externalT_C, double condenserT_C, double& input_kW, double& cap_kW, double& cop)
    {
        getCapacity(externalT_C, condenserT_C, hpwh->getSetpointT(), input_kW, cap_kW, cop);
    };

    /// An equivalent getCapacity function just for multipass external (or split) HPWHs
    void getCapacityMP(
        double externalT_C, double condenserT_C, double& input_kW, double& cap_kW, double& cop);

    void calcHeatDist(std::vector<double>& heatDistribution);

    /// returns the tank temperature (C) weighted by the condensity for this heat source
    double getTankT_C() const;

    /// sorts the Performance Map by increasing external temperatures
    void sortPerformanceMap();

}; // end of HeatSource class

const double Pi = 4. * atan(1.);

// reference conversion factors
constexpr double s_per_min = 60.;            // s / min
constexpr double min_per_h = 60.;            // min / h
constexpr double in3_per_gal = 231.;         // in^3 / gal (U.S., exact)
constexpr double m_per_in = 0.0254;          // m / in (exact)
constexpr double F_per_C = 1.8;              // degF / degC
constexpr double offsetF = 32.;              // degF offset
constexpr double kJ_per_Btu = 1.05505585262; // kJ / Btu (IT), https://www.unitconverters.net/

// useful conversion factors
constexpr double s_per_h = s_per_min * min_per_h;                                  // s / h
constexpr double Btu_per_kJ = 1. / kJ_per_Btu;                                     // Btu / kJ
constexpr double ft_per_m = 1. / m_per_in / 12.;                                   // ft / m
constexpr double gal_per_L = 1.e-3 / in3_per_gal / m_per_in / m_per_in / m_per_in; // gal / L
constexpr double ft3_per_L = ft_per_m * ft_per_m * ft_per_m / 1000.;               // ft^3 / L

// identity
inline double ident(const double x) { return x; }

// time conversion
inline double S_TO_MIN(const double s) { return s / s_per_min; }
inline double MIN_TO_S(const double min) { return s_per_min * min; }

inline double S_TO_H(const double s) { return s / s_per_h; }
inline double H_TO_S(const double h) { return s_per_h * h; }

inline double MIN_TO_H(const double min) { return S_TO_H(MIN_TO_S(min)); }
inline double H_TO_MIN(const double h) { return S_TO_MIN(H_TO_S(h)); }

// temperature conversion
inline double dC_TO_dF(const double dC) { return F_per_C * dC; }
inline double dF_TO_dC(const double dF) { return dF / F_per_C; }

inline double C_TO_F(const double C) { return (F_per_C * C) + offsetF; }
inline double F_TO_C(const double F) { return (F - offsetF) / F_per_C; }

// energy conversion
inline double KJ_TO_KWH(const double kJ) { return kJ / s_per_h; }
inline double KWH_TO_KJ(const double kWh) { return kWh * s_per_h; }

inline double KJ_TO_BTU(const double kJ) { return Btu_per_kJ * kJ; }
inline double BTU_TO_KJ(const double Btu) { return kJ_per_Btu * Btu; }

inline double KWH_TO_BTU(const double kWh) { return KJ_TO_BTU(KWH_TO_KJ(kWh)); }
inline double BTU_TO_KWH(const double Btu) { return KJ_TO_KWH(BTU_TO_KJ(Btu)); }

// power conversion
inline double KW_TO_BTUperH(const double kW) { return Btu_per_kJ * s_per_h * kW; }
inline double BTUperH_TO_KW(const double Btu_per_h) { return kJ_per_Btu * Btu_per_h / s_per_h; }

inline double KW_TO_W(const double kW) { return 1000. * kW; }
inline double W_TO_KW(const double W) { return W / 1000.; }

inline double KW_TO_KJperH(const double kW) { return kW * s_per_h; }
inline double KJperH_TO_KW(const double kJ_per_h) { return kJ_per_h / s_per_h; }

inline double BTUperH_TO_W(const double Btu_per_h) { return KW_TO_W(BTUperH_TO_KW(Btu_per_h)); }
inline double W_TO_BTUperH(const double W) { return KW_TO_BTUperH(W_TO_KW(W)); }

inline double BTUperH_TO_KJperH(const double Btu_per_h)
{
    return KW_TO_KJperH(BTUperH_TO_KW(Btu_per_h));
}
inline double KJperH_TO_BTUperH(const double kJ_per_h)
{
    return KW_TO_BTUperH(KJperH_TO_KW(kJ_per_h));
}

inline double W_TO_KJperH(const double W) { return KW_TO_KJperH(W_TO_KW(W)); }
inline double KJperH_TO_W(const double kJ_per_h) { return KW_TO_W(KJperH_TO_KW(kJ_per_h)); }

// length conversion
inline double M_TO_FT(const double m) { return ft_per_m * m; }
inline double FT_TO_M(const double ft) { return ft / ft_per_m; }

// area conversion
inline double M2_TO_FT2(const double m2) { return (ft_per_m * ft_per_m * m2); }
inline double FT2_TO_M2(const double ft2) { return (ft2 / ft_per_m / ft_per_m); }

// volume conversion
inline double L_TO_GAL(const double L) { return gal_per_L * L; }
inline double GAL_TO_L(const double gal) { return gal / gal_per_L; }

inline double L_TO_FT3(const double L) { return ft3_per_L * L; }
inline double FT3_TO_L(const double ft3) { return ft3 / ft3_per_L; }

inline double GAL_TO_FT3(const double gal) { return L_TO_FT3(GAL_TO_L(gal)); }
inline double FT3_TO_GAL(const double ft3) { return L_TO_GAL(FT3_TO_L(ft3)); }

// flow-rate conversion
inline double GPM_TO_LPS(const double gpm) { return (gpm / gal_per_L / s_per_min); }
inline double LPS_TO_GPM(const double lps) { return (gal_per_L * lps * s_per_min); }

// UA conversion
inline double KJperHC_TO_BTUperHF(const double UA_kJperhC)
{
    return Btu_per_kJ * UA_kJperhC / F_per_C;
}

inline double BTUperHF_TO_KJperHC(const double UA_BTUperhF)
{
    return F_per_C * UA_BTUperhF / Btu_per_kJ;
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

/// linear interpolation between two points to the xnew point
void linearInterp(double& ynew, double xnew, double x0, double x1, double y0, double y1);

double expandSeries(const std::vector<double>& coeffs, const double x);

/// applies ten-term regression
void regressedMethod(
    double& ynew, std::vector<double>& coefficents, double x1, double x2, double x3);

/// applies five-term regression for MP split systems
void regressedMethodMP(double& ynew, std::vector<double>& coefficents, double x1, double x2);

#endif
