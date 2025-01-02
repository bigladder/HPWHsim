#include <centralwaterheatingsystem.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace central_water_heating_system_ns  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		const std::string_view Schema::schema_title = "Central Water Heating System";

		const std::string_view Schema::schema_version = "0.1.0";

		const std::string_view Schema::schema_description = "Components that make up a central water heating system";

		void from_json(const nlohmann::json& j, CentralWaterHeatingSystem& x) {
			json_get<rstank_ns::RSTANK>(j, logger.get(), "tank", x.tank, x.tank_is_set, true);
			json_get<std::vector<heat_source_configuration_ns::HeatSourceConfiguration>>(j, logger.get(), "heat_source_configurations", x.heat_source_configurations, x.heat_source_configurations_is_set, false);
			json_get<std::string>(j, logger.get(), "primary_heat_source_id", x.primary_heat_source_id, x.primary_heat_source_id_is_set, false);
			json_get<double>(j, logger.get(), "standby_power", x.standby_power, x.standby_power_is_set, false);
			json_get<double>(j, logger.get(), "external_inlet_height", x.external_inlet_height, x.external_inlet_height_is_set, false);
			json_get<double>(j, logger.get(), "external_outlet_height", x.external_outlet_height, x.external_outlet_height_is_set, false);
			json_get<double>(j, logger.get(), "multipass_flow_rate", x.multipass_flow_rate, x.multipass_flow_rate_is_set, false);
		}
		const std::string_view CentralWaterHeatingSystem::tank_units = "";

		const std::string_view CentralWaterHeatingSystem::heat_source_configurations_units = "";

		const std::string_view CentralWaterHeatingSystem::primary_heat_source_id_units = "";

		const std::string_view CentralWaterHeatingSystem::standby_power_units = "";

		const std::string_view CentralWaterHeatingSystem::external_inlet_height_units = "";

		const std::string_view CentralWaterHeatingSystem::external_outlet_height_units = "";

		const std::string_view CentralWaterHeatingSystem::multipass_flow_rate_units = "";

		const std::string_view CentralWaterHeatingSystem::tank_description = "The corresponding Standard 205 tank representation";

		const std::string_view CentralWaterHeatingSystem::heat_source_configurations_description = "Describes how the heat sources are configured within the tank";

		const std::string_view CentralWaterHeatingSystem::primary_heat_source_id_description = "Turns on independently of other heat sources";

		const std::string_view CentralWaterHeatingSystem::standby_power_description = "Power drawn when system is in standby mode";

		const std::string_view CentralWaterHeatingSystem::external_inlet_height_description = "Fractional tank height of inlet";

		const std::string_view CentralWaterHeatingSystem::external_outlet_height_description = "Fractional tank height of outlet";

		const std::string_view CentralWaterHeatingSystem::multipass_flow_rate_description = "Flow rate, if multipass system";

		const std::string_view CentralWaterHeatingSystem::tank_name = "tank";

		const std::string_view CentralWaterHeatingSystem::heat_source_configurations_name = "heat_source_configurations";

		const std::string_view CentralWaterHeatingSystem::primary_heat_source_id_name = "primary_heat_source_id";

		const std::string_view CentralWaterHeatingSystem::standby_power_name = "standby_power";

		const std::string_view CentralWaterHeatingSystem::external_inlet_height_name = "external_inlet_height";

		const std::string_view CentralWaterHeatingSystem::external_outlet_height_name = "external_outlet_height";

		const std::string_view CentralWaterHeatingSystem::multipass_flow_rate_name = "multipass_flow_rate";

	}
}

