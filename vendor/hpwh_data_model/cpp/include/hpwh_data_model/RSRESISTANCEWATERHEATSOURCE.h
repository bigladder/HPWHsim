#ifndef RSRESISTANCEWATERHEATSOURCE_H_
#define RSRESISTANCEWATERHEATSOURCE_H_
#include <ASHRAE205.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <core.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model {
	namespace rsresistancewaterheatsource {
		inline std::shared_ptr<Courier::Courier> logger;
		void set_logger(std::shared_ptr<Courier::Courier> value);
		struct Schema {
			static constexpr std::string_view schema_title = "Resistance Water Heat Source";
			static constexpr std::string_view schema_version = "0.1.0";
			static constexpr std::string_view schema_description = "Schema for ASHRAE 205 annex RSRESISTANCEWATERHEATSOURCE: Resistance heat source";
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
			rsresistancewaterheatsource::ProductInformation product_information;
			bool product_information_is_set = false;
			static constexpr std::string_view product_information_units = "";
			static constexpr std::string_view product_information_description = "Data group describing product information";
			static constexpr std::string_view product_information_name = "product_information";
		};
		struct Performance {
			double input_power;
			bool input_power_is_set = false;
			static constexpr std::string_view input_power_units = "W";
			static constexpr std::string_view input_power_description = "Input power";
			static constexpr std::string_view input_power_name = "input_power";
		};
		struct RSRESISTANCEWATERHEATSOURCE : ashrae205::HeatSourceTemplate {
			core::Metadata metadata;
			bool metadata_is_set = false;
			static constexpr std::string_view metadata_units = "";
			static constexpr std::string_view metadata_description = "Metadata data group";
			static constexpr std::string_view metadata_name = "metadata";
			rsresistancewaterheatsource::Description description;
			bool description_is_set = false;
			static constexpr std::string_view description_units = "";
			static constexpr std::string_view description_description = "Data group describing product and rating information";
			static constexpr std::string_view description_name = "description";
			rsresistancewaterheatsource::Performance performance;
			bool performance_is_set = false;
			static constexpr std::string_view performance_units = "";
			static constexpr std::string_view performance_description = "Data group containing performance information";
			static constexpr std::string_view performance_name = "performance";
		};
		void from_json(const nlohmann::json& j, RSRESISTANCEWATERHEATSOURCE& x);
		void to_json(nlohmann::json& j, const RSRESISTANCEWATERHEATSOURCE& x);
		void from_json(const nlohmann::json& j, Description& x);
		void to_json(nlohmann::json& j, const Description& x);
		void from_json(const nlohmann::json& j, ProductInformation& x);
		void to_json(nlohmann::json& j, const ProductInformation& x);
		void from_json(const nlohmann::json& j, Performance& x);
		void to_json(nlohmann::json& j, const Performance& x);
	}
}
#endif
