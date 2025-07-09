#ifndef RSINTEGRATEDWATERHEATER_H_
#define RSINTEGRATEDWATERHEATER_H_
#include <ASHRAE205.h>
#include <RSTANK.h>
#include <RSRESISTANCEWATERHEATSOURCE.h>
#include <RSCONDENSERWATERHEATSOURCE.h>
#include <HeatSourceConfiguration.h>
#include <RSAIRTOWATERHEATPUMP.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <core.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model {
	namespace rsintegratedwaterheater {
		inline std::shared_ptr<Courier::Courier> logger;
		void set_logger(std::shared_ptr<Courier::Courier> value);
		struct Schema {
			static constexpr std::string_view schema_title = "Integrated Heat-Pump Water Heater";
			static constexpr std::string_view schema_version = "0.1.0";
			static constexpr std::string_view schema_description = "Schema for ASHRAE 205 annex RSINTEGRATEDWATERHEATER: Integrated Heat-Pump Water Heater";
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
		struct Rating10CFR430 {
			std::string certified_reference_number;
			bool certified_reference_number_is_set = false;
			static constexpr std::string_view certified_reference_number_units = "";
			static constexpr std::string_view certified_reference_number_description = "AHRI certified reference number";
			static constexpr std::string_view certified_reference_number_name = "certified_reference_number";
			double nominal_tank_volume;
			bool nominal_tank_volume_is_set = false;
			static constexpr std::string_view nominal_tank_volume_units = "gal";
			static constexpr std::string_view nominal_tank_volume_description = "Nominal storage capacity of the tank";
			static constexpr std::string_view nominal_tank_volume_name = "nominal_tank_volume";
			double first_hour_rating;
			bool first_hour_rating_is_set = false;
			static constexpr std::string_view first_hour_rating_units = "gal/h";
			static constexpr std::string_view first_hour_rating_description = "First-hour rating";
			static constexpr std::string_view first_hour_rating_name = "first_hour_rating";
			double recovery_efficiency;
			bool recovery_efficiency_is_set = false;
			static constexpr std::string_view recovery_efficiency_units = "-";
			static constexpr std::string_view recovery_efficiency_description = "Recovery Efficiency";
			static constexpr std::string_view recovery_efficiency_name = "recovery_efficiency";
			double uniform_energy_factor;
			bool uniform_energy_factor_is_set = false;
			static constexpr std::string_view uniform_energy_factor_units = "-";
			static constexpr std::string_view uniform_energy_factor_description = "Uniform Energy Factor (UEF)";
			static constexpr std::string_view uniform_energy_factor_name = "uniform_energy_factor";
		};
		struct Description {
			rsintegratedwaterheater::ProductInformation product_information;
			bool product_information_is_set = false;
			static constexpr std::string_view product_information_units = "";
			static constexpr std::string_view product_information_description = "Data group describing product information";
			static constexpr std::string_view product_information_name = "product_information";
			rsintegratedwaterheater::Rating10CFR430 rating_10_cfr_430;
			bool rating_10_cfr_430_is_set = false;
			static constexpr std::string_view rating_10_cfr_430_units = "";
			static constexpr std::string_view rating_10_cfr_430_description = "Data group containing information relevant to products rated under AHRI 10 CFR Part 430";
			static constexpr std::string_view rating_10_cfr_430_name = "rating_10_cfr_430";
		};
		struct Performance {
			rstank::RSTANK tank;
			bool tank_is_set = false;
			static constexpr std::string_view tank_units = "";
			static constexpr std::string_view tank_description = "The corresponding Standard 205 tank representation";
			static constexpr std::string_view tank_name = "tank";
			std::vector<heat_source_configuration::HeatSourceConfiguration> heat_source_configurations;
			bool heat_source_configurations_is_set = false;
			static constexpr std::string_view heat_source_configurations_units = "";
			static constexpr std::string_view heat_source_configurations_description = "Describes how the heat sources are configured within the tank";
			static constexpr std::string_view heat_source_configurations_name = "heat_source_configurations";
			std::string primary_heat_source_id;
			bool primary_heat_source_id_is_set = false;
			static constexpr std::string_view primary_heat_source_id_units = "";
			static constexpr std::string_view primary_heat_source_id_description = "Turns on independently of other heat sources";
			static constexpr std::string_view primary_heat_source_id_name = "primary_heat_source_id";
			double standby_power;
			bool standby_power_is_set = false;
			static constexpr std::string_view standby_power_units = "W";
			static constexpr std::string_view standby_power_description = "Power drawn when system is in standby mode";
			static constexpr std::string_view standby_power_name = "standby_power";
		};
		struct RSINTEGRATEDWATERHEATER {
			core::Metadata metadata;
			bool metadata_is_set = false;
			static constexpr std::string_view metadata_units = "";
			static constexpr std::string_view metadata_description = "Metadata data group";
			static constexpr std::string_view metadata_name = "metadata";
			rsintegratedwaterheater::Description description;
			bool description_is_set = false;
			static constexpr std::string_view description_units = "";
			static constexpr std::string_view description_description = "Data group describing product and rating information";
			static constexpr std::string_view description_name = "description";
			rsintegratedwaterheater::Performance performance;
			bool performance_is_set = false;
			static constexpr std::string_view performance_units = "";
			static constexpr std::string_view performance_description = "Data group containing performance information";
			static constexpr std::string_view performance_name = "performance";
		};
		void from_json(const nlohmann::json& j, RSINTEGRATEDWATERHEATER& x);
		void to_json(nlohmann::json& j, const RSINTEGRATEDWATERHEATER& x);
		void from_json(const nlohmann::json& j, Description& x);
		void to_json(nlohmann::json& j, const Description& x);
		void from_json(const nlohmann::json& j, ProductInformation& x);
		void to_json(nlohmann::json& j, const ProductInformation& x);
		void from_json(const nlohmann::json& j, Rating10CFR430& x);
		void to_json(nlohmann::json& j, const Rating10CFR430& x);
		void from_json(const nlohmann::json& j, Performance& x);
		void to_json(nlohmann::json& j, const Performance& x);
	}
}
#endif
