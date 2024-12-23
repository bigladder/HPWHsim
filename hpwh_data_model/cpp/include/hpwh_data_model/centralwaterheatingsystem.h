#ifndef CENTRAL_WATER_HEATING_SYSTEM_H_
#define CENTRAL_WATER_HEATING_SYSTEM_H_
#include <rstank.h>
#include <heatsourceconfiguration.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model {
	namespace central_water_heating_system_ns {
		inline std::shared_ptr<Courier::Courier> logger;
		void set_logger(std::shared_ptr<Courier::Courier> value);
		struct Schema {
			const static std::string_view schema_title;
			const static std::string_view schema_version;
			const static std::string_view schema_description;
		};
		struct CentralWaterHeatingSystem {
			rstank_ns::RSTANK tank;
			std::vector<heat_source_configuration_ns::HeatSourceConfiguration> heat_source_configurations;
			std::string primary_heat_source_id;
			double standby_power;
			double external_inlet_height;
			double external_outlet_height;
			double multipass_flow_rate;
			bool tank_is_set;
			bool heat_source_configurations_is_set;
			bool primary_heat_source_id_is_set;
			bool standby_power_is_set;
			bool external_inlet_height_is_set;
			bool external_outlet_height_is_set;
			bool multipass_flow_rate_is_set;
			const static std::string_view tank_units;
			const static std::string_view heat_source_configurations_units;
			const static std::string_view primary_heat_source_id_units;
			const static std::string_view standby_power_units;
			const static std::string_view external_inlet_height_units;
			const static std::string_view external_outlet_height_units;
			const static std::string_view multipass_flow_rate_units;
			const static std::string_view tank_description;
			const static std::string_view heat_source_configurations_description;
			const static std::string_view primary_heat_source_id_description;
			const static std::string_view standby_power_description;
			const static std::string_view external_inlet_height_description;
			const static std::string_view external_outlet_height_description;
			const static std::string_view multipass_flow_rate_description;
			const static std::string_view tank_name;
			const static std::string_view heat_source_configurations_name;
			const static std::string_view primary_heat_source_id_name;
			const static std::string_view standby_power_name;
			const static std::string_view external_inlet_height_name;
			const static std::string_view external_outlet_height_name;
			const static std::string_view multipass_flow_rate_name;
		};
		void from_json(const nlohmann::json& j, CentralWaterHeatingSystem& x);
	}
}
#endif
