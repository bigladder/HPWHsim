#include <HeatSourceConfiguration.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace heat_source_configuration  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		const std::string_view Schema::schema_title = "ASHRAE 205";

		const std::string_view Schema::schema_version = "0.2.0";

		const std::string_view Schema::schema_description = "Base schema for ASHRAE 205 representations";

		void from_json(const nlohmann::json& j, WeightedDistribution& x) {
			json_get<std::vector<double>>(j, logger.get(), "normalized_height", x.normalized_height, x.normalized_height_is_set, true);
			json_get<std::vector<double>>(j, logger.get(), "weight", x.weight, x.weight_is_set, true);
		}
		void to_json(nlohmann::json& j, const WeightedDistribution& x) {
			json_set<std::vector<double>>(j, "normalized_height", x.normalized_height, x.normalized_height_is_set);
			json_set<std::vector<double>>(j, "weight", x.weight, x.weight_is_set);
		}
		const std::string_view WeightedDistribution::normalized_height_units = "-";

		const std::string_view WeightedDistribution::normalized_height_description = "Normalized heights within the tank where the weight is defined between the bottom (0.0), and the top (1.0)";

		const std::string_view WeightedDistribution::normalized_height_name = "normalized_height";

		const std::string_view WeightedDistribution::weight_units = "-";

		const std::string_view WeightedDistribution::weight_description = "Weight at the corresponding normalized height";

		const std::string_view WeightedDistribution::weight_name = "weight";

		void from_json(const nlohmann::json& j, HeatingLogic& x) {
			json_get<heat_source_configuration::HeatingLogicType>(j, logger.get(), "heating_logic_type", x.heating_logic_type, x.heating_logic_type_is_set, true);
			if (x.heating_logic_type == heat_source_configuration::HeatingLogicType::STATE_OF_CHARGE_BASED) {
				x.heating_logic = std::make_unique<heat_source_configuration::StateOfChargeBasedHeatingLogic>();
				if (x.heating_logic) {
					from_json(j.at("heating_logic"), *dynamic_cast<heat_source_configuration::StateOfChargeBasedHeatingLogic*>(x.heating_logic.get()));
				}
			}
			if (x.heating_logic_type == heat_source_configuration::HeatingLogicType::TEMPERATURE_BASED) {
				x.heating_logic = std::make_unique<heat_source_configuration::TemperatureBasedHeatingLogic>();
				if (x.heating_logic) {
					from_json(j.at("heating_logic"), *dynamic_cast<heat_source_configuration::TemperatureBasedHeatingLogic*>(x.heating_logic.get()));
				}
			}
			json_get<heat_source_configuration::ComparisonType>(j, logger.get(), "comparison_type", x.comparison_type, x.comparison_type_is_set, true);
		}
		void to_json(nlohmann::json& j, const HeatingLogic& x) {
			json_set<heat_source_configuration::HeatingLogicType>(j, "heating_logic_type", x.heating_logic_type, x.heating_logic_type_is_set);
			if (x.heating_logic_type == heat_source_configuration::HeatingLogicType::STATE_OF_CHARGE_BASED) {
				json_set<heat_source_configuration::StateOfChargeBasedHeatingLogic>(j, "heating_logic", *dynamic_cast<const heat_source_configuration::StateOfChargeBasedHeatingLogic*>(x.heating_logic.get()), x.heating_logic_is_set);
			}
			if (x.heating_logic_type == heat_source_configuration::HeatingLogicType::TEMPERATURE_BASED) {
				json_set<heat_source_configuration::TemperatureBasedHeatingLogic>(j, "heating_logic", *dynamic_cast<const heat_source_configuration::TemperatureBasedHeatingLogic*>(x.heating_logic.get()), x.heating_logic_is_set);
			}
			json_set<heat_source_configuration::ComparisonType>(j, "comparison_type", x.comparison_type, x.comparison_type_is_set);
		}
		const std::string_view HeatingLogic::heating_logic_type_units = "";

		const std::string_view HeatingLogic::heating_logic_type_description = "Type of heating logic";

		const std::string_view HeatingLogic::heating_logic_type_name = "heating_logic_type";

		const std::string_view HeatingLogic::heating_logic_units = "";

		const std::string_view HeatingLogic::heating_logic_description = "Specific heating logic";

		const std::string_view HeatingLogic::heating_logic_name = "heating_logic";

		const std::string_view HeatingLogic::comparison_type_units = "";

		const std::string_view HeatingLogic::comparison_type_description = "Result of comparison for activation";

		const std::string_view HeatingLogic::comparison_type_name = "comparison_type";

		void from_json(const nlohmann::json& j, HeatSourceConfiguration& x) {
			json_get<std::string>(j, logger.get(), "id", x.id, x.id_is_set, true);
			json_get<heat_source_configuration::HeatSourceType>(j, logger.get(), "heat_source_type", x.heat_source_type, x.heat_source_type_is_set, true);
			if (x.heat_source_type == heat_source_configuration::HeatSourceType::RESISTANCE) {
				x.heat_source = std::make_unique<rsresistancewaterheatsource::RSRESISTANCEWATERHEATSOURCE>();
				if (x.heat_source) {
					from_json(j.at("heat_source"), *dynamic_cast<rsresistancewaterheatsource::RSRESISTANCEWATERHEATSOURCE*>(x.heat_source.get()));
				}
			}
			if (x.heat_source_type == heat_source_configuration::HeatSourceType::CONDENSER) {
				x.heat_source = std::make_unique<rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE>();
				if (x.heat_source) {
					from_json(j.at("heat_source"), *dynamic_cast<rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE*>(x.heat_source.get()));
				}
			}
			if (x.heat_source_type == heat_source_configuration::HeatSourceType::AIRTOWATERHEATPUMP) {
				x.heat_source = std::make_unique<rsairtowaterheatpump::RSAIRTOWATERHEATPUMP>();
				if (x.heat_source) {
					from_json(j.at("heat_source"), *dynamic_cast<rsairtowaterheatpump::RSAIRTOWATERHEATPUMP*>(x.heat_source.get()));
				}
			}
			json_get<heat_source_configuration::WeightedDistribution>(j, logger.get(), "heat_distribution", x.heat_distribution, x.heat_distribution_is_set, true);
			json_get<std::vector<heat_source_configuration::HeatingLogic>>(j, logger.get(), "turn_on_logic", x.turn_on_logic, x.turn_on_logic_is_set, true);
			json_get<std::vector<heat_source_configuration::HeatingLogic>>(j, logger.get(), "shut_off_logic", x.shut_off_logic, x.shut_off_logic_is_set, false);
			json_get<heat_source_configuration::HeatingLogic>(j, logger.get(), "standby_logic", x.standby_logic, x.standby_logic_is_set, false);
			json_get<std::string>(j, logger.get(), "backup_heat_source_id", x.backup_heat_source_id, x.backup_heat_source_id_is_set, false);
			json_get<std::string>(j, logger.get(), "followed_by_heat_source_id", x.followed_by_heat_source_id, x.followed_by_heat_source_id_is_set, false);
			json_get<std::string>(j, logger.get(), "companion_heat_source_id", x.companion_heat_source_id, x.companion_heat_source_id_is_set, false);
		}
		void to_json(nlohmann::json& j, const HeatSourceConfiguration& x) {
			json_set<std::string>(j, "id", x.id, x.id_is_set);
			json_set<heat_source_configuration::HeatSourceType>(j, "heat_source_type", x.heat_source_type, x.heat_source_type_is_set);
			if (x.heat_source_type == heat_source_configuration::HeatSourceType::RESISTANCE) {
				json_set<rsresistancewaterheatsource::RSRESISTANCEWATERHEATSOURCE>(j, "heat_source", *dynamic_cast<const rsresistancewaterheatsource::RSRESISTANCEWATERHEATSOURCE*>(x.heat_source.get()), x.heat_source_is_set);
			}
			if (x.heat_source_type == heat_source_configuration::HeatSourceType::CONDENSER) {
				json_set<rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE>(j, "heat_source", *dynamic_cast<const rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE*>(x.heat_source.get()), x.heat_source_is_set);
			}
			if (x.heat_source_type == heat_source_configuration::HeatSourceType::AIRTOWATERHEATPUMP) {
				json_set<rsairtowaterheatpump::RSAIRTOWATERHEATPUMP>(j, "heat_source", *dynamic_cast<const rsairtowaterheatpump::RSAIRTOWATERHEATPUMP*>(x.heat_source.get()), x.heat_source_is_set);
			}
			json_set<heat_source_configuration::WeightedDistribution>(j, "heat_distribution", x.heat_distribution, x.heat_distribution_is_set);
			json_set<std::vector<heat_source_configuration::HeatingLogic>>(j, "turn_on_logic", x.turn_on_logic, x.turn_on_logic_is_set);
			json_set<std::vector<heat_source_configuration::HeatingLogic>>(j, "shut_off_logic", x.shut_off_logic, x.shut_off_logic_is_set);
			json_set<heat_source_configuration::HeatingLogic>(j, "standby_logic", x.standby_logic, x.standby_logic_is_set);
			json_set<std::string>(j, "backup_heat_source_id", x.backup_heat_source_id, x.backup_heat_source_id_is_set);
			json_set<std::string>(j, "followed_by_heat_source_id", x.followed_by_heat_source_id, x.followed_by_heat_source_id_is_set);
			json_set<std::string>(j, "companion_heat_source_id", x.companion_heat_source_id, x.companion_heat_source_id_is_set);
		}
		const std::string_view HeatSourceConfiguration::id_units = "";

		const std::string_view HeatSourceConfiguration::id_description = "Identifier for reference";

		const std::string_view HeatSourceConfiguration::id_name = "id";

		const std::string_view HeatSourceConfiguration::heat_source_type_units = "";

		const std::string_view HeatSourceConfiguration::heat_source_type_description = "Type of heat source";

		const std::string_view HeatSourceConfiguration::heat_source_type_name = "heat_source_type";

		const std::string_view HeatSourceConfiguration::heat_source_units = "";

		const std::string_view HeatSourceConfiguration::heat_source_description = "A corresponding Standard 205 heat-source representation";

		const std::string_view HeatSourceConfiguration::heat_source_name = "heat_source";

		const std::string_view HeatSourceConfiguration::heat_distribution_units = "";

		const std::string_view HeatSourceConfiguration::heat_distribution_description = "Weighted distribution of heat from the heat source along the height of the tank";

		const std::string_view HeatSourceConfiguration::heat_distribution_name = "heat_distribution";

		const std::string_view HeatSourceConfiguration::turn_on_logic_units = "";

		const std::string_view HeatSourceConfiguration::turn_on_logic_description = "An array of logic conditions that define when the heat source turns on if it is currently off";

		const std::string_view HeatSourceConfiguration::turn_on_logic_name = "turn_on_logic";

		const std::string_view HeatSourceConfiguration::shut_off_logic_units = "";

		const std::string_view HeatSourceConfiguration::shut_off_logic_description = "An array of logic conditions that define when the heat source shuts off if it is currently on";

		const std::string_view HeatSourceConfiguration::shut_off_logic_name = "shut_off_logic";

		const std::string_view HeatSourceConfiguration::standby_logic_units = "";

		const std::string_view HeatSourceConfiguration::standby_logic_description = "Checks that bottom is below a temperature to prevent short cycling";

		const std::string_view HeatSourceConfiguration::standby_logic_name = "standby_logic";

		const std::string_view HeatSourceConfiguration::backup_heat_source_id_units = "";

		const std::string_view HeatSourceConfiguration::backup_heat_source_id_description = "Reference to the `HeatSourceConfiguration` that engages if this one cannot meet the setpoint";

		const std::string_view HeatSourceConfiguration::backup_heat_source_id_name = "backup_heat_source_id";

		const std::string_view HeatSourceConfiguration::followed_by_heat_source_id_units = "";

		const std::string_view HeatSourceConfiguration::followed_by_heat_source_id_description = "Reference to the `HeatSourceConfiguration` that should always engage after this one";

		const std::string_view HeatSourceConfiguration::followed_by_heat_source_id_name = "followed_by_heat_source_id";

		const std::string_view HeatSourceConfiguration::companion_heat_source_id_units = "";

		const std::string_view HeatSourceConfiguration::companion_heat_source_id_description = "Reference to the `HeatSourceConfiguration` that should always engage concurrently with this one";

		const std::string_view HeatSourceConfiguration::companion_heat_source_id_name = "companion_heat_source_id";

		void from_json(const nlohmann::json& j, TemperatureBasedHeatingLogic& x) {
			json_get<double>(j, logger.get(), "absolute_temperature", x.absolute_temperature, x.absolute_temperature_is_set, true);
			json_get<double>(j, logger.get(), "differential_temperature", x.differential_temperature, x.differential_temperature_is_set, true);
			json_get<heat_source_configuration::StandbyTemperatureLocation>(j, logger.get(), "standby_temperature_location", x.standby_temperature_location, x.standby_temperature_location_is_set, true);
			json_get<heat_source_configuration::WeightedDistribution>(j, logger.get(), "temperature_weight_distribution", x.temperature_weight_distribution, x.temperature_weight_distribution_is_set, true);
		}
		void to_json(nlohmann::json& j, const TemperatureBasedHeatingLogic& x) {
			json_set<double>(j, "absolute_temperature", x.absolute_temperature, x.absolute_temperature_is_set);
			json_set<double>(j, "differential_temperature", x.differential_temperature, x.differential_temperature_is_set);
			json_set<heat_source_configuration::StandbyTemperatureLocation>(j, "standby_temperature_location", x.standby_temperature_location, x.standby_temperature_location_is_set);
			json_set<heat_source_configuration::WeightedDistribution>(j, "temperature_weight_distribution", x.temperature_weight_distribution, x.temperature_weight_distribution_is_set);
		}
		const std::string_view TemperatureBasedHeatingLogic::absolute_temperature_units = "K";

		const std::string_view TemperatureBasedHeatingLogic::absolute_temperature_description = "Absolute temperature for activation";

		const std::string_view TemperatureBasedHeatingLogic::absolute_temperature_name = "absolute_temperature";

		const std::string_view TemperatureBasedHeatingLogic::differential_temperature_units = "K";

		const std::string_view TemperatureBasedHeatingLogic::differential_temperature_description = "Temperature difference for activation";

		const std::string_view TemperatureBasedHeatingLogic::differential_temperature_name = "differential_temperature";

		const std::string_view TemperatureBasedHeatingLogic::standby_temperature_location_units = "";

		const std::string_view TemperatureBasedHeatingLogic::standby_temperature_location_description = "Standby temperature location";

		const std::string_view TemperatureBasedHeatingLogic::standby_temperature_location_name = "standby_temperature_location";

		const std::string_view TemperatureBasedHeatingLogic::temperature_weight_distribution_units = "";

		const std::string_view TemperatureBasedHeatingLogic::temperature_weight_distribution_description = "Weighted distribution for comparison, by division, in order";

		const std::string_view TemperatureBasedHeatingLogic::temperature_weight_distribution_name = "temperature_weight_distribution";

		void from_json(const nlohmann::json& j, StateOfChargeBasedHeatingLogic& x) {
			json_get<double>(j, logger.get(), "decision_point", x.decision_point, x.decision_point_is_set, true);
			json_get<double>(j, logger.get(), "minimum_useful_temperature", x.minimum_useful_temperature, x.minimum_useful_temperature_is_set, false);
			json_get<double>(j, logger.get(), "hysteresis_fraction", x.hysteresis_fraction, x.hysteresis_fraction_is_set, false);
			json_get<bool>(j, logger.get(), "uses_constant_mains", x.uses_constant_mains, x.uses_constant_mains_is_set, true);
			json_get<double>(j, logger.get(), "constant_mains_temperature", x.constant_mains_temperature, x.constant_mains_temperature_is_set, true);
		}
		void to_json(nlohmann::json& j, const StateOfChargeBasedHeatingLogic& x) {
			json_set<double>(j, "decision_point", x.decision_point, x.decision_point_is_set);
			json_set<double>(j, "minimum_useful_temperature", x.minimum_useful_temperature, x.minimum_useful_temperature_is_set);
			json_set<double>(j, "hysteresis_fraction", x.hysteresis_fraction, x.hysteresis_fraction_is_set);
			json_set<bool>(j, "uses_constant_mains", x.uses_constant_mains, x.uses_constant_mains_is_set);
			json_set<double>(j, "constant_mains_temperature", x.constant_mains_temperature, x.constant_mains_temperature_is_set);
		}
		const std::string_view StateOfChargeBasedHeatingLogic::decision_point_units = "";

		const std::string_view StateOfChargeBasedHeatingLogic::decision_point_description = "Decision point";

		const std::string_view StateOfChargeBasedHeatingLogic::decision_point_name = "decision_point";

		const std::string_view StateOfChargeBasedHeatingLogic::minimum_useful_temperature_units = "";

		const std::string_view StateOfChargeBasedHeatingLogic::minimum_useful_temperature_description = "Minimum useful temperature";

		const std::string_view StateOfChargeBasedHeatingLogic::minimum_useful_temperature_name = "minimum_useful_temperature";

		const std::string_view StateOfChargeBasedHeatingLogic::hysteresis_fraction_units = "-";

		const std::string_view StateOfChargeBasedHeatingLogic::hysteresis_fraction_description = "Hysteresis fraction";

		const std::string_view StateOfChargeBasedHeatingLogic::hysteresis_fraction_name = "hysteresis_fraction";

		const std::string_view StateOfChargeBasedHeatingLogic::uses_constant_mains_units = "";

		const std::string_view StateOfChargeBasedHeatingLogic::uses_constant_mains_description = "Uses constant mains";

		const std::string_view StateOfChargeBasedHeatingLogic::uses_constant_mains_name = "uses_constant_mains";

		const std::string_view StateOfChargeBasedHeatingLogic::constant_mains_temperature_units = "K";

		const std::string_view StateOfChargeBasedHeatingLogic::constant_mains_temperature_description = "Constant mains temperature";

		const std::string_view StateOfChargeBasedHeatingLogic::constant_mains_temperature_name = "constant_mains_temperature";

	}
}

