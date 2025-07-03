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
#include <nlohmann/json.hpp>

#include "hpwh-data-model.hh"
#include "presets.h"
#include "HPWHUtils.hh"

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

    class Tank;
    class HeatSource;
    class Condenser;
    class Resistance;
    struct HeatingLogic;
    struct SoCBasedHeatingLogic;
    struct TempBasedHeatingLogic;

    static const int version_major = HPWHVRSN_MAJOR;
    static const int version_minor = HPWHVRSN_MINOR;
    static const int version_patch = HPWHVRSN_PATCH;
    static const std::string version_maint;

    static const int LOGIC_SIZE =
        12; /**< number of logic nodes associated with temperature-based heating logic */

    static const double DENSITYWATER_kgperL; /// mass density of water
    static const double KWATER_WpermC;       /// thermal conductivity of water
    static const double CPWATER_kJperkgC;    /// specific heat capcity of water
    static const double TOL_MINVALUE; /**< any amount of heat distribution less than this is reduced
                                                to 0 this saves on computations */

    static const float UNINITIALIZED_LOCATIONTEMP; /**< this is used to tell the
   simulation when the location temperature has not been initialized */
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
                                                                                       eventually?
              */

    void from(const hpwh_data_model::hpwh_sim_input::HPWHSimInput& hsi);

    void to(hpwh_data_model::hpwh_sim_input::HPWHSimInput& hsi) const;

    void from(const hpwh_data_model::rsintegratedwaterheater::RSINTEGRATEDWATERHEATER& rswh);

    void to(hpwh_data_model::rsintegratedwaterheater::RSINTEGRATEDWATERHEATER& rswh) const;

    void from(const hpwh_data_model::central_water_heating_system::CentralWaterHeatingSystem& cwhs);

    void to(hpwh_data_model::central_water_heating_system::CentralWaterHeatingSystem& cwhs) const;

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
    hpwh_presets::MODELS model;

    /// data entry to/from schema
    template <typename T>
    class Entry
    {
      private:
        T t;
        bool is_set = false;

      public:
        Entry(const T& t_in, const bool is_set_in) : is_set(is_set_in)
        {
            if (is_set)
                t = t_in;
            else
                t = T();
        }
        Entry(const T& t_in) : t(t_in), is_set(true) {}
        Entry() : T(T()), is_set(false) {}

        T operator()() const { return is_set ? t : T(); }
        bool isSet() const { return is_set; }

        void from(const T& t_in, const bool is_set_in) { *this = {t_in, is_set_in}; }

        void to(T& t_in, bool& is_set_in) const
        {
            if (is_set)
            {
                t_in = t;
            }
            is_set_in |= is_set;
        }
    };

    struct Description : public Entry<std::string>
    {
        Description() : Entry<std::string>("", false) {}
        Description(std::string description_in) : Entry<std::string>(description_in) {}
        Description(const Entry<std::string>& entry) : Entry<std::string>(entry) {}
        bool empty() const { return !(Entry<std::string>::isSet()); }

        //-----------------------------------------------------------------------------
        ///	@brief	Transfer field from schema
        //-----------------------------------------------------------------------------
        template <typename RSTYPE>
        void from(const RSTYPE& rs)
        {
            if (rs.metadata_is_set)
            {
                auto& metadata = rs.metadata;
                Entry<std::string>::from(metadata.description, metadata.description_is_set);
            }
        }

        //-----------------------------------------------------------------------------
        ///	@brief	Transfer field to schema
        //-----------------------------------------------------------------------------
        template <typename RSTYPE>
        void to(RSTYPE& rs) const
        {
            auto& metadata = rs.metadata;
            Entry<std::string>::to(metadata.description, metadata.description_is_set);
        }

    } description;

    struct ProductInformation
    {
        Entry<std::string> manufacturer;
        Entry<std::string> model_number;
        ProductInformation() : manufacturer("", false), model_number("", false) {}
        ProductInformation(std::string manufacturer_in, std::string model_number_in)
            : manufacturer(manufacturer_in), model_number(model_number_in)
        {
        }
        bool empty() const { return !(manufacturer.isSet() || model_number.isSet()); }
        bool full() const { return (manufacturer.isSet() && model_number.isSet()); }

        //-----------------------------------------------------------------------------
        ///	@brief	Transfer fields from schema
        //-----------------------------------------------------------------------------
        template <typename RSTYPE>
        void from(const RSTYPE& rs)
        {
            if (rs.description_is_set)
            {
                auto& desc = rs.description;
                if (desc.product_information_is_set)
                {
                    auto& info = desc.product_information;
                    manufacturer.from(info.manufacturer, info.manufacturer_is_set);
                    model_number.from(info.model_number, info.model_number_is_set);
                }
            }
        }

        //-----------------------------------------------------------------------------
        ///	@brief	Transfer fields to schema
        //-----------------------------------------------------------------------------
        template <typename RSTYPE>
        void to(RSTYPE& rs) const
        {
            auto& desc = rs.description;
            auto& prod_info = desc.product_information;

            manufacturer.to(prod_info.manufacturer, prod_info.manufacturer_is_set);
            model_number.to(prod_info.model_number, prod_info.model_number_is_set);

            // data model requires both or none
            checkTo(prod_info, desc.product_information_is_set, desc.product_information, full());
            checkTo(desc, rs.description_is_set, rs.description, full());
        }

    } productInformation;

    struct Rating10CFR430
    {
        Entry<std::string> certified_reference_number;
        Entry<double> nominal_tank_volume;
        Entry<double> first_hour_rating;
        Entry<double> recovery_efficiency;
        Entry<double> uniform_energy_factor;

        Rating10CFR430()
            : certified_reference_number("", false)
            , nominal_tank_volume(0., false)
            , first_hour_rating(0., false)
            , recovery_efficiency(0., false)
            , uniform_energy_factor(0., false)
        {
        }

        bool empty() const
        {
            return !(certified_reference_number.isSet() || nominal_tank_volume.isSet() ||
                     first_hour_rating.isSet() || recovery_efficiency.isSet() ||
                     uniform_energy_factor.isSet());
        }

        //-----------------------------------------------------------------------------
        ///	@brief	Transfer fields from schema
        //-----------------------------------------------------------------------------
        void from(const hpwh_data_model::rsintegratedwaterheater::RSINTEGRATEDWATERHEATER& rs)
        {
            if (rs.description_is_set)
            {
                auto& desc = rs.description;
                if (desc.rating_10_cfr_430_is_set)
                {
                    auto& info = desc.rating_10_cfr_430;
                    certified_reference_number.from(info.certified_reference_number,
                                                    info.certified_reference_number_is_set);
                    nominal_tank_volume.from(info.nominal_tank_volume,
                                             info.nominal_tank_volume_is_set);
                    first_hour_rating.from(info.first_hour_rating, info.first_hour_rating_is_set);
                    recovery_efficiency.from(info.recovery_efficiency,
                                             info.recovery_efficiency_is_set);
                    uniform_energy_factor.from(info.uniform_energy_factor,
                                               info.uniform_energy_factor_is_set);
                }
            }
        }

        //-----------------------------------------------------------------------------
        ///	@brief	Transfer fields to schema
        //-----------------------------------------------------------------------------
        void to(hpwh_data_model::rsintegratedwaterheater::RSINTEGRATEDWATERHEATER& rs) const
        {
            auto& desc = rs.description;
            auto& rating = desc.rating_10_cfr_430;

            certified_reference_number.to(rating.certified_reference_number,
                                          rating.certified_reference_number_is_set);
            nominal_tank_volume.to(rating.nominal_tank_volume, rating.nominal_tank_volume_is_set);
            first_hour_rating.to(rating.first_hour_rating, rating.first_hour_rating_is_set);
            recovery_efficiency.to(rating.recovery_efficiency, rating.recovery_efficiency_is_set);
            uniform_energy_factor.to(rating.uniform_energy_factor,
                                     rating.uniform_energy_factor_is_set);

            checkTo(rating, desc.rating_10_cfr_430_is_set, desc.rating_10_cfr_430, !empty());
            checkTo(desc, rs.description_is_set, rs.description, !empty());
        }

    } rating10CFR430;

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

    /** specifies the unit type for outputs in the CSV file-s  */
    enum CSVOPTIONS
    {
        CSVOPT_NONE = 0,
        CSVOPT_IPUNITS = 1 << 0,
        CSVOPT_IS_DRAWING = 1 << 1
    };

    ///	@struct DistributionPoint
    /// (height, weight) pair for weighted distributions
    struct DistributionPoint
    {
        double height, weight;
    };

    ///	@struct WeightedDistribution
    /// distribution specified by (height, weight) pairs
    struct WeightedDistribution : public std::vector<DistributionPoint>
    {
      public:
        /// default constructor
        WeightedDistribution() : std::vector<DistributionPoint>() {}

        /// construction from separate height, weight vectors
        WeightedDistribution(const std::vector<double> heights, const std::vector<double> weights)
        {
            clear();
            reserve(heights.size());
            auto weight = weights.begin();
            for (auto& height : heights)
                if (weight != weights.end())
                {
                    push_back({height, *weight});
                    ++weight;
                }
        }

        /// construct from a node distribution
        WeightedDistribution(const std::vector<double> node_distribution)
        {
            clear();
            auto nNodes = node_distribution.size();
            double node_sum = 0.;
            for (auto& node : node_distribution)
                node_sum += node;
            for (std::size_t i = 0; i < nNodes; ++i)
            {
                double height = static_cast<double>(i + 1) / nNodes;
                double weight = static_cast<double>(nNodes) * node_distribution[i] / node_sum;
                if (i == nNodes - 1)
                {
                    push_back({height, weight});
                    break;
                }
                if (node_distribution[i] != node_distribution[i + 1])
                {
                    push_back({height, weight});
                }
            }
        }

        double maximumHeight() const { return back().height; }

        double maximumWeight() const
        {
            double res = 0.;
            for (auto distPoint : (*this))
                res = std::max(distPoint.weight, res);
            return res;
        }

        /// find total weight (using normalized height)
        double totalWeight() const
        {
            double total = 0.;
            double prevHeight = 0.;
            for (auto distPoint : (*this))
            {
                double& height = distPoint.height;
                double& weight = distPoint.weight;
                total += weight * (height - prevHeight);
                prevHeight = height;
            }
            return total / prevHeight;
        }

        /// find fractional (of maxima) values by index
        double fractionalHeight(std::size_t i) const { return (*this)[i].height / maximumHeight(); }
        double fractionalWeight(std::size_t i) const { return (*this)[i].weight / maximumWeight(); }

        bool isValid() const
        {
            bool isNotEmpty = !empty();
            bool hasWeight = false;
            bool isSorted = true;
            double prevHeight = 0.;
            for (auto& distPoint : (*this))
            {
                isSorted &= (distPoint.height > prevHeight);
                hasWeight |= (distPoint.weight > 0.);
                prevHeight = distPoint.height;
            }
            return isNotEmpty && hasWeight && isSorted;
        }

        /// @brief returns the normalized weight within a normalized height range
        double normalizedWeight(double beginFrac, double endFrac) const
        {
            double res = 0.;
            double prevFrac = beginFrac;
            for (auto distPoint : (*this))
            {
                double frac = distPoint.height / maximumHeight();
                if (frac < beginFrac)
                    continue;
                if (frac > prevFrac)
                {
                    if (frac > endFrac)
                    {
                        res += distPoint.weight * (endFrac - prevFrac);
                        break;
                    }
                    else
                        res += distPoint.weight * (frac - prevFrac);
                }
                prevFrac = frac;
            }
            return res / totalWeight();
        }

        /// @brief returns the lowest normalized height with non-zero weight
        double lowestNormalizedHeight() const
        {
            double prev_height = 0.;
            for (auto distPoint = begin(); distPoint != end(); ++distPoint)
            {
                if (distPoint->weight > 0.)
                    return prev_height / maximumHeight();
                prev_height = distPoint->height;
            }
            return 0.;
        }
    };

    enum class DistributionType
    {
        Weighted,
        TopOfTank,
        BottomOfTank
    };

    struct Distribution
    {
      public:
        DistributionType distributionType;
        WeightedDistribution weightedDistribution;

        Distribution(const DistributionType distribType_in = DistributionType::TopOfTank)
            : distributionType(distribType_in), weightedDistribution({}, {})
        {
        }

        Distribution(const std::vector<double>& heights, const std::vector<double>& weights)
            : distributionType(DistributionType::Weighted), weightedDistribution(heights, weights)
        {
        }

        [[nodiscard]] bool isValid() const
        {
            switch (distributionType)
            {
            case DistributionType::TopOfTank:
            case DistributionType::BottomOfTank:
                return true;

            case DistributionType::Weighted:
                return weightedDistribution.isValid();
            }
            return false;
        }
    };

    struct NodeWeight
    {
        int nodeNum;
        double weight;

        NodeWeight(int n, double w) : nodeNum(n), weight(w) {};

        NodeWeight(int n) : nodeNum(n), weight(1.0) {};
    };

    std::shared_ptr<SoCBasedHeatingLogic> shutOffSoC(std::string desc,
                                                     double targetSoC,
                                                     double hystFract,
                                                     double tempMinUseful_C,
                                                     bool constMains,
                                                     double mains_C);

    std::shared_ptr<SoCBasedHeatingLogic> turnOnSoC(std::string desc,
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
    std::shared_ptr<TempBasedHeatingLogic> topNode(double decisionPoint);
    std::shared_ptr<TempBasedHeatingLogic> bottomNode(double decisionPoint);
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

    /** specifies the type of heat source  */
    enum HEATSOURCE_TYPE
    {
        TYPE_none,       /**< a default to check to make sure it's been set  */
        TYPE_resistance, /**< a resistance element  */
        TYPE_compressor  /**< a vapor cycle compressor  */
    };

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

    static bool getPresetNameFromNumber(std::string& modelName, const hpwh_presets::MODELS model);
    static bool getPresetNumberFromName(const std::string& modelName, hpwh_presets::MODELS& model);

    void configure();

    /// init Preset from embedded CBOR representation
    void initPreset(hpwh_presets::MODELS presetNum);
    void initPreset(const std::string& modelName);

    void initLegacy(hpwh_presets::MODELS presetNum);
    void initLegacy(const std::string& modelName);

    /// init from hpwh-data-model in JSON format
    void initFromJSON(const nlohmann::json& j, const std::string& modelName = "custom");

    void runOneStep(double drawVolume_L,
                    double ambientT_C,
                    double externalT_C,
                    DRMODES DRstatus,
                    double inletVol2_L = 0.,
                    double inletT2_C = 0.,
                    std::vector<double>* extraHeatDist_W = NULL);

    double minutesPerStep = 1.;
    double secondsPerStep, hoursPerStep;

    /// the HeatSources, in order of priority
    std::vector<std::shared_ptr<HeatSource>> heatSources;

    /// the tank
    std::shared_ptr<Tank> tank;

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

    int writeCSVHeading(std::ofstream& outFILE,
                        const char* preamble = "",
                        int nTCouples = 6,
                        int options = CSVOPT_NONE) const;

    int writeCSVRow(std::ofstream& outFILE,
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
    double calcSoCFraction(double tMains_C, double tMinUseful_C) const;

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

    double getFittingsUA_kJperHrC() const;
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

    Condenser* getCompressor();

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

    double getTankVolume_L() const;
    /**< get the tank volume (L) */

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

    /// returns 1 if compressor is external multipass, 0 if compressor is not external multipass
    int isCompressorExternal() const;

    bool hasACompressor() const;
    /// Returns if the HPWH model has a compressor or not, could be a storage or resistance tank.

    /// returns 1 if compressor running, 0 if compressor not running, ABORT if no compressor
    int isCompressorRunning() const;

    bool hasExternalHeatSource(std::size_t& heatSourceIndex) const;
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

    bool isScalable() const;
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
    double getAverageTankTemp_C(const Distribution& dist) const;

    /// returns the tank temperature averaged using weighted logic nodes
    double getAverageTankTemp_C(const WeightedDistribution& wdist) const;

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
    bool isEnergyBalanced(const double drawVol1_L,
                          const double inlet1T_C,
                          const double drawVol2_L,
                          const double inlet2T_C,
                          const double prevHeatContent_kJ,
                          const double fracEnergyTolerance = 0.001);

    /// Overloaded version of above with one inlet only.
    bool isEnergyBalanced(const double drawVol1_L,
                          double inletT_C_in,
                          const double prevHeatContent_kJ,
                          const double fracEnergyTolerance)
    {
        return isEnergyBalanced(
            drawVol1_L, inletT_C_in, 0., 0., prevHeatContent_kJ, fracEnergyTolerance);
    }

    /// Overloaded version of above using current inletT
    bool isEnergyBalanced(const double drawVol_L,
                          const double prevHeatContent_kJ,
                          const double fracEnergyTolerance = 0.001)
    {
        return isEnergyBalanced(
            drawVol_L, member_inletT_C, prevHeatContent_kJ, fracEnergyTolerance);
    }

    /// Addition of heat from a normal heat sources; return excess heat, if needed, to prevent
    /// exceeding maximum or setpoint
    double addHeatAboveNode(double qAdd_kJ, const int nodeNum, const double maxT_C);

    /// Addition of extra heat handled separately from normal heat sources
    void addExtraHeatAboveNode(double qAdd_kJ, const int nodeNum);

    void mixTankInversions();

    /// first-hour rating designations to determine draw pattern for 24-hr test
    struct FirstHourRating
    {
        enum class Designation
        {
            VerySmall,
            Low,
            Medium,
            High
        } designation;

        static inline std::unordered_map<Designation, std::string> DesignationMap = {
            {Designation::VerySmall, "Very Small"},
            {Designation::Low, "Low"},
            {Designation::Medium, "Medium"},
            {Designation::High, "High"}};

        double drawVolume_L;
        std::string report();
    };

    /// fields for test output to csv
    struct TestData
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

    /// collection of information derived from standard 24-h test
    struct TestSummary
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
        double averageOutletT_C = 0.;      // <Tdel,i>
        double averageInletT_C = 0.;       // <Tin,i>

        double usedFossilFuelEnergy_kJ = 0.;               // Q_f
        double usedElectricalEnergy_kJ = 0.;               // Q_e
        double usedEnergy_kJ = 0.;                         // Q
        double consumedHeatingEnergy_kJ = 0.;              // Q_d
        double standardWaterHeatingEnergy_kJ = 0.;         // Q_HW,T
        double adjustedConsumedWaterHeatingEnergy_kJ = 0.; // Q_da
        double modifiedConsumedWaterHeatingEnergy_kJ = 0.; // Q_dm
        double EF = 0.;

        // (calculated) annual totals
        double annualConsumedElectricalEnergy_kJ = 0.; // E_annual,e
        double annualConsumedEnergy_kJ = 0.;           // E_annual

        bool qualifies = false;

        std::vector<TestData> testDataSet = {};

        // return a verbose string summary
        std::string report();
    };

    struct TestConfiguration
    {
        double ambientT_C;
        double inletT_C;
        double externalT_C;
    };

    static TestConfiguration testConfiguration_E50;
    static TestConfiguration testConfiguration_UEF;
    static TestConfiguration testConfiguration_E95;
    static double testSetpointT_C;

    /// perform a draw/heat cycle to prepare for test
    void prepareForTest(const TestConfiguration& test_configuration);

    /// determine first-hour rating
    FirstHourRating findFirstHourRating();

    /// run 24-hr draw pattern
    TestSummary run24hrTest(TestConfiguration testConfiguration,
                            FirstHourRating::Designation designation,
                            bool saveOutput = false);

    TestSummary run24hrTest(TestConfiguration testConfiguration, bool saveOutput = false)
    {
        return run24hrTest(testConfiguration, findFirstHourRating().designation, saveOutput);
    }

    /// specific information for a single draw
    struct Draw
    {
        double startTime_min;
        double volume_L;
        double flowRate_L_per_min;

        Draw(const double startTime_min_in,
             const double volume_L_in,
             const double flowRate_Lper_min_in)
            : startTime_min(startTime_min_in)
            , volume_L(volume_L_in)
            , flowRate_L_per_min(flowRate_Lper_min_in)
        {
        }
    };

    /// sequence of draws in pattern
    typedef std::vector<Draw> DrawPattern;

    static std::unordered_map<FirstHourRating::Designation, std::size_t> firstDrawClusterSizes;

    /// collection of standard draw patterns
    static std::unordered_map<FirstHourRating::Designation, DrawPattern> drawPatterns;

    struct Fitter;

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

        PerformancePoly(double T_F_in,
                        const std::vector<double>& inputPower_coeffs_in,
                        const std::vector<double>& COP_coeffs_in)
            : T_F(T_F_in), inputPower_coeffs(inputPower_coeffs_in), COP_coeffs(COP_coeffs_in)
        {
        }
    };

    struct PerformancePolySet : public std::vector<PerformancePoly>
    {
        PerformancePolySet(const std::vector<PerformancePoly>& vect)
            : std::vector<PerformancePoly>(vect)
        {
        }

        /// pick the nearest temperature index in a PolySet
        int getAmbientT_index(double ambientT_C) const;

        Performance evaluate(double externalT_C, double heatSourceT_C) const;

        inline std::function<Performance(double, double)> make() const
        {
            return [*this](double externalT_C, double heatSourceT_C)
            { return evaluate(externalT_C, heatSourceT_C); };
        }

        inline std::function<Performance(double, double)> use() const
        {
            return [this](double externalT_C, double heatSourceT_C)
            { return evaluate(externalT_C, heatSourceT_C); };
        }
    };

    static const PerformancePolySet tier3, tier4;

    void makeCondenserPerformance(const PerformancePolySet& perfPolySet);

    ///
    struct PerformancePoly_CWHS_SP : public PerformancePoly
    {
        PerformancePoly_CWHS_SP(const PerformancePoly& perfPoly) : PerformancePoly(perfPoly) {}

        PerformancePoly_CWHS_SP(double T_F_in,
                                const std::vector<double>& inputPower_coeffs_in,
                                const std::vector<double>& COP_coeffs_in)
            : PerformancePoly(T_F_in, inputPower_coeffs_in, COP_coeffs_in)
        {
        }

        std::function<Performance(double, double)> make(Condenser* condenser) const;
    };

    void makeCondenserPerformance(const PerformancePoly_CWHS_SP& perfPoly_cwhs_sp);

    ///
    struct PerformancePoly_CWHS_MP : public PerformancePoly
    {
        PerformancePoly_CWHS_MP(const PerformancePoly& perfPoly) : PerformancePoly(perfPoly) {}

        PerformancePoly_CWHS_MP(double T_F_in,
                                const std::vector<double>& inputPower_coeffs_in,
                                const std::vector<double>& COP_coeffs_in)
            : PerformancePoly(T_F_in, inputPower_coeffs_in, COP_coeffs_in)
        {
        }

        std::function<Performance(double, double)> make() const;
    };

    void makeCondenserPerformance(const PerformancePoly_CWHS_MP& perfPoly_cwhs_mp);

    /// fit using a single configuration
    TestSummary makeGenericEF(double targetEF,
                              TestConfiguration testConfiguration,
                              FirstHourRating::Designation designation,
                              PerformancePolySet& perfPolySet);
    TestSummary makeGenericEF(double targetEF,
                              TestConfiguration testConfiguration,
                              PerformancePolySet& perfPolySet)
    {
        return makeGenericEF(
            targetEF, testConfiguration, findFirstHourRating().designation, perfPolySet);
    }
    TestSummary makeGenericEF(double targetEF,
                              TestConfiguration testConfiguration,
                              FirstHourRating::Designation designation,
                              const PerformancePolySet& perfPolySet)
    {
        PerformancePolySet perfCopy = perfPolySet;
        return makeGenericEF(targetEF, testConfiguration, designation, perfCopy);
    }
    TestSummary makeGenericEF(double targetEF,
                              TestConfiguration testConfiguration,
                              const PerformancePolySet& perfPolySet)
    {
        return makeGenericEF(
            targetEF, testConfiguration, findFirstHourRating().designation, perfPolySet);
    }

    /// fit using each of three configurations, independently
    void makeGenericE50_UEF_E95(double targetE50,
                                double targetUEF,
                                double targetE95,
                                FirstHourRating::Designation designation,
                                PerformancePolySet& perfPolySet);
    void makeGenericE50_UEF_E95(double targetE50,
                                double targetUEF,
                                double targetE95,
                                PerformancePolySet& perfPolySet)
    {
        return makeGenericE50_UEF_E95(
            targetE50, targetUEF, targetE95, findFirstHourRating().designation, perfPolySet);
    }
    void makeGenericE50_UEF_E95(double targetE50,
                                double targetUEF,
                                double targetE95,
                                FirstHourRating::Designation designation,
                                const PerformancePolySet& perfPolySet)
    {
        PerformancePolySet perfCopy = perfPolySet;
        return makeGenericE50_UEF_E95(targetE50, targetUEF, targetE95, designation, perfCopy);
    }
    void makeGenericE50_UEF_E95(double targetE50,
                                double targetUEF,
                                double targetE95,
                                const PerformancePolySet& perfPolySet)
    {
        return makeGenericE50_UEF_E95(
            targetE50, targetUEF, targetE95, findFirstHourRating().designation, perfPolySet);
    }

    /// fit using UEF config, then adjust E50, E95 coefficients
    TestSummary makeGenericUEF(double targetUEF,
                               FirstHourRating::Designation designation,
                               PerformancePolySet& perfPolySet);
    TestSummary makeGenericUEF(double targetUEF, PerformancePolySet& perfPolySet)
    {
        return makeGenericUEF(targetUEF, findFirstHourRating().designation, perfPolySet);
    }
    TestSummary makeGenericUEF(double targetUEF,
                               FirstHourRating::Designation designation,
                               const PerformancePolySet& perfPolySet)
    {
        PerformancePolySet perfCopy = perfPolySet;
        return makeGenericUEF(targetUEF, designation, perfCopy);
    }
    TestSummary makeGenericUEF(double targetUEF, const PerformancePolySet& perfPolySet)
    {
        return makeGenericUEF(targetUEF, findFirstHourRating().designation, perfPolySet);
    }

    static void linearInterp(double& ynew, double xnew, double x0, double x1, double y0, double y1);

  private:
    void setAllDefaults(); /**< sets all the defaults */

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

    void calcAndSetSoCFraction();

    bool isHeating;
    /**< is the hpwh currently heating or not?  */

    bool setpointFixed;
    /**< does the HPWH allow the setpoint to vary  */

    bool canScale;
    /**< can the HPWH scale capactiy and COP or not  */

    Condenser* addCondenser(const std::string& name_in);
    Resistance* addResistance(const std::string& name_in);

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

    double currentSoCFraction;
    /**< the current state of charge according to the logic */

    double setpoint_C;
    /**< the setpoint of the tank  */

    DRMODES prevDRstatus;
    /**< the DRstatus of the tank in the previous time step and at the end of runOneStep */

    double timerLimitTOT;
    /**< the time limit in minutes on the timer when the compressor and resistance elements are
     * turned back on, used with DR_TOT. */
    double timerTOT;
    /**< the timer used for DR_TOT to turn on the compressor and resistance elements. */

    bool usesSoCLogic;

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

    struct resPoint
    {
        int index;
        int position;
    };
    std::vector<resPoint> resistanceHeightMap;
    /**< A map from index of an resistance element in heatSources to position in the tank, its
    is sorted by height from lowest to highest*/

    /// Generates a top-hat distribution
    Distribution getRangeDistribution(double bottomFraction, double topFraction);

  public:
    static double getResampledValue(const std::vector<double>& sampleValues,
                                    double beginFraction,
                                    double endFraction);

    static void resample(std::vector<double>& values, const std::vector<double>& sampleValues);

    static void resampleExtensive(std::vector<double>& values,
                                  const std::vector<double>& sampleValues);

    static inline void resampleIntensive(std::vector<double>& values,
                                         const std::vector<double>& sampleValues)
    {
        resample(values, sampleValues);
    }

    static double expitFunc(double x, double offset);

    static void normalize(std::vector<double>& distribution);

    static int findLowestNode(const WeightedDistribution& wdist, const int numTankNodes);

    static double findShrinkageT_C(const WeightedDistribution& wDist, const int numTankNodes);

    static void calcThermalDist(std::vector<double>& thermalDist,
                                const double shrinkageT_C,
                                const int lowestNode,
                                const std::vector<double>& nodeT_C,
                                const double setpointT_C);

    static void scaleVector(std::vector<double>& coeffs, const double scaleFactor);

    static double getChargePerNode(double tCold, double tMix, double tHot);
}; // end of HPWH class

constexpr double BTUperKWH =
    3412.14163312794;             // https://www.rapidtables.com/convert/energy/kWh_to_BTU.html
constexpr double FperC = 9. / 5.; // degF / degC
constexpr double offsetF = 32.;   // degF offset
constexpr double absolute_zeroT_C = -273.15;            // absolute zero (degC)
constexpr double sec_per_min = 60.;                     // s / min
constexpr double min_per_hr = 60.;                      // min / hr
constexpr double sec_per_hr = sec_per_min * min_per_hr; // s / hr
constexpr double L_per_gal = 3.78541;                   // liters / gal
constexpr double ft_per_m = 3.2808;                     // ft / m
constexpr double ft2_per_m2 = ft_per_m * ft_per_m;      // ft^2 / m^2
constexpr double BTUm2C_per_kWhft2F =
    BTUperKWH / ft2_per_m2 / FperC; // BTU m^2 degC / kW h ft^2 degC

// a few extra functions for unit conversion
inline double dF_TO_dC(double temperature) { return (temperature / FperC); }
inline double dC_TO_dF(double temperature) { return (FperC * temperature); }
inline double F_TO_C(double temperature) { return ((temperature - offsetF) / FperC); }
inline double C_TO_F(double temperature) { return ((FperC * temperature) + offsetF); }
inline double K_TO_C(double kelvin) { return (kelvin + absolute_zeroT_C); }
inline double C_TO_K(double C) { return (C - absolute_zeroT_C); }
inline double K_TO_F(double K) { return C_TO_F(K_TO_C(K)); }
inline double F_TO_K(double F) { return C_TO_K(F_TO_C(F)); }
inline double KWH_TO_BTU(double kwh) { return (BTUperKWH * kwh); }
inline double KWH_TO_KJ(double kwh) { return (kwh * sec_per_hr); }
inline double BTU_TO_KWH(double btu) { return (btu / BTUperKWH); }
inline double BTUperH_TO_KW(double btu) { return (btu / BTUperKWH); }
inline double BTUperH_TO_W(double btu) { return (1000. * btu / BTUperKWH); }
inline double KW_TO_W(double kw) { return 1000. * kw; }
inline double W_TO_KW(double w) { return w / 1000.; }
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
inline double FT2_TO_M2(double feet2) { return FT_TO_M(FT_TO_M(feet2)); }

inline double M_TO_FT(double m) { return (ft_per_m * m); }
inline double M2_TO_FT2(double m2) { return M_TO_FT(M_TO_FT(m2)); }

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

inline std::string getModelNameFromFilename(const std::string& modelFilename)
{
    std::string modelName = "custom";
    if (modelFilename.find("/") != std::string::npos)
    {
        std::size_t iLast = modelFilename.find_last_of("/");
        modelName = modelFilename.substr(iLast + 1);
    }
    if (modelName.find(".") != std::string::npos)
    {
        std::size_t iLast = modelName.find_last_of(".");
        modelName = modelName.substr(0, iLast);
    }
    return modelName;
}

#endif
