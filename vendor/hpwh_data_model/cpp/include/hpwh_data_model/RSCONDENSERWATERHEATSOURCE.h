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
			static constexpr std::string_view schema_title = "Condenser Water Heat Source";
			static constexpr std::string_view schema_version = "0.1.0";
			static constexpr std::string_view schema_description = "Schema for ASHRAE 205 annex RSCONDENSERWATERHEATSOURCE: Condenser heat source";
		};
		struct ProductInformation {
			std::string manufacturer;
			bool manufacturer_is_set = false;
			static constexpr std::string_view manufacturer_units = "";
			static constexpr std::string_view manufacturer_description = "Manufacturer name";
			static constexpr std::string_view manufacturer_name = "manufacturer";
			std::string model_number;
			bool model_number_is_set = false;
			static constexpr std::string_view model_number_units = "";
			static constexpr std::string_view model_number_description = "Model number";
			static constexpr std::string_view model_number_name = "model_number";
		};
		struct Description {
			rscondenserwaterheatsource::ProductInformation product_information;
			bool product_information_is_set = false;
			static constexpr std::string_view product_information_units = "";
			static constexpr std::string_view product_information_description = "Data group describing product information";
			static constexpr std::string_view product_information_name = "product_information";
		};
		struct GridVariables {
			std::vector<double> evaporator_environment_dry_bulb_temperature;
			bool evaporator_environment_dry_bulb_temperature_is_set = false;
			static constexpr std::string_view evaporator_environment_dry_bulb_temperature_units = "K";
			static constexpr std::string_view evaporator_environment_dry_bulb_temperature_description = "Dry bulb temperature of the air entering the evaporator coil";
			static constexpr std::string_view evaporator_environment_dry_bulb_temperature_name = "evaporator_environment_dry_bulb_temperature";
			std::vector<double> heat_source_temperature;
			bool heat_source_temperature_is_set = false;
			static constexpr std::string_view heat_source_temperature_units = "K";
			static constexpr std::string_view heat_source_temperature_description = "Average water temperature at the heat source";
			static constexpr std::string_view heat_source_temperature_name = "heat_source_temperature";
		};
		struct LookupVariables {
			std::vector<double> input_power;
			bool input_power_is_set = false;
			static constexpr std::string_view input_power_units = "W";
			static constexpr std::string_view input_power_description = "Power draw from the compressor, evaporator fan, and any auxiliary power used by the units controls";
			static constexpr std::string_view input_power_name = "input_power";
			std::vector<double> heating_capacity;
			bool heating_capacity_is_set = false;
			static constexpr std::string_view heating_capacity_units = "W";
			static constexpr std::string_view heating_capacity_description = "Total heat added by the condenser to the adjacent water";
			static constexpr std::string_view heating_capacity_name = "heating_capacity";
		};
		struct PerformanceMap {
			rscondenserwaterheatsource::GridVariables grid_variables;
			bool grid_variables_is_set = false;
			static constexpr std::string_view grid_variables_units = "";
			static constexpr std::string_view grid_variables_description = "Data group defining the grid variables for heating performance";
			static constexpr std::string_view grid_variables_name = "grid_variables";
			rscondenserwaterheatsource::LookupVariables lookup_variables;
			bool lookup_variables_is_set = false;
			static constexpr std::string_view lookup_variables_units = "";
			static constexpr std::string_view lookup_variables_description = "Data group defining the lookup variables for heating performance";
			static constexpr std::string_view lookup_variables_name = "lookup_variables";
		};
		struct Performance {
			rscondenserwaterheatsource::PerformanceMap performance_map;
			bool performance_map_is_set = false;
			static constexpr std::string_view performance_map_units = "";
			static constexpr std::string_view performance_map_description = "Performance map";
			static constexpr std::string_view performance_map_name = "performance_map";
			double standby_power;
			bool standby_power_is_set = false;
			static constexpr std::string_view standby_power_units = "W";
			static constexpr std::string_view standby_power_description = "";
			static constexpr std::string_view standby_power_name = "standby_power";
			rscondenserwaterheatsource::CoilConfiguration coil_configuration;
			bool coil_configuration_is_set = false;
			static constexpr std::string_view coil_configuration_units = "";
			static constexpr std::string_view coil_configuration_description = "Coil configuration";
			static constexpr std::string_view coil_configuration_name = "coil_configuration";
			double maximum_refrigerant_temperature;
			bool maximum_refrigerant_temperature_is_set = false;
			static constexpr std::string_view maximum_refrigerant_temperature_units = "K";
			static constexpr std::string_view maximum_refrigerant_temperature_description = "Maximum temperature of the refrigerant entering the condenser";
			static constexpr std::string_view maximum_refrigerant_temperature_name = "maximum_refrigerant_temperature";
			double compressor_lockout_temperature_hysteresis;
			bool compressor_lockout_temperature_hysteresis_is_set = false;
			static constexpr std::string_view compressor_lockout_temperature_hysteresis_units = "K";
			static constexpr std::string_view compressor_lockout_temperature_hysteresis_description = "Hysteresis for compressor lockout";
			static constexpr std::string_view compressor_lockout_temperature_hysteresis_name = "compressor_lockout_temperature_hysteresis";
			bool use_defrost_map;
			bool use_defrost_map_is_set = false;
			static constexpr std::string_view use_defrost_map_units = "";
			static constexpr std::string_view use_defrost_map_description = "Use defrost map";
			static constexpr std::string_view use_defrost_map_name = "use_defrost_map";
		};
		struct RSCONDENSERWATERHEATSOURCE : ashrae205::HeatSourceTemplate {
			core::Metadata metadata;
			bool metadata_is_set = false;
			static constexpr std::string_view metadata_units = "";
			static constexpr std::string_view metadata_description = "Metadata data group";
			static constexpr std::string_view metadata_name = "metadata";
			rscondenserwaterheatsource::Description description;
			bool description_is_set = false;
			static constexpr std::string_view description_units = "";
			static constexpr std::string_view description_description = "Data group describing product and rating information";
			static constexpr std::string_view description_name = "description";
			rscondenserwaterheatsource::Performance performance;
			bool performance_is_set = false;
			static constexpr std::string_view performance_units = "";
			static constexpr std::string_view performance_description = "Data group containing performance information";
			static constexpr std::string_view performance_name = "performance";
		};
		NLOHMANN_JSON_SERIALIZE_ENUM (CoilConfiguration, {
			{CoilConfiguration::UNKNOWN, "UNKNOWN"},
			{CoilConfiguration::SUBMERGED, "SUBMERGED"},
			{CoilConfiguration::WRAPPED, "WRAPPED"}
		})
		void from_json(const nlohmann::json& j, RSCONDENSERWATERHEATSOURCE& x);
		void to_json(nlohmann::json& j, const RSCONDENSERWATERHEATSOURCE& x);
		void from_json(const nlohmann::json& j, Description& x);
		void to_json(nlohmann::json& j, const Description& x);
		void from_json(const nlohmann::json& j, ProductInformation& x);
		void to_json(nlohmann::json& j, const ProductInformation& x);
		void from_json(const nlohmann::json& j, Performance& x);
		void to_json(nlohmann::json& j, const Performance& x);
		void from_json(const nlohmann::json& j, PerformanceMap& x);
		void to_json(nlohmann::json& j, const PerformanceMap& x);
		void from_json(const nlohmann::json& j, GridVariables& x);
		void to_json(nlohmann::json& j, const GridVariables& x);
		void from_json(const nlohmann::json& j, LookupVariables& x);
		void to_json(nlohmann::json& j, const LookupVariables& x);
	}
}
#endif
