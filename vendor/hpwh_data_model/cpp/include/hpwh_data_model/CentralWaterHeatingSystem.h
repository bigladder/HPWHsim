#ifndef CENTRAL_WATER_HEATING_SYSTEM_H_
#define CENTRAL_WATER_HEATING_SYSTEM_H_
#include <RSTANK.h>
#include <HeatSourceConfiguration.h>
#include <ASHRAE205.h>
#include <RSRESISTANCEWATERHEATSOURCE.h>
#include <RSCONDENSERWATERHEATSOURCE.h>
#include <RSAIRTOWATERHEATPUMP.h>
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
			static constexpr std::string_view schema_title = "Central Water Heating System";
			static constexpr std::string_view schema_version = "0.1.0";
			static constexpr std::string_view schema_description = "Components that make up a central water heating system";
		};
		struct SecondaryHeatExchanger {
			double cold_side_temperature_offset;
			bool cold_side_temperature_offset_is_set = false;
			static constexpr std::string_view cold_side_temperature_offset_units = "K";
			static constexpr std::string_view cold_side_temperature_offset_description = "Cold-side offset temperature";
			static constexpr std::string_view cold_side_temperature_offset_name = "cold_side_temperature_offset";
			double hot_side_temperature_offset;
			bool hot_side_temperature_offset_is_set = false;
			static constexpr std::string_view hot_side_temperature_offset_units = "K";
			static constexpr std::string_view hot_side_temperature_offset_description = "Hot-side offset temperature";
			static constexpr std::string_view hot_side_temperature_offset_name = "hot_side_temperature_offset";
			double extra_pump_power;
			bool extra_pump_power_is_set = false;
			static constexpr std::string_view extra_pump_power_units = "W";
			static constexpr std::string_view extra_pump_power_description = "Extra pump power";
			static constexpr std::string_view extra_pump_power_name = "extra_pump_power";
		};
		struct CentralWaterHeatingSystem {
			rstank::RSTANK tank;
			bool tank_is_set = false;
			static constexpr std::string_view tank_units = "";
			static constexpr std::string_view tank_description = "The corresponding Standard 205 tank representation";
			static constexpr std::string_view tank_name = "tank";
			std::vector<heat_source_configuration::HeatSourceConfiguration> heat_source_configurations;
			bool heat_source_configurations_is_set = false;
			static constexpr std::string_view heat_source_configurations_units = "";
			static constexpr std::string_view heat_source_configurations_description = "Describes how the heat sources are configured within the tank";
			static constexpr std::string_view heat_source_configurations_name = "heat_source_configurations";
			std::string primary_heat_source_id;
			bool primary_heat_source_id_is_set = false;
			static constexpr std::string_view primary_heat_source_id_units = "";
			static constexpr std::string_view primary_heat_source_id_description = "Turns on independently of other heat sources";
			static constexpr std::string_view primary_heat_source_id_name = "primary_heat_source_id";
			double standby_power;
			bool standby_power_is_set = false;
			static constexpr std::string_view standby_power_units = "";
			static constexpr std::string_view standby_power_description = "Power drawn when system is in standby mode";
			static constexpr std::string_view standby_power_name = "standby_power";
			double external_inlet_height;
			bool external_inlet_height_is_set = false;
			static constexpr std::string_view external_inlet_height_units = "";
			static constexpr std::string_view external_inlet_height_description = "Fractional tank height of inlet";
			static constexpr std::string_view external_inlet_height_name = "external_inlet_height";
			double external_outlet_height;
			bool external_outlet_height_is_set = false;
			static constexpr std::string_view external_outlet_height_units = "";
			static constexpr std::string_view external_outlet_height_description = "Fractional tank height of outlet";
			static constexpr std::string_view external_outlet_height_name = "external_outlet_height";
			central_water_heating_system::ControlType control_type;
			bool control_type_is_set = false;
			static constexpr std::string_view control_type_units = "";
			static constexpr std::string_view control_type_description = "Control type";
			static constexpr std::string_view control_type_name = "control_type";
			double fixed_flow_rate;
			bool fixed_flow_rate_is_set = false;
			static constexpr std::string_view fixed_flow_rate_units = "m3/s";
			static constexpr std::string_view fixed_flow_rate_description = "Flow rate, if multipass system";
			static constexpr std::string_view fixed_flow_rate_name = "fixed_flow_rate";
			central_water_heating_system::SecondaryHeatExchanger secondary_heat_exchanger;
			bool secondary_heat_exchanger_is_set = false;
			static constexpr std::string_view secondary_heat_exchanger_units = "";
			static constexpr std::string_view secondary_heat_exchanger_description = "Secondary heat exchanger";
			static constexpr std::string_view secondary_heat_exchanger_name = "secondary_heat_exchanger";
		};
		NLOHMANN_JSON_SERIALIZE_ENUM (ControlType, {
			{ControlType::UNKNOWN, "UNKNOWN"},
			{ControlType::FIXED_FLOW_RATE, "FIXED_FLOW_RATE"},
			{ControlType::FIXED_OUTLET_TEMPERATURE, "FIXED_OUTLET_TEMPERATURE"}
		})
		void from_json(const nlohmann::json& j, CentralWaterHeatingSystem& x);
		void to_json(nlohmann::json& j, const CentralWaterHeatingSystem& x);
		void from_json(const nlohmann::json& j, SecondaryHeatExchanger& x);
		void to_json(nlohmann::json& j, const SecondaryHeatExchanger& x);
	}
}
#endif
