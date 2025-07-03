#include <RSAIRTOWATERHEATPUMP.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace rsairtowaterheatpump  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		void from_json(const nlohmann::json& j, ProductInformation& x) {
			json_get<std::string>(j, logger.get(), "manufacturer", x.manufacturer, x.manufacturer_is_set, true);
			json_get<std::string>(j, logger.get(), "model_number", x.model_number, x.model_number_is_set, true);
		}
		void to_json(nlohmann::json& j, const ProductInformation& x) {
			json_set<std::string>(j, "manufacturer", x.manufacturer, x.manufacturer_is_set);
			json_set<std::string>(j, "model_number", x.model_number, x.model_number_is_set);
		}
		void from_json(const nlohmann::json& j, Description& x) {
			json_get<rsairtowaterheatpump::ProductInformation>(j, logger.get(), "product_information", x.product_information, x.product_information_is_set, false);
		}
		void to_json(nlohmann::json& j, const Description& x) {
			json_set<rsairtowaterheatpump::ProductInformation>(j, "product_information", x.product_information, x.product_information_is_set);
		}
		void from_json(const nlohmann::json& j, GridVariables& x) {
			json_get<std::vector<double>>(j, logger.get(), "evaporator_environment_dry_bulb_temperature", x.evaporator_environment_dry_bulb_temperature, x.evaporator_environment_dry_bulb_temperature_is_set, true);
			json_get<std::vector<double>>(j, logger.get(), "condenser_entering_temperature", x.condenser_entering_temperature, x.condenser_entering_temperature_is_set, true);
			json_get<std::vector<double>>(j, logger.get(), "condenser_leaving_temperature", x.condenser_leaving_temperature, x.condenser_leaving_temperature_is_set, true);
		}
		void to_json(nlohmann::json& j, const GridVariables& x) {
			json_set<std::vector<double>>(j, "evaporator_environment_dry_bulb_temperature", x.evaporator_environment_dry_bulb_temperature, x.evaporator_environment_dry_bulb_temperature_is_set);
			json_set<std::vector<double>>(j, "condenser_entering_temperature", x.condenser_entering_temperature, x.condenser_entering_temperature_is_set);
			json_set<std::vector<double>>(j, "condenser_leaving_temperature", x.condenser_leaving_temperature, x.condenser_leaving_temperature_is_set);
		}
		void from_json(const nlohmann::json& j, LookupVariables& x) {
			json_get<std::vector<double>>(j, logger.get(), "input_power", x.input_power, x.input_power_is_set, true);
			json_get<std::vector<double>>(j, logger.get(), "heating_capacity", x.heating_capacity, x.heating_capacity_is_set, true);
		}
		void to_json(nlohmann::json& j, const LookupVariables& x) {
			json_set<std::vector<double>>(j, "input_power", x.input_power, x.input_power_is_set);
			json_set<std::vector<double>>(j, "heating_capacity", x.heating_capacity, x.heating_capacity_is_set);
		}
		void from_json(const nlohmann::json& j, PerformanceMap& x) {
			json_get<rsairtowaterheatpump::GridVariables>(j, logger.get(), "grid_variables", x.grid_variables, x.grid_variables_is_set, true);
			json_get<rsairtowaterheatpump::LookupVariables>(j, logger.get(), "lookup_variables", x.lookup_variables, x.lookup_variables_is_set, true);
		}
		void to_json(nlohmann::json& j, const PerformanceMap& x) {
			json_set<rsairtowaterheatpump::GridVariables>(j, "grid_variables", x.grid_variables, x.grid_variables_is_set);
			json_set<rsairtowaterheatpump::LookupVariables>(j, "lookup_variables", x.lookup_variables, x.lookup_variables_is_set);
		}
		void from_json(const nlohmann::json& j, Performance& x) {
			json_get<rsairtowaterheatpump::PerformanceMap>(j, logger.get(), "performance_map", x.performance_map, x.performance_map_is_set, true);
			json_get<double>(j, logger.get(), "standby_power", x.standby_power, x.standby_power_is_set, false);
			json_get<double>(j, logger.get(), "maximum_refrigerant_temperature", x.maximum_refrigerant_temperature, x.maximum_refrigerant_temperature_is_set, false);
			json_get<double>(j, logger.get(), "compressor_lockout_temperature_hysteresis", x.compressor_lockout_temperature_hysteresis, x.compressor_lockout_temperature_hysteresis_is_set, false);
			json_get<bool>(j, logger.get(), "use_defrost_map", x.use_defrost_map, x.use_defrost_map_is_set, false);
			json_get<rsairtowaterheatpump::LowTemperatureSetpointLimit>(j, logger.get(), "low_temperature_setpoint_limit", x.low_temperature_setpoint_limit, x.low_temperature_setpoint_limit_is_set, false);
			json_get<rsairtowaterheatpump::ResistanceElementDefrost>(j, logger.get(), "resistance_element_defrost", x.resistance_element_defrost, x.resistance_element_defrost_is_set, false);
		}
		void to_json(nlohmann::json& j, const Performance& x) {
			json_set<rsairtowaterheatpump::PerformanceMap>(j, "performance_map", x.performance_map, x.performance_map_is_set);
			json_set<double>(j, "standby_power", x.standby_power, x.standby_power_is_set);
			json_set<double>(j, "maximum_refrigerant_temperature", x.maximum_refrigerant_temperature, x.maximum_refrigerant_temperature_is_set);
			json_set<double>(j, "compressor_lockout_temperature_hysteresis", x.compressor_lockout_temperature_hysteresis, x.compressor_lockout_temperature_hysteresis_is_set);
			json_set<bool>(j, "use_defrost_map", x.use_defrost_map, x.use_defrost_map_is_set);
			json_set<rsairtowaterheatpump::LowTemperatureSetpointLimit>(j, "low_temperature_setpoint_limit", x.low_temperature_setpoint_limit, x.low_temperature_setpoint_limit_is_set);
			json_set<rsairtowaterheatpump::ResistanceElementDefrost>(j, "resistance_element_defrost", x.resistance_element_defrost, x.resistance_element_defrost_is_set);
		}
		void from_json(const nlohmann::json& j, RSAIRTOWATERHEATPUMP& x) {
			json_get<core::Metadata>(j, logger.get(), "metadata", x.metadata, x.metadata_is_set, true);
			json_get<rsairtowaterheatpump::Description>(j, logger.get(), "description", x.description, x.description_is_set, false);
			json_get<rsairtowaterheatpump::Performance>(j, logger.get(), "performance", x.performance, x.performance_is_set, true);
		}
		void to_json(nlohmann::json& j, const RSAIRTOWATERHEATPUMP& x) {
			json_set<core::Metadata>(j, "metadata", x.metadata, x.metadata_is_set);
			json_set<rsairtowaterheatpump::Description>(j, "description", x.description, x.description_is_set);
			json_set<rsairtowaterheatpump::Performance>(j, "performance", x.performance, x.performance_is_set);
		}
		void from_json(const nlohmann::json& j, LowTemperatureSetpointLimit& x) {
			json_get<double>(j, logger.get(), "low_temperature_threshold", x.low_temperature_threshold, x.low_temperature_threshold_is_set, true);
			json_get<double>(j, logger.get(), "maximum_setpoint_at_low_temperature", x.maximum_setpoint_at_low_temperature, x.maximum_setpoint_at_low_temperature_is_set, true);
		}
		void to_json(nlohmann::json& j, const LowTemperatureSetpointLimit& x) {
			json_set<double>(j, "low_temperature_threshold", x.low_temperature_threshold, x.low_temperature_threshold_is_set);
			json_set<double>(j, "maximum_setpoint_at_low_temperature", x.maximum_setpoint_at_low_temperature, x.maximum_setpoint_at_low_temperature_is_set);
		}
		void from_json(const nlohmann::json& j, ResistanceElementDefrost& x) {
			json_get<double>(j, logger.get(), "input_power", x.input_power, x.input_power_is_set, true);
			json_get<double>(j, logger.get(), "temperature_lift", x.temperature_lift, x.temperature_lift_is_set, true);
			json_get<double>(j, logger.get(), "activation_temperature_threshold", x.activation_temperature_threshold, x.activation_temperature_threshold_is_set, true);
		}
		void to_json(nlohmann::json& j, const ResistanceElementDefrost& x) {
			json_set<double>(j, "input_power", x.input_power, x.input_power_is_set);
			json_set<double>(j, "temperature_lift", x.temperature_lift, x.temperature_lift_is_set);
			json_set<double>(j, "activation_temperature_threshold", x.activation_temperature_threshold, x.activation_temperature_threshold_is_set);
		}
	}
}

