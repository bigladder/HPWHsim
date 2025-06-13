#include <CentralWaterHeatingSystem.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace central_water_heating_system  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

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
		void from_json(const nlohmann::json& j, CentralWaterHeatingSystem& x) {
			json_get<rstank::RSTANK>(j, logger.get(), "tank", x.tank, x.tank_is_set, true);
			json_get<std::vector<heat_source_configuration::HeatSourceConfiguration>>(j, logger.get(), "heat_source_configurations", x.heat_source_configurations, x.heat_source_configurations_is_set, false);
			json_get<std::string>(j, logger.get(), "primary_heat_source_id", x.primary_heat_source_id, x.primary_heat_source_id_is_set, false);
			json_get<double>(j, logger.get(), "standby_power", x.standby_power, x.standby_power_is_set, false);
			json_get<double>(j, logger.get(), "external_inlet_height", x.external_inlet_height, x.external_inlet_height_is_set, false);
			json_get<double>(j, logger.get(), "external_outlet_height", x.external_outlet_height, x.external_outlet_height_is_set, false);
			json_get<central_water_heating_system::ControlType>(j, logger.get(), "control_type", x.control_type, x.control_type_is_set, true);
			json_get<double>(j, logger.get(), "fixed_flow_rate", x.fixed_flow_rate, x.fixed_flow_rate_is_set, false);
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
	}
}

