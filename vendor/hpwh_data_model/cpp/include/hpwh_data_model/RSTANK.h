#ifndef RSTANK_H_
#define RSTANK_H_
#include <ASHRAE205.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <core.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model {
	namespace rstank {
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
			rstank::ProductInformation product_information;
			bool product_information_is_set = false;
			const static std::string_view product_information_units;
			const static std::string_view product_information_description;
			const static std::string_view product_information_name;
		};
		struct Performance {
			double volume;
			bool volume_is_set = false;
			const static std::string_view volume_units;
			const static std::string_view volume_description;
			const static std::string_view volume_name;
			double diameter;
			bool diameter_is_set = false;
			const static std::string_view diameter_units;
			const static std::string_view diameter_description;
			const static std::string_view diameter_name;
			double ua;
			bool ua_is_set = false;
			const static std::string_view ua_units;
			const static std::string_view ua_description;
			const static std::string_view ua_name;
			double fittings_ua;
			bool fittings_ua_is_set = false;
			const static std::string_view fittings_ua_units;
			const static std::string_view fittings_ua_description;
			const static std::string_view fittings_ua_name;
			double bottom_fraction_of_tank_mixing_on_draw;
			bool bottom_fraction_of_tank_mixing_on_draw_is_set = false;
			const static std::string_view bottom_fraction_of_tank_mixing_on_draw_units;
			const static std::string_view bottom_fraction_of_tank_mixing_on_draw_description;
			const static std::string_view bottom_fraction_of_tank_mixing_on_draw_name;
			double heat_exchanger_effectiveness;
			bool heat_exchanger_effectiveness_is_set = false;
			const static std::string_view heat_exchanger_effectiveness_units;
			const static std::string_view heat_exchanger_effectiveness_description;
			const static std::string_view heat_exchanger_effectiveness_name;
		};
		struct RSTANK {
			core::Metadata metadata;
			bool metadata_is_set = false;
			const static std::string_view metadata_units;
			const static std::string_view metadata_description;
			const static std::string_view metadata_name;
			rstank::Description description;
			bool description_is_set = false;
			const static std::string_view description_units;
			const static std::string_view description_description;
			const static std::string_view description_name;
			rstank::Performance performance;
			bool performance_is_set = false;
			const static std::string_view performance_units;
			const static std::string_view performance_description;
			const static std::string_view performance_name;
		};
		void from_json(const nlohmann::json& j, RSTANK& x);
		void to_json(nlohmann::json& j, const RSTANK& x);
		void from_json(const nlohmann::json& j, Description& x);
		void to_json(nlohmann::json& j, const Description& x);
		void from_json(const nlohmann::json& j, ProductInformation& x);
		void to_json(nlohmann::json& j, const ProductInformation& x);
		void from_json(const nlohmann::json& j, Performance& x);
		void to_json(nlohmann::json& j, const Performance& x);
	}
}
#endif
