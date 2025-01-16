#ifndef HPWH_SIM_INPUT_H_
#define HPWH_SIM_INPUT_H_
#include <ashrae205.h>
#include <rsintegratedwaterheater.h>
#include <centralwaterheatingsystem.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <hpwhsiminput.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model
{
namespace hpwh_sim_input_ns
{
enum class HPWHSystemType
{
    INTEGRATED,
    CENTRAL,
    UNKNOWN
};
const static std::unordered_map<HPWHSystemType, enum_info> HPWHSystemType_info {
    {HPWHSystemType::INTEGRATED, {"INTEGRATED", "Integrated", "Integrated Heat Pump Water Heater"}},
    {HPWHSystemType::CENTRAL, {"CENTRAL", "Central", "Integrated Heat Pump Water Heater"}},
    {HPWHSystemType::UNKNOWN, {"UNKNOWN", "None", "None"}}};
inline std::shared_ptr<Courier::Courier> logger;
void set_logger(std::shared_ptr<Courier::Courier> value);
struct Schema
{
    const static std::string_view schema_title;
    const static std::string_view schema_version;
    const static std::string_view schema_description;
};
struct HPWHSimInput
{
    int number_of_nodes;
    bool depresses_temperature;
    bool fixed_volume;
    double standard_setpoint;
    hpwh_sim_input_ns::HPWHSystemType system_type;
    rsintegratedwaterheater_ns::RSINTEGRATEDWATERHEATER integrated_system;
    central_water_heating_system_ns::CentralWaterHeatingSystem central_system;
    bool number_of_nodes_is_set;
    bool depresses_temperature_is_set;
    bool fixed_volume_is_set;
    bool standard_setpoint_is_set;
    bool system_type_is_set;
    bool integrated_system_is_set;
    bool central_system_is_set;
    const static std::string_view number_of_nodes_units;
    const static std::string_view depresses_temperature_units;
    const static std::string_view fixed_volume_units;
    const static std::string_view standard_setpoint_units;
    const static std::string_view system_type_units;
    const static std::string_view integrated_system_units;
    const static std::string_view central_system_units;
    const static std::string_view number_of_nodes_description;
    const static std::string_view depresses_temperature_description;
    const static std::string_view fixed_volume_description;
    const static std::string_view standard_setpoint_description;
    const static std::string_view system_type_description;
    const static std::string_view integrated_system_description;
    const static std::string_view central_system_description;
    const static std::string_view number_of_nodes_name;
    const static std::string_view depresses_temperature_name;
    const static std::string_view fixed_volume_name;
    const static std::string_view standard_setpoint_name;
    const static std::string_view system_type_name;
    const static std::string_view integrated_system_name;
    const static std::string_view central_system_name;
};
NLOHMANN_JSON_SERIALIZE_ENUM(HPWHSystemType,
                             {
                                 {HPWHSystemType::UNKNOWN, "UNKNOWN"},
                                 {HPWHSystemType::INTEGRATED, "INTEGRATED"},
                                 {HPWHSystemType::CENTRAL, "CENTRAL"},
                             })
void from_json(const nlohmann::json& j, HPWHSimInput& x);
} // namespace hpwh_sim_input_ns
} // namespace hpwh_data_model
#endif
