#ifndef RSINTEGRATEDWATERHEATER_H_
#define RSINTEGRATEDWATERHEATER_H_
#include <ASHRAE205.h>
#include <RSTANK.h>
#include <RSRESISTANCEWATERHEATSOURCE.h>
#include <RSCONDENSERWATERHEATSOURCE.h>
#include <HeatSourceConfiguration.h>
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
			const static std::string_view schema_title;
			const static std::string_view schema_version;
			const static std::string_view schema_description;
		};
		struct ProductInformation {
			std::string manufacturer;
			bool manufacturer_is_set;
			const static std::string_view manufacturer_units;
			const static std::string_view manufacturer_description;
			const static std::string_view manufacturer_name;
			std::string model_number;
			bool model_number_is_set;
			const static std::string_view model_number_units;
			const static std::string_view model_number_description;
			const static std::string_view model_number_name;
		};
		struct Rating10CFR430 {
			std::string certified_reference_number;
			bool certified_reference_number_is_set;
			const static std::string_view certified_reference_number_units;
			const static std::string_view certified_reference_number_description;
			const static std::string_view certified_reference_number_name;
			double nominal_tank_volume;
			bool nominal_tank_volume_is_set;
			const static std::string_view nominal_tank_volume_units;
			const static std::string_view nominal_tank_volume_description;
			const static std::string_view nominal_tank_volume_name;
			double tested_first_hour_rating;
			bool tested_first_hour_rating_is_set;
			const static std::string_view tested_first_hour_rating_units;
			const static std::string_view tested_first_hour_rating_description;
			const static std::string_view tested_first_hour_rating_name;
			double tested_recovery_efficiency;
			bool tested_recovery_efficiency_is_set;
			const static std::string_view tested_recovery_efficiency_units;
			const static std::string_view tested_recovery_efficiency_description;
			const static std::string_view tested_recovery_efficiency_name;
			double tested_uniform_energy_factor;
			bool tested_uniform_energy_factor_is_set;
			const static std::string_view tested_uniform_energy_factor_units;
			const static std::string_view tested_uniform_energy_factor_description;
			const static std::string_view tested_uniform_energy_factor_name;
		};
		struct Description {
			rsintegratedwaterheater::ProductInformation product_information;
			bool product_information_is_set;
			const static std::string_view product_information_units;
			const static std::string_view product_information_description;
			const static std::string_view product_information_name;
			rsintegratedwaterheater::Rating10CFR430 rating_10_cfr_430;
			bool rating_10_cfr_430_is_set;
			const static std::string_view rating_10_cfr_430_units;
			const static std::string_view rating_10_cfr_430_description;
			const static std::string_view rating_10_cfr_430_name;
		};
		struct Performance {
			rstank::RSTANK tank;
			bool tank_is_set;
			const static std::string_view tank_units;
			const static std::string_view tank_description;
			const static std::string_view tank_name;
			std::vector<heat_source_configuration::HeatSourceConfiguration> heat_source_configurations;
			bool heat_source_configurations_is_set;
			const static std::string_view heat_source_configurations_units;
			const static std::string_view heat_source_configurations_description;
			const static std::string_view heat_source_configurations_name;
			std::string primary_heat_source_id;
			bool primary_heat_source_id_is_set;
			const static std::string_view primary_heat_source_id_units;
			const static std::string_view primary_heat_source_id_description;
			const static std::string_view primary_heat_source_id_name;
			double standby_power;
			bool standby_power_is_set;
			const static std::string_view standby_power_units;
			const static std::string_view standby_power_description;
			const static std::string_view standby_power_name;
		};
		struct RSINTEGRATEDWATERHEATER {
			core::Metadata metadata;
			bool metadata_is_set;
			const static std::string_view metadata_units;
			const static std::string_view metadata_description;
			const static std::string_view metadata_name;
			rsintegratedwaterheater::Description description;
			bool description_is_set;
			const static std::string_view description_units;
			const static std::string_view description_description;
			const static std::string_view description_name;
			rsintegratedwaterheater::Performance performance;
			bool performance_is_set;
			const static std::string_view performance_units;
			const static std::string_view performance_description;
			const static std::string_view performance_name;
		};
		void from_json(const nlohmann::json& j, RSINTEGRATEDWATERHEATER& x);
		void from_json(const nlohmann::json& j, Description& x);
		void from_json(const nlohmann::json& j, ProductInformation& x);
		void from_json(const nlohmann::json& j, Rating10CFR430& x);
		void from_json(const nlohmann::json& j, Performance& x);
	}
}
#endif
