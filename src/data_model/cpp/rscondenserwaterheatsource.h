#ifndef RSCONDENSERWATERHEATSOURCE_H_
#define RSCONDENSERWATERHEATSOURCE_H_
#include <ashrae205.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <heat-source-base.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace data_model {
	namespace rscondenserwaterheatsource_ns {
		enum class CoilConfiguration {
			SUBMERGED,
			WRAPPED,
			EXTERNAL,
			UNKNOWN
		};
		const static std::unordered_map<CoilConfiguration, enum_info> CoilConfiguration_info {
			{CoilConfiguration::SUBMERGED, {"SUBMERGED", "Submerged", "Coil is submerged within the tank"}},
			{CoilConfiguration::WRAPPED, {"WRAPPED", "Wrapped", "Coil is wrapped around the tank interior"}},
			{CoilConfiguration::EXTERNAL, {"EXTERNAL", "External", "Coil is external to the tank"}},
			{CoilConfiguration::UNKNOWN, {"UNKNOWN", "None", "None"}}
		};
		inline std::shared_ptr<Courier::Courier> logger;
		struct Schema {
			const static std::string_view schema_title;
			const static std::string_view schema_version;
			const static std::string_view schema_description;
		};
		struct ProductInformation {
			std::string manufacturer;
			std::string model_number;
			bool manufacturer_is_set;
			bool model_number_is_set;
			const static std::string_view manufacturer_units;
			const static std::string_view model_number_units;
			const static std::string_view manufacturer_description;
			const static std::string_view model_number_description;
			const static std::string_view manufacturer_name;
			const static std::string_view model_number_name;
		};
		struct Description {
			rscondenserwaterheatsource_ns::ProductInformation product_information;
			bool product_information_is_set;
			const static std::string_view product_information_units;
			const static std::string_view product_information_description;
			const static std::string_view product_information_name;
		};
		struct PerformancePoint {
			double heat_source_temperature;
			std::vector<double> input_power_coefficients;
			std::vector<double> cop_coefficients;
			bool heat_source_temperature_is_set;
			bool input_power_coefficients_is_set;
			bool cop_coefficients_is_set;
			const static std::string_view heat_source_temperature_units;
			const static std::string_view input_power_coefficients_units;
			const static std::string_view cop_coefficients_units;
			const static std::string_view heat_source_temperature_description;
			const static std::string_view input_power_coefficients_description;
			const static std::string_view cop_coefficients_description;
			const static std::string_view heat_source_temperature_name;
			const static std::string_view input_power_coefficients_name;
			const static std::string_view cop_coefficients_name;
		};
		struct GridVariables {
			std::vector<double> evaporator_environment_temperature;
			std::vector<double> heat_source_temperature;
			bool evaporator_environment_temperature_is_set;
			bool heat_source_temperature_is_set;
			const static std::string_view evaporator_environment_temperature_units;
			const static std::string_view heat_source_temperature_units;
			const static std::string_view evaporator_environment_temperature_description;
			const static std::string_view heat_source_temperature_description;
			const static std::string_view evaporator_environment_temperature_name;
			const static std::string_view heat_source_temperature_name;
		};
		struct LookupVariables {
			std::vector<double> input_power;
			std::vector<double> cop;
			bool input_power_is_set;
			bool cop_is_set;
			const static std::string_view input_power_units;
			const static std::string_view cop_units;
			const static std::string_view input_power_description;
			const static std::string_view cop_description;
			const static std::string_view input_power_name;
			const static std::string_view cop_name;
		};
		struct PerformanceMap {
			rscondenserwaterheatsource_ns::GridVariables grid_variables;
			rscondenserwaterheatsource_ns::LookupVariables lookup_variables;
			bool grid_variables_is_set;
			bool lookup_variables_is_set;
			const static std::string_view grid_variables_units;
			const static std::string_view lookup_variables_units;
			const static std::string_view grid_variables_description;
			const static std::string_view lookup_variables_description;
			const static std::string_view grid_variables_name;
			const static std::string_view lookup_variables_name;
		};
		struct Performance {
			std::vector<rscondenserwaterheatsource_ns::PerformancePoint> performance_points;
			rscondenserwaterheatsource_ns::PerformanceMap performance_map;
			double standby_power;
			rscondenserwaterheatsource_ns::CoilConfiguration coil_configuration;
			bool use_defrost_map;
			bool performance_points_is_set;
			bool performance_map_is_set;
			bool standby_power_is_set;
			bool coil_configuration_is_set;
			bool use_defrost_map_is_set;
			const static std::string_view performance_points_units;
			const static std::string_view performance_map_units;
			const static std::string_view standby_power_units;
			const static std::string_view coil_configuration_units;
			const static std::string_view use_defrost_map_units;
			const static std::string_view performance_points_description;
			const static std::string_view performance_map_description;
			const static std::string_view standby_power_description;
			const static std::string_view coil_configuration_description;
			const static std::string_view use_defrost_map_description;
			const static std::string_view performance_points_name;
			const static std::string_view performance_map_name;
			const static std::string_view standby_power_name;
			const static std::string_view coil_configuration_name;
			const static std::string_view use_defrost_map_name;
		};
		struct RSCONDENSERWATERHEATSOURCE : HeatSourceBase {
			core_ns::Metadata metadata;
			rscondenserwaterheatsource_ns::Description description;
			rscondenserwaterheatsource_ns::Performance performance;
			bool metadata_is_set;
			bool description_is_set;
			bool performance_is_set;
			const static std::string_view metadata_units;
			const static std::string_view description_units;
			const static std::string_view performance_units;
			const static std::string_view metadata_description;
			const static std::string_view description_description;
			const static std::string_view performance_description;
			const static std::string_view metadata_name;
			const static std::string_view description_name;
			const static std::string_view performance_name;
		};
		NLOHMANN_JSON_SERIALIZE_ENUM (CoilConfiguration, {
			{CoilConfiguration::UNKNOWN, "UNKNOWN"},
			{CoilConfiguration::SUBMERGED, "SUBMERGED"},
			{CoilConfiguration::WRAPPED, "WRAPPED"},
			{CoilConfiguration::EXTERNAL, "EXTERNAL"},
		})
		void from_json (const nlohmann::json& j, RSCONDENSERWATERHEATSOURCE& x);
		void from_json (const nlohmann::json& j, Description& x);
		void from_json (const nlohmann::json& j, ProductInformation& x);
		void from_json (const nlohmann::json& j, Performance& x);
		void from_json (const nlohmann::json& j, PerformancePoint& x);
		void from_json (const nlohmann::json& j, PerformanceMap& x);
		void from_json (const nlohmann::json& j, GridVariables& x);
		void from_json (const nlohmann::json& j, LookupVariables& x);
	}
}
#endif
