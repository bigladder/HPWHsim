#ifndef RSINTEGRATEDWATERHEATER_H_
#define RSINTEGRATEDWATERHEATER_H_
#include <ashrae205.h>
#include <rstank.h>
#include <rsresistancewaterheatsource.h>
#include <rscondenserwaterheatsource.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <heat-source-base.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model
{
namespace rsintegratedwaterheater_ns
{
enum class HeatSourceType
{
    RESISTANCE,
    CONDENSER,
    UNKNOWN
};
const static std::unordered_map<HeatSourceType, enum_info> HeatSourceType_info {
    {HeatSourceType::RESISTANCE,
     {"RESISTANCE", "Resistance", "Heat sources that operate by electrical resistance"}},
    {HeatSourceType::CONDENSER,
     {"CONDENSER", "Condenser", "Heat sources that operate by coolant condenser systems"}},
    {HeatSourceType::UNKNOWN, {"UNKNOWN", "None", "None"}}};
enum class ComparisonType
{
    GREATER_THAN,
    LESS_THAN,
    UNKNOWN
};
const static std::unordered_map<ComparisonType, enum_info> ComparisonType_info {
    {ComparisonType::GREATER_THAN,
     {"GREATER_THAN", "Greater than", "Decision value is greater than reference value"}},
    {ComparisonType::LESS_THAN,
     {"LESS_THAN", "Less than", "Decision value is less than reference value"}},
    {ComparisonType::UNKNOWN, {"UNKNOWN", "None", "None"}}};
struct Schema
{
    const static std::string_view schema_title;
    const static std::string_view schema_version;
    const static std::string_view schema_description;
};
struct ProductInformation
{
    std::string manufacturer;
    std::string model_number;
    bool manufacturer_is_set;
    bool model_number_is_set;
    const static std::string_view manufacturer_units;
    const static std::string_view model_number_units;
    const static std::string_view manufacturer_description;
    const static std::string_view model_number_description;
    const static std::string_view manufacturer_name;
    const static std::string_view model_number_name;
};
struct Description
{
    rsintegratedwaterheater_ns::ProductInformation product_information;
    bool product_information_is_set;
    const static std::string_view product_information_units;
    const static std::string_view product_information_description;
    const static std::string_view product_information_name;
};
struct HeatingLogic
{
    double absolute_temperature;
    double differential_temperature;
    std::vector<double> logic_distribution;
    rsintegratedwaterheater_ns::ComparisonType comparison_type;
    double hysteresis_temperature;
    bool absolute_temperature_is_set;
    bool differential_temperature_is_set;
    bool logic_distribution_is_set;
    bool comparison_type_is_set;
    bool hysteresis_temperature_is_set;
    const static std::string_view absolute_temperature_units;
    const static std::string_view differential_temperature_units;
    const static std::string_view logic_distribution_units;
    const static std::string_view comparison_type_units;
    const static std::string_view hysteresis_temperature_units;
    const static std::string_view absolute_temperature_description;
    const static std::string_view differential_temperature_description;
    const static std::string_view logic_distribution_description;
    const static std::string_view comparison_type_description;
    const static std::string_view hysteresis_temperature_description;
    const static std::string_view absolute_temperature_name;
    const static std::string_view differential_temperature_name;
    const static std::string_view logic_distribution_name;
    const static std::string_view comparison_type_name;
    const static std::string_view hysteresis_temperature_name;
};
struct HeatSourceConfiguration
{
    rsintegratedwaterheater_ns::HeatSourceType heat_source_type;
    std::unique_ptr<HeatSourceBase> heat_source;
    std::vector<double> heat_distribution;
    std::vector<rsintegratedwaterheater_ns::HeatingLogic> turn_on_logic;
    std::vector<rsintegratedwaterheater_ns::HeatingLogic> shut_off_logic;
    std::vector<rsintegratedwaterheater_ns::HeatingLogic> standby_logic;
    double maximum_setpoint;
    bool heat_source_type_is_set;
    bool heat_source_is_set;
    bool heat_distribution_is_set;
    bool turn_on_logic_is_set;
    bool shut_off_logic_is_set;
    bool standby_logic_is_set;
    bool maximum_setpoint_is_set;
    const static std::string_view heat_source_type_units;
    const static std::string_view heat_source_units;
    const static std::string_view heat_distribution_units;
    const static std::string_view turn_on_logic_units;
    const static std::string_view shut_off_logic_units;
    const static std::string_view standby_logic_units;
    const static std::string_view maximum_setpoint_units;
    const static std::string_view heat_source_type_description;
    const static std::string_view heat_source_description;
    const static std::string_view heat_distribution_description;
    const static std::string_view turn_on_logic_description;
    const static std::string_view shut_off_logic_description;
    const static std::string_view standby_logic_description;
    const static std::string_view maximum_setpoint_description;
    const static std::string_view heat_source_type_name;
    const static std::string_view heat_source_name;
    const static std::string_view heat_distribution_name;
    const static std::string_view turn_on_logic_name;
    const static std::string_view shut_off_logic_name;
    const static std::string_view standby_logic_name;
    const static std::string_view maximum_setpoint_name;
};
struct Performance
{
    rstank_ns::RSTANK tank;
    std::vector<rsintegratedwaterheater_ns::HeatSourceConfiguration> heat_source_configurations;
    double standby_power;
    bool tank_is_set;
    bool heat_source_configurations_is_set;
    bool standby_power_is_set;
    const static std::string_view tank_units;
    const static std::string_view heat_source_configurations_units;
    const static std::string_view standby_power_units;
    const static std::string_view tank_description;
    const static std::string_view heat_source_configurations_description;
    const static std::string_view standby_power_description;
    const static std::string_view tank_name;
    const static std::string_view heat_source_configurations_name;
    const static std::string_view standby_power_name;
};
struct RSINTEGRATEDWATERHEATER
{
    core_ns::Metadata metadata;
    rsintegratedwaterheater_ns::Description description;
    rsintegratedwaterheater_ns::Performance performance;
    double standby_power;
    bool metadata_is_set;
    bool description_is_set;
    bool performance_is_set;
    bool standby_power_is_set;
    const static std::string_view metadata_units;
    const static std::string_view description_units;
    const static std::string_view performance_units;
    const static std::string_view standby_power_units;
    const static std::string_view metadata_description;
    const static std::string_view description_description;
    const static std::string_view performance_description;
    const static std::string_view standby_power_description;
    const static std::string_view metadata_name;
    const static std::string_view description_name;
    const static std::string_view performance_name;
    const static std::string_view standby_power_name;
};
NLOHMANN_JSON_SERIALIZE_ENUM(HeatSourceType,
                             {
                                 {HeatSourceType::UNKNOWN, "UNKNOWN"},
                                 {HeatSourceType::RESISTANCE, "RESISTANCE"},
                                 {HeatSourceType::CONDENSER, "CONDENSER"},
                             })
NLOHMANN_JSON_SERIALIZE_ENUM(ComparisonType,
                             {
                                 {ComparisonType::UNKNOWN, "UNKNOWN"},
                                 {ComparisonType::GREATER_THAN, "GREATER_THAN"},
                                 {ComparisonType::LESS_THAN, "LESS_THAN"},
                             })
void from_json(const nlohmann::json& j, RSINTEGRATEDWATERHEATER& x);
void from_json(const nlohmann::json& j, Description& x);
void from_json(const nlohmann::json& j, ProductInformation& x);
void from_json(const nlohmann::json& j, Performance& x);
void from_json(const nlohmann::json& j, HeatSourceConfiguration& x);
void from_json(const nlohmann::json& j, HeatingLogic& x);
} // namespace rsintegratedwaterheater_ns
} // namespace hpwh_data_model
#endif
