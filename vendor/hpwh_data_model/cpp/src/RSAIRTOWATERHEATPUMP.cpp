// SPDX-FileCopyrightText: © 2026 Big Ladder Software <info@bigladdersoftware.com>
// SPDX-License-Identifier: BSD-3-Clause
#include <RSAIRTOWATERHEATPUMP.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace rsairtowaterheatpump  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		const std::string_view Schema::schema_title = "Condenser Water Heat Source";

		const std::string_view Schema::schema_version = "0.1.0";

		const std::string_view Schema::schema_description = "Schema for ASHRAE 205 annex RSAIRTOWATERHEATPUMP: Air-to-Water Heat Pump";

		void from_json(const nlohmann::json& j, ProductInformation& x) {
			json_get<std::string>(j, logger.get(), "manufacturer", x.manufacturer, x.manufacturer_is_set, true);
			json_get<std::string>(j, logger.get(), "model_number", x.model_number, x.model_number_is_set, true);
		}
		void to_json(nlohmann::json& j, const ProductInformation& x) {
			json_set<std::string>(j, "manufacturer", x.manufacturer, x.manufacturer_is_set);
			json_set<std::string>(j, "model_number", x.model_number, x.model_number_is_set);
		}
		const std::string_view ProductInformation::manufacturer_units = "";

		const std::string_view ProductInformation::manufacturer_description = "Manufacturer name";

		const std::string_view ProductInformation::manufacturer_name = "manufacturer";

		const std::string_view ProductInformation::model_number_units = "";

		const std::string_view ProductInformation::model_number_description = "Model number";

		const std::string_view ProductInformation::model_number_name = "model_number";

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
		const std::string_view GridVariables::evaporator_environment_dry_bulb_temperature_units = "K";

		const std::string_view GridVariables::evaporator_environment_dry_bulb_temperature_description = "Dry bulb temperature of the air entering the evaporator coil";

		const std::string_view GridVariables::evaporator_environment_dry_bulb_temperature_name = "evaporator_environment_dry_bulb_temperature";

		const std::string_view GridVariables::condenser_entering_temperature_units = "K";

		const std::string_view GridVariables::condenser_entering_temperature_description = "Average water temperature entering the condenser";

		const std::string_view GridVariables::condenser_entering_temperature_name = "condenser_entering_temperature";

		const std::string_view GridVariables::condenser_leaving_temperature_units = "K";

		const std::string_view GridVariables::condenser_leaving_temperature_description = "Average water temperature leaving the condenser";

		const std::string_view GridVariables::condenser_leaving_temperature_name = "condenser_leaving_temperature";

		void from_json(const nlohmann::json& j, LookupVariables& x) {
			json_get<std::vector<double>>(j, logger.get(), "input_power", x.input_power, x.input_power_is_set, true);
			json_get<std::vector<double>>(j, logger.get(), "heating_capacity", x.heating_capacity, x.heating_capacity_is_set, true);
		}
		void to_json(nlohmann::json& j, const LookupVariables& x) {
			json_set<std::vector<double>>(j, "input_power", x.input_power, x.input_power_is_set);
			json_set<std::vector<double>>(j, "heating_capacity", x.heating_capacity, x.heating_capacity_is_set);
		}
		const std::string_view LookupVariables::input_power_units = "W";

		const std::string_view LookupVariables::input_power_description = "Power draw from the compressor, evaporator fan, and any auxiliary power used by the units controls";

		const std::string_view LookupVariables::input_power_name = "input_power";

		const std::string_view LookupVariables::heating_capacity_units = "W";

		const std::string_view LookupVariables::heating_capacity_description = "Total heat added by the condenser to the adjacent water";

		const std::string_view LookupVariables::heating_capacity_name = "heating_capacity";

		void from_json(const nlohmann::json& j, MaximumSetpointAtLowTemperature& x) {
			json_get<double>(j, logger.get(), "threshold_environment_temperature", x.threshold_environment_temperature, x.threshold_environment_temperature_is_set, true);
			json_get<double>(j, logger.get(), "maximum_setpoint_temperature", x.maximum_setpoint_temperature, x.maximum_setpoint_temperature_is_set, true);
		}
		void to_json(nlohmann::json& j, const MaximumSetpointAtLowTemperature& x) {
			json_set<double>(j, "threshold_environment_temperature", x.threshold_environment_temperature, x.threshold_environment_temperature_is_set);
			json_set<double>(j, "maximum_setpoint_temperature", x.maximum_setpoint_temperature, x.maximum_setpoint_temperature_is_set);
		}
		const std::string_view MaximumSetpointAtLowTemperature::threshold_environment_temperature_units = "K";

		const std::string_view MaximumSetpointAtLowTemperature::threshold_environment_temperature_description = "Low environment-temperature threshold for setpoint control";

		const std::string_view MaximumSetpointAtLowTemperature::threshold_environment_temperature_name = "threshold_environment_temperature";

		const std::string_view MaximumSetpointAtLowTemperature::maximum_setpoint_temperature_units = "K";

		const std::string_view MaximumSetpointAtLowTemperature::maximum_setpoint_temperature_description = "Maximum setpoint temperature below `threshold_environment_temperature`";

		const std::string_view MaximumSetpointAtLowTemperature::maximum_setpoint_temperature_name = "maximum_setpoint_temperature";

		void from_json(const nlohmann::json& j, ResistanceElementDefrost& x) {
			json_get<double>(j, logger.get(), "input_power", x.input_power, x.input_power_is_set, true);
			json_get<double>(j, logger.get(), "lift_temperature", x.lift_temperature, x.lift_temperature_is_set, true);
			json_get<double>(j, logger.get(), "activation_temperature", x.activation_temperature, x.activation_temperature_is_set, true);
		}
		void to_json(nlohmann::json& j, const ResistanceElementDefrost& x) {
			json_set<double>(j, "input_power", x.input_power, x.input_power_is_set);
			json_set<double>(j, "lift_temperature", x.lift_temperature, x.lift_temperature_is_set);
			json_set<double>(j, "activation_temperature", x.activation_temperature, x.activation_temperature_is_set);
		}
		const std::string_view ResistanceElementDefrost::input_power_units = "K";

		const std::string_view ResistanceElementDefrost::input_power_description = "Input power";

		const std::string_view ResistanceElementDefrost::input_power_name = "input_power";

		const std::string_view ResistanceElementDefrost::lift_temperature_units = "K";

		const std::string_view ResistanceElementDefrost::lift_temperature_description = "Resulting increase of environment temperature";

		const std::string_view ResistanceElementDefrost::lift_temperature_name = "lift_temperature";

		const std::string_view ResistanceElementDefrost::activation_temperature_units = "K";

		const std::string_view ResistanceElementDefrost::activation_temperature_description = "Low environment temperature for activation";

		const std::string_view ResistanceElementDefrost::activation_temperature_name = "activation_temperature";

		void from_json(const nlohmann::json& j, Description& x) {
			json_get<rsairtowaterheatpump::ProductInformation>(j, logger.get(), "product_information", x.product_information, x.product_information_is_set, false);
		}
		void to_json(nlohmann::json& j, const Description& x) {
			json_set<rsairtowaterheatpump::ProductInformation>(j, "product_information", x.product_information, x.product_information_is_set);
		}
		const std::string_view Description::product_information_units = "";

		const std::string_view Description::product_information_description = "Data group describing product information";

		const std::string_view Description::product_information_name = "product_information";

		void from_json(const nlohmann::json& j, PerformanceMap& x) {
			json_get<rsairtowaterheatpump::GridVariables>(j, logger.get(), "grid_variables", x.grid_variables, x.grid_variables_is_set, true);
			json_get<rsairtowaterheatpump::LookupVariables>(j, logger.get(), "lookup_variables", x.lookup_variables, x.lookup_variables_is_set, true);
		}
		void to_json(nlohmann::json& j, const PerformanceMap& x) {
			json_set<rsairtowaterheatpump::GridVariables>(j, "grid_variables", x.grid_variables, x.grid_variables_is_set);
			json_set<rsairtowaterheatpump::LookupVariables>(j, "lookup_variables", x.lookup_variables, x.lookup_variables_is_set);
		}
		const std::string_view PerformanceMap::grid_variables_units = "";

		const std::string_view PerformanceMap::grid_variables_description = "Data group defining the grid variables for heating performance";

		const std::string_view PerformanceMap::grid_variables_name = "grid_variables";

		const std::string_view PerformanceMap::lookup_variables_units = "";

		const std::string_view PerformanceMap::lookup_variables_description = "Data group defining the lookup variables for heating performance";

		const std::string_view PerformanceMap::lookup_variables_name = "lookup_variables";

		void from_json(const nlohmann::json& j, Performance& x) {
			json_get<rsairtowaterheatpump::PerformanceMap>(j, logger.get(), "performance_map", x.performance_map, x.performance_map_is_set, true);
			json_get<double>(j, logger.get(), "standby_power", x.standby_power, x.standby_power_is_set, false);
			json_get<double>(j, logger.get(), "maximum_refrigerant_temperature", x.maximum_refrigerant_temperature, x.maximum_refrigerant_temperature_is_set, false);
			json_get<double>(j, logger.get(), "compressor_lockout_temperature_hysteresis", x.compressor_lockout_temperature_hysteresis, x.compressor_lockout_temperature_hysteresis_is_set, false);
			json_get<bool>(j, logger.get(), "use_defrost_map", x.use_defrost_map, x.use_defrost_map_is_set, false);
			json_get<rsairtowaterheatpump::MaximumSetpointAtLowTemperature>(j, logger.get(), "maximum_setpoint_at_low_temperature", x.maximum_setpoint_at_low_temperature, x.maximum_setpoint_at_low_temperature_is_set, false);
			json_get<rsairtowaterheatpump::ResistanceElementDefrost>(j, logger.get(), "resistance_element_defrost", x.resistance_element_defrost, x.resistance_element_defrost_is_set, false);
		}
		void to_json(nlohmann::json& j, const Performance& x) {
			json_set<rsairtowaterheatpump::PerformanceMap>(j, "performance_map", x.performance_map, x.performance_map_is_set);
			json_set<double>(j, "standby_power", x.standby_power, x.standby_power_is_set);
			json_set<double>(j, "maximum_refrigerant_temperature", x.maximum_refrigerant_temperature, x.maximum_refrigerant_temperature_is_set);
			json_set<double>(j, "compressor_lockout_temperature_hysteresis", x.compressor_lockout_temperature_hysteresis, x.compressor_lockout_temperature_hysteresis_is_set);
			json_set<bool>(j, "use_defrost_map", x.use_defrost_map, x.use_defrost_map_is_set);
			json_set<rsairtowaterheatpump::MaximumSetpointAtLowTemperature>(j, "maximum_setpoint_at_low_temperature", x.maximum_setpoint_at_low_temperature, x.maximum_setpoint_at_low_temperature_is_set);
			json_set<rsairtowaterheatpump::ResistanceElementDefrost>(j, "resistance_element_defrost", x.resistance_element_defrost, x.resistance_element_defrost_is_set);
		}
		const std::string_view Performance::performance_map_units = "";

		const std::string_view Performance::performance_map_description = "Performance map";

		const std::string_view Performance::performance_map_name = "performance_map";

		const std::string_view Performance::standby_power_units = "W";

		const std::string_view Performance::standby_power_description = "";

		const std::string_view Performance::standby_power_name = "standby_power";

		const std::string_view Performance::maximum_refrigerant_temperature_units = "K";

		const std::string_view Performance::maximum_refrigerant_temperature_description = "Maximum temperature of the refrigerant entering the condenser";

		const std::string_view Performance::maximum_refrigerant_temperature_name = "maximum_refrigerant_temperature";

		const std::string_view Performance::compressor_lockout_temperature_hysteresis_units = "K";

		const std::string_view Performance::compressor_lockout_temperature_hysteresis_description = "Hysteresis for compressor lockout";

		const std::string_view Performance::compressor_lockout_temperature_hysteresis_name = "compressor_lockout_temperature_hysteresis";

		const std::string_view Performance::use_defrost_map_units = "";

		const std::string_view Performance::use_defrost_map_description = "Use defrost map";

		const std::string_view Performance::use_defrost_map_name = "use_defrost_map";

		const std::string_view Performance::maximum_setpoint_at_low_temperature_units = "";

		const std::string_view Performance::maximum_setpoint_at_low_temperature_description = "Use maximum setpoint temperature at low environment temperature.";

		const std::string_view Performance::maximum_setpoint_at_low_temperature_name = "maximum_setpoint_at_low_temperature";

		const std::string_view Performance::resistance_element_defrost_units = "";

		const std::string_view Performance::resistance_element_defrost_description = "Resistance element for defrost";

		const std::string_view Performance::resistance_element_defrost_name = "resistance_element_defrost";

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
		const std::string_view RSAIRTOWATERHEATPUMP::metadata_units = "";

		const std::string_view RSAIRTOWATERHEATPUMP::metadata_description = "Metadata data group";

		const std::string_view RSAIRTOWATERHEATPUMP::metadata_name = "metadata";

		const std::string_view RSAIRTOWATERHEATPUMP::description_units = "";

		const std::string_view RSAIRTOWATERHEATPUMP::description_description = "Data group describing product and rating information";

		const std::string_view RSAIRTOWATERHEATPUMP::description_name = "description";

		const std::string_view RSAIRTOWATERHEATPUMP::performance_units = "";

		const std::string_view RSAIRTOWATERHEATPUMP::performance_description = "Data group containing performance information";

		const std::string_view RSAIRTOWATERHEATPUMP::performance_name = "performance";

	}
}

