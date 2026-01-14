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
			static constexpr std::string_view schema_title = "Water Tank";
			static constexpr std::string_view schema_version = "0.1.0";
			static constexpr std::string_view schema_description = "Schema for ASHRAE 205 annex RSTANK: Water tank";
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
			rstank::ProductInformation product_information;
			bool product_information_is_set = false;
			static constexpr std::string_view product_information_units = "";
			static constexpr std::string_view product_information_description = "Data group describing product information";
			static constexpr std::string_view product_information_name = "product_information";
		};
		struct Performance {
			double volume;
			bool volume_is_set = false;
			static constexpr std::string_view volume_units = "m3";
			static constexpr std::string_view volume_description = "";
			static constexpr std::string_view volume_name = "volume";
			double diameter;
			bool diameter_is_set = false;
			static constexpr std::string_view diameter_units = "m";
			static constexpr std::string_view diameter_description = "";
			static constexpr std::string_view diameter_name = "diameter";
			double ua;
			bool ua_is_set = false;
			static constexpr std::string_view ua_units = "W/K";
			static constexpr std::string_view ua_description = "";
			static constexpr std::string_view ua_name = "ua";
			double fittings_ua;
			bool fittings_ua_is_set = false;
			static constexpr std::string_view fittings_ua_units = "W/K";
			static constexpr std::string_view fittings_ua_description = "";
			static constexpr std::string_view fittings_ua_name = "fittings_ua";
			double bottom_fraction_of_tank_mixing_on_draw;
			bool bottom_fraction_of_tank_mixing_on_draw_is_set = false;
			static constexpr std::string_view bottom_fraction_of_tank_mixing_on_draw_units = "-";
			static constexpr std::string_view bottom_fraction_of_tank_mixing_on_draw_description = "Bottom fraction of the tank that should mix during draws";
			static constexpr std::string_view bottom_fraction_of_tank_mixing_on_draw_name = "bottom_fraction_of_tank_mixing_on_draw";
			double heat_exchanger_effectiveness;
			bool heat_exchanger_effectiveness_is_set = false;
			static constexpr std::string_view heat_exchanger_effectiveness_units = "-";
			static constexpr std::string_view heat_exchanger_effectiveness_description = "Effectiveness for heat exchange";
			static constexpr std::string_view heat_exchanger_effectiveness_name = "heat_exchanger_effectiveness";
		};
		struct RSTANK {
			core::Metadata metadata;
			bool metadata_is_set = false;
			static constexpr std::string_view metadata_units = "";
			static constexpr std::string_view metadata_description = "Metadata data group";
			static constexpr std::string_view metadata_name = "metadata";
			rstank::Description description;
			bool description_is_set = false;
			static constexpr std::string_view description_units = "";
			static constexpr std::string_view description_description = "Data group describing product and rating information";
			static constexpr std::string_view description_name = "description";
			rstank::Performance performance;
			bool performance_is_set = false;
			static constexpr std::string_view performance_units = "";
			static constexpr std::string_view performance_description = "Data group containing performance information";
			static constexpr std::string_view performance_name = "performance";
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
