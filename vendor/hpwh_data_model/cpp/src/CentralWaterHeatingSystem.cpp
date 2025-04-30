#include <CentralWaterHeatingSystem.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace central_water_heating_system  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		const std::string_view Schema::schema_title = "Central Water Heating System";

		const std::string_view Schema::schema_version = "0.1.0";

		const std::string_view Schema::schema_description = "Components that make up a central water heating system";

		void from_json(const nlohmann::json& j, SecondaryHeatExchanger& x) {
			json_get<double>(j, logger.get(), "cold_side_temperature_offset", x.cold_side_temperature_offset, x.cold_side_temperature_offset_is_set, true);
			json_get<double>(j, logger.get(), "hot_side_temperature_offset", x.hot_side_temperature_offset, x.hot_side_temperature_offset_is_set, true);
			json_get<double>(j, logger.get(), "extra_pump_power", x.extra_pump_power, x.extra_pump_power_is_set, true);
		}
		void to_json(nlohmann::json& j, const SecondaryHeatExchanger& x) {
			json_set<double>(j, "cold_side_temperature_offset", x.cold_side_temperature_offset, x.cold_side_temperature_offset_is_set);
			json_set<double>(j, "hot_side_temperature_offset", x.hot_side_temperature_offset, x.hot_side_temperature_offset_is_set);
			json_set<double>(j, "extra_pump_power", x.extra_pump_power, x.extra_pump_power_is_set);
		}
		const std::string_view SecondaryHeatExchanger::cold_side_temperature_offset_units = "K";

		const std::string_view SecondaryHeatExchanger::cold_side_temperature_offset_description = "Cold-side offset temperature";

		const std::string_view SecondaryHeatExchanger::cold_side_temperature_offset_name = "cold_side_temperature_offset";

		const std::string_view SecondaryHeatExchanger::hot_side_temperature_offset_units = "K";

		const std::string_view SecondaryHeatExchanger::hot_side_temperature_offset_description = "Hot-side offset temperature";

		const std::string_view SecondaryHeatExchanger::hot_side_temperature_offset_name = "hot_side_temperature_offset";

		const std::string_view SecondaryHeatExchanger::extra_pump_power_units = "W";

		const std::string_view SecondaryHeatExchanger::extra_pump_power_description = "Extra pump power";

		const std::string_view SecondaryHeatExchanger::extra_pump_power_name = "extra_pump_power";

		void from_json(const nlohmann::json& j, CentralWaterHeatingSystem& x) {
			json_get<rstank::RSTANK>(j, logger.get(), "tank", x.tank, x.tank_is_set, true);
			json_get<std::vector<heat_source_configuration::HeatSourceConfiguration>>(j, logger.get(), "heat_source_configurations", x.heat_source_configurations, x.heat_source_configurations_is_set, false);
			json_get<std::string>(j, logger.get(), "primary_heat_source_id", x.primary_heat_source_id, x.primary_heat_source_id_is_set, false);
			json_get<double>(j, logger.get(), "standby_power", x.standby_power, x.standby_power_is_set, false);
			json_get<double>(j, logger.get(), "external_inlet_height", x.external_inlet_height, x.external_inlet_height_is_set, false);
			json_get<double>(j, logger.get(), "external_outlet_height", x.external_outlet_height, x.external_outlet_height_is_set, false);
			json_get<central_water_heating_system::ControlType>(j, logger.get(), "control_type", x.control_type, x.control_type_is_set, true);
			json_get<double>(j, logger.get(), "fixed_flow_rate", x.fixed_flow_rate, x.fixed_flow_rate_is_set, true);
			json_get<central_water_heating_system::SecondaryHeatExchanger>(j, logger.get(), "secondary_heat_exchanger", x.secondary_heat_exchanger, x.secondary_heat_exchanger_is_set, false);
		}
		void to_json(nlohmann::json& j, const CentralWaterHeatingSystem& x) {
			json_set<rstank::RSTANK>(j, "tank", x.tank, x.tank_is_set);
			json_set<std::vector<heat_source_configuration::HeatSourceConfiguration>>(j, "heat_source_configurations", x.heat_source_configurations, x.heat_source_configurations_is_set);
			json_set<std::string>(j, "primary_heat_source_id", x.primary_heat_source_id, x.primary_heat_source_id_is_set);
			json_set<double>(j, "standby_power", x.standby_power, x.standby_power_is_set);
			json_set<double>(j, "external_inlet_height", x.external_inlet_height, x.external_inlet_height_is_set);
			json_set<double>(j, "external_outlet_height", x.external_outlet_height, x.external_outlet_height_is_set);
			json_set<central_water_heating_system::ControlType>(j, "control_type", x.control_type, x.control_type_is_set);
			json_set<double>(j, "fixed_flow_rate", x.fixed_flow_rate, x.fixed_flow_rate_is_set);
			json_set<central_water_heating_system::SecondaryHeatExchanger>(j, "secondary_heat_exchanger", x.secondary_heat_exchanger, x.secondary_heat_exchanger_is_set);
		}
		const std::string_view CentralWaterHeatingSystem::tank_units = "";

		const std::string_view CentralWaterHeatingSystem::tank_description = "The corresponding Standard 205 tank representation";

		const std::string_view CentralWaterHeatingSystem::tank_name = "tank";

		const std::string_view CentralWaterHeatingSystem::heat_source_configurations_units = "";

		const std::string_view CentralWaterHeatingSystem::heat_source_configurations_description = "Describes how the heat sources are configured within the tank";

		const std::string_view CentralWaterHeatingSystem::heat_source_configurations_name = "heat_source_configurations";

		const std::string_view CentralWaterHeatingSystem::primary_heat_source_id_units = "";

		const std::string_view CentralWaterHeatingSystem::primary_heat_source_id_description = "Turns on independently of other heat sources";

		const std::string_view CentralWaterHeatingSystem::primary_heat_source_id_name = "primary_heat_source_id";

		const std::string_view CentralWaterHeatingSystem::standby_power_units = "";

		const std::string_view CentralWaterHeatingSystem::standby_power_description = "Power drawn when system is in standby mode";

		const std::string_view CentralWaterHeatingSystem::standby_power_name = "standby_power";

		const std::string_view CentralWaterHeatingSystem::external_inlet_height_units = "";

		const std::string_view CentralWaterHeatingSystem::external_inlet_height_description = "Fractional tank height of inlet";

		const std::string_view CentralWaterHeatingSystem::external_inlet_height_name = "external_inlet_height";

		const std::string_view CentralWaterHeatingSystem::external_outlet_height_units = "";

		const std::string_view CentralWaterHeatingSystem::external_outlet_height_description = "Fractional tank height of outlet";

		const std::string_view CentralWaterHeatingSystem::external_outlet_height_name = "external_outlet_height";

		const std::string_view CentralWaterHeatingSystem::control_type_units = "";

		const std::string_view CentralWaterHeatingSystem::control_type_description = "Control type";

		const std::string_view CentralWaterHeatingSystem::control_type_name = "control_type";

		const std::string_view CentralWaterHeatingSystem::fixed_flow_rate_units = "m3/s";

		const std::string_view CentralWaterHeatingSystem::fixed_flow_rate_description = "Flow rate, if multipass system";

		const std::string_view CentralWaterHeatingSystem::fixed_flow_rate_name = "fixed_flow_rate";

		const std::string_view CentralWaterHeatingSystem::secondary_heat_exchanger_units = "";

		const std::string_view CentralWaterHeatingSystem::secondary_heat_exchanger_description = "Secondary heat exchanger";

		const std::string_view CentralWaterHeatingSystem::secondary_heat_exchanger_name = "secondary_heat_exchanger";

	}
}

