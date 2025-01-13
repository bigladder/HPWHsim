#ifndef HEAT_SOURCE_CONFIGURATION_H_
#define HEAT_SOURCE_CONFIGURATION_H_
#include <rsresistancewaterheatsource.h>
#include <rscondenserwaterheatsource.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <heatsourceconfiguration.h>
#include <heat-source-template.h>
#include <heating-logic-template.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model {
	namespace heat_source_configuration_ns {
		enum class HeatSourceType {
			RESISTANCE,
			CONDENSER,
			UNKNOWN
		};
		const static std::unordered_map<HeatSourceType, enum_info> HeatSourceType_info {
			{HeatSourceType::RESISTANCE, {"RESISTANCE", "Resistance", "Heat sources that operate by electrical resistance"}},
			{HeatSourceType::CONDENSER, {"CONDENSER", "Condenser", "Heat sources that operate by coolant condenser systems"}},
			{HeatSourceType::UNKNOWN, {"UNKNOWN", "None", "None"}}
		};
		enum class HeatingLogicType {
			STATE_OF_CHARGE_BASED,
			TEMPERATURE_BASED,
			UNKNOWN
		};
		const static std::unordered_map<HeatingLogicType, enum_info> HeatingLogicType_info {
			{HeatingLogicType::STATE_OF_CHARGE_BASED, {"STATE_OF_CHARGE_BASED", "STATE_OF_CHARGE_BASED", "State-of-charge based"}},
			{HeatingLogicType::TEMPERATURE_BASED, {"TEMPERATURE_BASED", "TEMPERATURE_BASED", "Temperature based"}},
			{HeatingLogicType::UNKNOWN, {"UNKNOWN", "None", "None"}}
		};
		enum class StandbyTemperatureLocation {
			TOP_OF_TANK,
			BOTTOM_OF_TANK,
			UNKNOWN
		};
		const static std::unordered_map<StandbyTemperatureLocation, enum_info> StandbyTemperatureLocation_info {
			{StandbyTemperatureLocation::TOP_OF_TANK, {"TOP_OF_TANK", "Top of tank", "Refers to top of tank."}},
			{StandbyTemperatureLocation::BOTTOM_OF_TANK, {"BOTTOM_OF_TANK", "Bottom of tank", "Refers to bottom of tank."}},
			{StandbyTemperatureLocation::UNKNOWN, {"UNKNOWN", "None", "None"}}
		};
		enum class ComparisonType {
			GREATER_THAN,
			LESS_THAN,
			UNKNOWN
		};
		const static std::unordered_map<ComparisonType, enum_info> ComparisonType_info {
			{ComparisonType::GREATER_THAN, {"GREATER_THAN", "Greater than", "Decision value is greater than reference value"}},
			{ComparisonType::LESS_THAN, {"LESS_THAN", "Less than", "Decision value is less than reference value"}},
			{ComparisonType::UNKNOWN, {"UNKNOWN", "None", "None"}}
		};
		inline std::shared_ptr<Courier::Courier> logger;
		void set_logger(std::shared_ptr<Courier::Courier> value);
		struct Schema {
			const static std::string_view schema_title;
			const static std::string_view schema_version;
			const static std::string_view schema_description;
		};
		struct HeatingLogic {
			heat_source_configuration_ns::HeatingLogicType heating_logic_type;
			std::unique_ptr<HeatingLogicTemplate> heating_logic;
			heat_source_configuration_ns::ComparisonType comparison_type;
			bool heating_logic_type_is_set;
			bool heating_logic_is_set;
			bool comparison_type_is_set;
			const static std::string_view heating_logic_type_units;
			const static std::string_view heating_logic_units;
			const static std::string_view comparison_type_units;
			const static std::string_view heating_logic_type_description;
			const static std::string_view heating_logic_description;
			const static std::string_view comparison_type_description;
			const static std::string_view heating_logic_type_name;
			const static std::string_view heating_logic_name;
			const static std::string_view comparison_type_name;
		};
		struct HeatSourceConfiguration {
			std::string id;
			heat_source_configuration_ns::HeatSourceType heat_source_type;
			std::unique_ptr<HeatSourceTemplate> heat_source;
			std::vector<double> heat_distribution;
			std::vector<heat_source_configuration_ns::HeatingLogic> turn_on_logic;
			std::vector<heat_source_configuration_ns::HeatingLogic> shut_off_logic;
			heat_source_configuration_ns::HeatingLogic standby_logic;
			std::string backup_heat_source_id;
			std::string followed_by_heat_source_id;
			std::string companion_heat_source_id;
			bool id_is_set;
			bool heat_source_type_is_set;
			bool heat_source_is_set;
			bool heat_distribution_is_set;
			bool turn_on_logic_is_set;
			bool shut_off_logic_is_set;
			bool standby_logic_is_set;
			bool backup_heat_source_id_is_set;
			bool followed_by_heat_source_id_is_set;
			bool companion_heat_source_id_is_set;
			const static std::string_view id_units;
			const static std::string_view heat_source_type_units;
			const static std::string_view heat_source_units;
			const static std::string_view heat_distribution_units;
			const static std::string_view turn_on_logic_units;
			const static std::string_view shut_off_logic_units;
			const static std::string_view standby_logic_units;
			const static std::string_view backup_heat_source_id_units;
			const static std::string_view followed_by_heat_source_id_units;
			const static std::string_view companion_heat_source_id_units;
			const static std::string_view id_description;
			const static std::string_view heat_source_type_description;
			const static std::string_view heat_source_description;
			const static std::string_view heat_distribution_description;
			const static std::string_view turn_on_logic_description;
			const static std::string_view shut_off_logic_description;
			const static std::string_view standby_logic_description;
			const static std::string_view backup_heat_source_id_description;
			const static std::string_view followed_by_heat_source_id_description;
			const static std::string_view companion_heat_source_id_description;
			const static std::string_view id_name;
			const static std::string_view heat_source_type_name;
			const static std::string_view heat_source_name;
			const static std::string_view heat_distribution_name;
			const static std::string_view turn_on_logic_name;
			const static std::string_view shut_off_logic_name;
			const static std::string_view standby_logic_name;
			const static std::string_view backup_heat_source_id_name;
			const static std::string_view followed_by_heat_source_id_name;
			const static std::string_view companion_heat_source_id_name;
		};
        struct WeightedDistribution {
            std::vector<double> normalized_height;
            std::vector<double> weight;
            bool normalized_height_is_set;
            bool weight_is_set;
            const static std::string_view normalized_height_units;
            const static std::string_view weight_units;
            const static std::string_view normalized_height_description;
            const static std::string_view weight_description;
            const static std::string_view normalized_height_name;
            const static std::string_view weight_name;
        };
        struct TemperatureBasedHeatingLogic : HeatingLogicTemplate {
            double absolute_temperature;
            double differential_temperature;
            heat_source_configuration_ns::StandbyTemperatureLocation standby_temperature_location;
            heat_source_configuration_ns::WeightedDistribution temperature_weight_distribution;
            bool absolute_temperature_is_set;
            bool differential_temperature_is_set;
            bool standby_temperature_location_is_set;
            bool temperature_weight_distribution_is_set;
            const static std::string_view absolute_temperature_units;
            const static std::string_view differential_temperature_units;
            const static std::string_view standby_temperature_location_units;
            const static std::string_view temperature_weight_distribution_units;
            const static std::string_view absolute_temperature_description;
            const static std::string_view differential_temperature_description;
            const static std::string_view standby_temperature_location_description;
            const static std::string_view temperature_weight_distribution_description;
            const static std::string_view absolute_temperature_name;
            const static std::string_view differential_temperature_name;
            const static std::string_view standby_temperature_location_name;
            const static std::string_view temperature_weight_distribution_name;
        };
		struct StateOfChargeBasedHeatingLogic : HeatingLogicTemplate {
			double decision_point;
			double minimum_useful_temperature;
			double hysteresis_fraction;
			bool uses_constant_mains;
			double constant_mains_temperature;
			bool decision_point_is_set;
			bool minimum_useful_temperature_is_set;
			bool hysteresis_fraction_is_set;
			bool uses_constant_mains_is_set;
			bool constant_mains_temperature_is_set;
			const static std::string_view decision_point_units;
			const static std::string_view minimum_useful_temperature_units;
			const static std::string_view hysteresis_fraction_units;
			const static std::string_view uses_constant_mains_units;
			const static std::string_view constant_mains_temperature_units;
			const static std::string_view decision_point_description;
			const static std::string_view minimum_useful_temperature_description;
			const static std::string_view hysteresis_fraction_description;
			const static std::string_view uses_constant_mains_description;
			const static std::string_view constant_mains_temperature_description;
			const static std::string_view decision_point_name;
			const static std::string_view minimum_useful_temperature_name;
			const static std::string_view hysteresis_fraction_name;
			const static std::string_view uses_constant_mains_name;
			const static std::string_view constant_mains_temperature_name;
		};
		NLOHMANN_JSON_SERIALIZE_ENUM (HeatSourceType, {
			{HeatSourceType::UNKNOWN, "UNKNOWN"},
			{HeatSourceType::RESISTANCE, "RESISTANCE"},
			{HeatSourceType::CONDENSER, "CONDENSER"},
		})
		NLOHMANN_JSON_SERIALIZE_ENUM (HeatingLogicType, {
			{HeatingLogicType::UNKNOWN, "UNKNOWN"},
			{HeatingLogicType::STATE_OF_CHARGE_BASED, "STATE_OF_CHARGE_BASED"},
			{HeatingLogicType::TEMPERATURE_BASED, "TEMPERATURE_BASED"},
		})
		NLOHMANN_JSON_SERIALIZE_ENUM (StandbyTemperatureLocation, {
			{StandbyTemperatureLocation::UNKNOWN, "UNKNOWN"},
			{StandbyTemperatureLocation::TOP_OF_TANK, "TOP_OF_TANK"},
			{StandbyTemperatureLocation::BOTTOM_OF_TANK, "BOTTOM_OF_TANK"},
		})
		NLOHMANN_JSON_SERIALIZE_ENUM (ComparisonType, {
			{ComparisonType::UNKNOWN, "UNKNOWN"},
			{ComparisonType::GREATER_THAN, "GREATER_THAN"},
			{ComparisonType::LESS_THAN, "LESS_THAN"},
		})
		void from_json(const nlohmann::json& j, HeatSourceConfiguration& x);
		void from_json(const nlohmann::json& j, WeightedDistribution& x);
		void from_json(const nlohmann::json& j, HeatingLogic& x);
		void from_json(const nlohmann::json& j, TemperatureBasedHeatingLogic& x);
		void from_json(const nlohmann::json& j, StateOfChargeBasedHeatingLogic& x);
	}
}
#endif
