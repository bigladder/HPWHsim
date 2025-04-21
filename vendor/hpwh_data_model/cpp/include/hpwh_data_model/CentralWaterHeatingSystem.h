#ifndef CENTRAL_WATER_HEATING_SYSTEM_H_
#define CENTRAL_WATER_HEATING_SYSTEM_H_
#include <RSTANK.h>
#include <HeatSourceConfiguration.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model {
	namespace central_water_heating_system {
		enum class ControlType {
			FIXED_FLOW_RATE,
			FIXED_OUTLET_TEMPERATURE,
			UNKNOWN
		};
		const static std::unordered_map<ControlType, enum_info> ControlType_info {
			{ControlType::FIXED_FLOW_RATE, {"FIXED_FLOW_RATE", "FIXED_FLOW_RATE", "Fixed Flow Rate"}},
			{ControlType::FIXED_OUTLET_TEMPERATURE, {"FIXED_OUTLET_TEMPERATURE", "FIXED_OUTLET_TEMPERATURE", "Fixed Outlet Temperature"}},
			{ControlType::UNKNOWN, {"UNKNOWN", "None", "None"}}
		};
		inline std::shared_ptr<Courier::Courier> logger;
		void set_logger(std::shared_ptr<Courier::Courier> value);
		struct Schema {
			const static std::string_view schema_title;
			const static std::string_view schema_version;
			const static std::string_view schema_description;
		};
		struct SecondaryHeatExchanger {
			double cold_side_temperature_offset;
			bool cold_side_temperature_offset_is_set = false;
			const static std::string_view cold_side_temperature_offset_units;
			const static std::string_view cold_side_temperature_offset_description;
			const static std::string_view cold_side_temperature_offset_name;
			double hot_side_temperature_offset;
			bool hot_side_temperature_offset_is_set = false;
			const static std::string_view hot_side_temperature_offset_units;
			const static std::string_view hot_side_temperature_offset_description;
			const static std::string_view hot_side_temperature_offset_name;
			double extra_pump_power;
			bool extra_pump_power_is_set = false;
			const static std::string_view extra_pump_power_units;
			const static std::string_view extra_pump_power_description;
			const static std::string_view extra_pump_power_name;
		};
		struct CentralWaterHeatingSystem {
			rstank::RSTANK tank;
			bool tank_is_set = false;
			const static std::string_view tank_units;
			const static std::string_view tank_description;
			const static std::string_view tank_name;
			std::vector<heat_source_configuration::HeatSourceConfiguration> heat_source_configurations;
			bool heat_source_configurations_is_set = false;
			const static std::string_view heat_source_configurations_units;
			const static std::string_view heat_source_configurations_description;
			const static std::string_view heat_source_configurations_name;
			std::string primary_heat_source_id;
			bool primary_heat_source_id_is_set = false;
			const static std::string_view primary_heat_source_id_units;
			const static std::string_view primary_heat_source_id_description;
			const static std::string_view primary_heat_source_id_name;
			double standby_power;
			bool standby_power_is_set = false;
			const static std::string_view standby_power_units;
			const static std::string_view standby_power_description;
			const static std::string_view standby_power_name;
			double external_inlet_height;
			bool external_inlet_height_is_set = false;
			const static std::string_view external_inlet_height_units;
			const static std::string_view external_inlet_height_description;
			const static std::string_view external_inlet_height_name;
			double external_outlet_height;
			bool external_outlet_height_is_set = false;
			const static std::string_view external_outlet_height_units;
			const static std::string_view external_outlet_height_description;
			const static std::string_view external_outlet_height_name;
			central_water_heating_system::ControlType control_type;
			bool control_type_is_set = false;
			const static std::string_view control_type_units;
			const static std::string_view control_type_description;
			const static std::string_view control_type_name;
			double fixed_flow_rate;
			bool fixed_flow_rate_is_set = false;
			const static std::string_view fixed_flow_rate_units;
			const static std::string_view fixed_flow_rate_description;
			const static std::string_view fixed_flow_rate_name;
			central_water_heating_system::SecondaryHeatExchanger secondary_heat_exchanger;
			bool secondary_heat_exchanger_is_set = false;
			const static std::string_view secondary_heat_exchanger_units;
			const static std::string_view secondary_heat_exchanger_description;
			const static std::string_view secondary_heat_exchanger_name;
		};
		NLOHMANN_JSON_SERIALIZE_ENUM (ControlType, {
			{ControlType::UNKNOWN, "UNKNOWN"},
			{ControlType::FIXED_FLOW_RATE, "FIXED_FLOW_RATE"},
			{ControlType::FIXED_OUTLET_TEMPERATURE, "FIXED_OUTLET_TEMPERATURE"}
		})
		void from_json(const nlohmann::json& j, CentralWaterHeatingSystem& x);
		void from_json(const nlohmann::json& j, SecondaryHeatExchanger& x);
	}
}
#endif
