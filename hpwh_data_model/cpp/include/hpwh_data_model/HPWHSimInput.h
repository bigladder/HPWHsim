#ifndef HPWH_SIM_INPUT_H_
#define HPWH_SIM_INPUT_H_
#include <ASHRAE205.h>
#include <RSINTEGRATEDWATERHEATER.h>
#include <CentralWaterHeatingSystem.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model
{
namespace hpwh_sim_input
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
    bool number_of_nodes_is_set;
    const static std::string_view number_of_nodes_units;
    const static std::string_view number_of_nodes_description;
    const static std::string_view number_of_nodes_name;
    bool depresses_temperature;
    bool depresses_temperature_is_set;
    const static std::string_view depresses_temperature_units;
    const static std::string_view depresses_temperature_description;
    const static std::string_view depresses_temperature_name;
    bool fixed_volume;
    bool fixed_volume_is_set;
    const static std::string_view fixed_volume_units;
    const static std::string_view fixed_volume_description;
    const static std::string_view fixed_volume_name;
    double standard_setpoint;
    bool standard_setpoint_is_set;
    const static std::string_view standard_setpoint_units;
    const static std::string_view standard_setpoint_description;
    const static std::string_view standard_setpoint_name;
    hpwh_sim_input::HPWHSystemType system_type;
    bool system_type_is_set;
    const static std::string_view system_type_units;
    const static std::string_view system_type_description;
    const static std::string_view system_type_name;
    rsintegratedwaterheater::RSINTEGRATEDWATERHEATER integrated_system;
    bool integrated_system_is_set;
    const static std::string_view integrated_system_units;
    const static std::string_view integrated_system_description;
    const static std::string_view integrated_system_name;
    central_water_heating_system::CentralWaterHeatingSystem central_system;
    bool central_system_is_set;
    const static std::string_view central_system_units;
    const static std::string_view central_system_description;
    const static std::string_view central_system_name;
};
NLOHMANN_JSON_SERIALIZE_ENUM(HPWHSystemType,
                             {{HPWHSystemType::UNKNOWN, "UNKNOWN"},
                              {HPWHSystemType::INTEGRATED, "INTEGRATED"},
                              {HPWHSystemType::CENTRAL, "CENTRAL"}})
void from_json(const nlohmann::json& j, HPWHSimInput& x);
} // namespace hpwh_sim_input
} // namespace hpwh_data_model
#endif
