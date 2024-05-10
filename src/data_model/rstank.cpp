#include <rstank.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace rstank_ns  {

		const std::string_view Schema::schema_title = "Water Tank";

		const std::string_view Schema::schema_version = "0.1.0";

		const std::string_view Schema::schema_description = "Schema for ASHRAE 205 annex RSTANK: Water tank";

		void from_json(const nlohmann::json& j, ProductInformation& x) {
			json_get<std::string>(j, *RSTANK::logger, "manufacturer", x.manufacturer, x.manufacturer_is_set, true);
			json_get<std::string>(j, *RSTANK::logger, "model_number", x.model_number, x.model_number_is_set, true);
		}
		const std::string_view ProductInformation::manufacturer_units = "";

		const std::string_view ProductInformation::model_number_units = "";

		const std::string_view ProductInformation::manufacturer_description = "Manufacturer name";

		const std::string_view ProductInformation::model_number_description = "Model number";

		const std::string_view ProductInformation::manufacturer_name = "manufacturer";

		const std::string_view ProductInformation::model_number_name = "model_number";

		void from_json(const nlohmann::json& j, Description& x) {
			json_get<rstank_ns::ProductInformation>(j, *RSTANK::logger, "product_information", x.product_information, x.product_information_is_set, false);
		}
		const std::string_view Description::product_information_units = "";

		const std::string_view Description::product_information_description = "Data group describing product information";

		const std::string_view Description::product_information_name = "product_information";

		void from_json(const nlohmann::json& j, Performance& x) {
			json_get<double>(j, *RSTANK::logger, "volume", x.volume, x.volume_is_set, true);
			json_get<double>(j, *RSTANK::logger, "diameter", x.diameter, x.diameter_is_set, false);
			json_get<double>(j, *RSTANK::logger, "ua", x.ua, x.ua_is_set, true);
			json_get<double>(j, *RSTANK::logger, "fittings_ua", x.fittings_ua, x.fittings_ua_is_set, false);
			json_get<double>(j, *RSTANK::logger, "bottom_fraction_of_tank_mixing_on_draw", x.bottom_fraction_of_tank_mixing_on_draw, x.bottom_fraction_of_tank_mixing_on_draw_is_set, false);
		}
		const std::string_view Performance::volume_units = "m";

		const std::string_view Performance::diameter_units = "m";

		const std::string_view Performance::ua_units = "W";

		const std::string_view Performance::fittings_ua_units = "W";

		const std::string_view Performance::bottom_fraction_of_tank_mixing_on_draw_units = "-";

		const std::string_view Performance::volume_description = "";

		const std::string_view Performance::diameter_description = "";

		const std::string_view Performance::ua_description = "";

		const std::string_view Performance::fittings_ua_description = "";

		const std::string_view Performance::bottom_fraction_of_tank_mixing_on_draw_description = "Bottom fraction of the tank that should mix during draws (simulation only?)";

		const std::string_view Performance::volume_name = "volume";

		const std::string_view Performance::diameter_name = "diameter";

		const std::string_view Performance::ua_name = "ua";

		const std::string_view Performance::fittings_ua_name = "fittings_ua";

		const std::string_view Performance::bottom_fraction_of_tank_mixing_on_draw_name = "bottom_fraction_of_tank_mixing_on_draw";

		void from_json(const nlohmann::json& j, RSTANK& x) {
			json_get<core_ns::Metadata>(j, *RSTANK::logger, "metadata", x.metadata, x.metadata_is_set, true);
			json_get<rstank_ns::Description>(j, *RSTANK::logger, "description", x.description, x.description_is_set, false);
			json_get<rstank_ns::Performance>(j, *RSTANK::logger, "performance", x.performance, x.performance_is_set, true);
		}
		const std::string_view RSTANK::metadata_units = "";

		const std::string_view RSTANK::description_units = "";

		const std::string_view RSTANK::performance_units = "";

		const std::string_view RSTANK::metadata_description = "Metadata data group";

		const std::string_view RSTANK::description_description = "Data group describing product and rating information";

		const std::string_view RSTANK::performance_description = "Data group containing performance information";

		const std::string_view RSTANK::metadata_name = "metadata";

		const std::string_view RSTANK::description_name = "description";

		const std::string_view RSTANK::performance_name = "performance";

	}
}

