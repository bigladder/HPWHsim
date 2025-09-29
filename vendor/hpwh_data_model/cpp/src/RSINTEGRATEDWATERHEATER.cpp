#include <RSINTEGRATEDWATERHEATER.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace rsintegratedwaterheater  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		void from_json(const nlohmann::json& j, ProductInformation& x) {
			json_get<std::string>(j, logger.get(), "manufacturer", x.manufacturer, x.manufacturer_is_set, true);
			json_get<std::string>(j, logger.get(), "model_number", x.model_number, x.model_number_is_set, true);
		}
		void to_json(nlohmann::json& j, const ProductInformation& x) {
			json_set<std::string>(j, "manufacturer", x.manufacturer, x.manufacturer_is_set);
			json_set<std::string>(j, "model_number", x.model_number, x.model_number_is_set);
		}
		void from_json(const nlohmann::json& j, Rating10CFR430& x) {
			json_get<std::string>(j, logger.get(), "certified_reference_number", x.certified_reference_number, x.certified_reference_number_is_set, false);
			json_get<double>(j, logger.get(), "nominal_tank_volume", x.nominal_tank_volume, x.nominal_tank_volume_is_set, false);
			json_get<double>(j, logger.get(), "first_hour_rating", x.first_hour_rating, x.first_hour_rating_is_set, false);
			json_get<double>(j, logger.get(), "recovery_efficiency", x.recovery_efficiency, x.recovery_efficiency_is_set, false);
			json_get<double>(j, logger.get(), "uniform_energy_factor", x.uniform_energy_factor, x.uniform_energy_factor_is_set, false);
		}
		void to_json(nlohmann::json& j, const Rating10CFR430& x) {
			json_set<std::string>(j, "certified_reference_number", x.certified_reference_number, x.certified_reference_number_is_set);
			json_set<double>(j, "nominal_tank_volume", x.nominal_tank_volume, x.nominal_tank_volume_is_set);
			json_set<double>(j, "first_hour_rating", x.first_hour_rating, x.first_hour_rating_is_set);
			json_set<double>(j, "recovery_efficiency", x.recovery_efficiency, x.recovery_efficiency_is_set);
			json_set<double>(j, "uniform_energy_factor", x.uniform_energy_factor, x.uniform_energy_factor_is_set);
		}
		void from_json(const nlohmann::json& j, Description& x) {
			json_get<rsintegratedwaterheater::ProductInformation>(j, logger.get(), "product_information", x.product_information, x.product_information_is_set, false);
			json_get<rsintegratedwaterheater::Rating10CFR430>(j, logger.get(), "rating_10_cfr_430", x.rating_10_cfr_430, x.rating_10_cfr_430_is_set, false);
		}
		void to_json(nlohmann::json& j, const Description& x) {
			json_set<rsintegratedwaterheater::ProductInformation>(j, "product_information", x.product_information, x.product_information_is_set);
			json_set<rsintegratedwaterheater::Rating10CFR430>(j, "rating_10_cfr_430", x.rating_10_cfr_430, x.rating_10_cfr_430_is_set);
		}
		void from_json(const nlohmann::json& j, Performance& x) {
			json_get<rstank::RSTANK>(j, logger.get(), "tank", x.tank, x.tank_is_set, true);
			json_get<std::vector<heat_source_configuration::HeatSourceConfiguration>>(j, logger.get(), "heat_source_configurations", x.heat_source_configurations, x.heat_source_configurations_is_set, false);
			json_get<std::string>(j, logger.get(), "primary_heat_source_id", x.primary_heat_source_id, x.primary_heat_source_id_is_set, false);
			json_get<double>(j, logger.get(), "standby_power", x.standby_power, x.standby_power_is_set, false);
		}
		void to_json(nlohmann::json& j, const Performance& x) {
			json_set<rstank::RSTANK>(j, "tank", x.tank, x.tank_is_set);
			json_set<std::vector<heat_source_configuration::HeatSourceConfiguration>>(j, "heat_source_configurations", x.heat_source_configurations, x.heat_source_configurations_is_set);
			json_set<std::string>(j, "primary_heat_source_id", x.primary_heat_source_id, x.primary_heat_source_id_is_set);
			json_set<double>(j, "standby_power", x.standby_power, x.standby_power_is_set);
		}
		void from_json(const nlohmann::json& j, RSINTEGRATEDWATERHEATER& x) {
			json_get<core::Metadata>(j, logger.get(), "metadata", x.metadata, x.metadata_is_set, true);
			json_get<rsintegratedwaterheater::Description>(j, logger.get(), "description", x.description, x.description_is_set, false);
			json_get<rsintegratedwaterheater::Performance>(j, logger.get(), "performance", x.performance, x.performance_is_set, true);
		}
		void to_json(nlohmann::json& j, const RSINTEGRATEDWATERHEATER& x) {
			json_set<core::Metadata>(j, "metadata", x.metadata, x.metadata_is_set);
			json_set<rsintegratedwaterheater::Description>(j, "description", x.description, x.description_is_set);
			json_set<rsintegratedwaterheater::Performance>(j, "performance", x.performance, x.performance_is_set);
		}
	}
}

