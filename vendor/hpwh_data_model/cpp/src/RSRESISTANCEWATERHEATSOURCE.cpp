#include <RSRESISTANCEWATERHEATSOURCE.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace rsresistancewaterheatsource  {
	
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
			json_get<rsresistancewaterheatsource::ProductInformation>(j, logger.get(), "product_information", x.product_information, x.product_information_is_set, false);
		}
		void to_json(nlohmann::json& j, const Description& x) {
			json_set<rsresistancewaterheatsource::ProductInformation>(j, "product_information", x.product_information, x.product_information_is_set);
		}
		void from_json(const nlohmann::json& j, Performance& x) {
			json_get<double>(j, logger.get(), "input_power", x.input_power, x.input_power_is_set, true);
		}
		void to_json(nlohmann::json& j, const Performance& x) {
			json_set<double>(j, "input_power", x.input_power, x.input_power_is_set);
		}
		void from_json(const nlohmann::json& j, RSRESISTANCEWATERHEATSOURCE& x) {
			json_get<core::Metadata>(j, logger.get(), "metadata", x.metadata, x.metadata_is_set, true);
			json_get<rsresistancewaterheatsource::Description>(j, logger.get(), "description", x.description, x.description_is_set, false);
			json_get<rsresistancewaterheatsource::Performance>(j, logger.get(), "performance", x.performance, x.performance_is_set, true);
		}
		void to_json(nlohmann::json& j, const RSRESISTANCEWATERHEATSOURCE& x) {
			json_set<core::Metadata>(j, "metadata", x.metadata, x.metadata_is_set);
			json_set<rsresistancewaterheatsource::Description>(j, "description", x.description, x.description_is_set);
			json_set<rsresistancewaterheatsource::Performance>(j, "performance", x.performance, x.performance_is_set);
		}
	}
}

