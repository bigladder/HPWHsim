#ifndef RSCONDENSERWATERHEATSOURCE_H_
#define RSCONDENSERWATERHEATSOURCE_H_
#include <ASHRAE205.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <core.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model {
	namespace rscondenserwaterheatsource {
		enum class CoilConfiguration {
			SUBMERGED,
			WRAPPED,
			UNKNOWN
		};
		const static std::unordered_map<CoilConfiguration, enum_info> CoilConfiguration_info {
			{CoilConfiguration::SUBMERGED, {"SUBMERGED", "Submerged", "Coil is submerged within the tank"}},
			{CoilConfiguration::WRAPPED, {"WRAPPED", "Wrapped", "Coil is wrapped around the tank interior"}},
			{CoilConfiguration::UNKNOWN, {"UNKNOWN", "None", "None"}}
		};
		inline std::shared_ptr<Courier::Courier> logger;
		void set_logger(std::shared_ptr<Courier::Courier> value);
		struct Schema {
			const static std::string_view schema_title;
			const static std::string_view schema_version;
			const static std::string_view schema_description;
		};
		struct ProductInformation {
			std::string manufacturer;
			bool manufacturer_is_set = false;
			const static std::string_view manufacturer_units;
			const static std::string_view manufacturer_description;
			const static std::string_view manufacturer_name;
			std::string model_number;
			bool model_number_is_set = false;
			const static std::string_view model_number_units;
			const static std::string_view model_number_description;
			const static std::string_view model_number_name;
		};
		struct Description {
			rscondenserwaterheatsource::ProductInformation product_information;
			bool product_information_is_set = false;
			const static std::string_view product_information_units;
			const static std::string_view product_information_description;
			const static std::string_view product_information_name;
		};
		struct GridVariables {
			std::vector<double> evaporator_environment_dry_bulb_temperature;
			bool evaporator_environment_dry_bulb_temperature_is_set = false;
			const static std::string_view evaporator_environment_dry_bulb_temperature_units;
			const static std::string_view evaporator_environment_dry_bulb_temperature_description;
			const static std::string_view evaporator_environment_dry_bulb_temperature_name;
			std::vector<double> heat_source_temperature;
			bool heat_source_temperature_is_set = false;
			const static std::string_view heat_source_temperature_units;
			const static std::string_view heat_source_temperature_description;
			const static std::string_view heat_source_temperature_name;
		};
		struct LookupVariables {
			std::vector<double> input_power;
			bool input_power_is_set = false;
			const static std::string_view input_power_units;
			const static std::string_view input_power_description;
			const static std::string_view input_power_name;
			std::vector<double> heating_capacity;
			bool heating_capacity_is_set = false;
			const static std::string_view heating_capacity_units;
			const static std::string_view heating_capacity_description;
			const static std::string_view heating_capacity_name;
		};
		struct PerformanceMap {
			rscondenserwaterheatsource::GridVariables grid_variables;
			bool grid_variables_is_set = false;
			const static std::string_view grid_variables_units;
			const static std::string_view grid_variables_description;
			const static std::string_view grid_variables_name;
			rscondenserwaterheatsource::LookupVariables lookup_variables;
			bool lookup_variables_is_set = false;
			const static std::string_view lookup_variables_units;
			const static std::string_view lookup_variables_description;
			const static std::string_view lookup_variables_name;
		};
		struct Performance {
			rscondenserwaterheatsource::PerformanceMap performance_map;
			bool performance_map_is_set = false;
			const static std::string_view performance_map_units;
			const static std::string_view performance_map_description;
			const static std::string_view performance_map_name;
			double standby_power;
			bool standby_power_is_set = false;
			const static std::string_view standby_power_units;
			const static std::string_view standby_power_description;
			const static std::string_view standby_power_name;
			rscondenserwaterheatsource::CoilConfiguration coil_configuration;
			bool coil_configuration_is_set = false;
			const static std::string_view coil_configuration_units;
			const static std::string_view coil_configuration_description;
			const static std::string_view coil_configuration_name;
			double maximum_refrigerant_temperature;
			bool maximum_refrigerant_temperature_is_set = false;
			const static std::string_view maximum_refrigerant_temperature_units;
			const static std::string_view maximum_refrigerant_temperature_description;
			const static std::string_view maximum_refrigerant_temperature_name;
			double compressor_lockout_temperature_hysteresis;
			bool compressor_lockout_temperature_hysteresis_is_set = false;
			const static std::string_view compressor_lockout_temperature_hysteresis_units;
			const static std::string_view compressor_lockout_temperature_hysteresis_description;
			const static std::string_view compressor_lockout_temperature_hysteresis_name;
			bool use_defrost_map;
			bool use_defrost_map_is_set = false;
			const static std::string_view use_defrost_map_units;
			const static std::string_view use_defrost_map_description;
			const static std::string_view use_defrost_map_name;
		};
		struct RSCONDENSERWATERHEATSOURCE : ashrae205::HeatSourceTemplate {
			core::Metadata metadata;
			bool metadata_is_set = false;
			const static std::string_view metadata_units;
			const static std::string_view metadata_description;
			const static std::string_view metadata_name;
			rscondenserwaterheatsource::Description description;
			bool description_is_set = false;
			const static std::string_view description_units;
			const static std::string_view description_description;
			const static std::string_view description_name;
			rscondenserwaterheatsource::Performance performance;
			bool performance_is_set = false;
			const static std::string_view performance_units;
			const static std::string_view performance_description;
			const static std::string_view performance_name;
		};
		NLOHMANN_JSON_SERIALIZE_ENUM (CoilConfiguration, {
			{CoilConfiguration::UNKNOWN, "UNKNOWN"},
			{CoilConfiguration::SUBMERGED, "SUBMERGED"},
			{CoilConfiguration::WRAPPED, "WRAPPED"}
		})
		void from_json(const nlohmann::json& j, RSCONDENSERWATERHEATSOURCE& x);
		void from_json(const nlohmann::json& j, Description& x);
		void from_json(const nlohmann::json& j, ProductInformation& x);
		void from_json(const nlohmann::json& j, Performance& x);
		void from_json(const nlohmann::json& j, PerformanceMap& x);
		void from_json(const nlohmann::json& j, GridVariables& x);
		void from_json(const nlohmann::json& j, LookupVariables& x);
	}
}
#endif
