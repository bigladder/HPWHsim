#ifndef RSRESISTANCEWATERHEATSOURCE_H_
#define RSRESISTANCEWATERHEATSOURCE_H_
#include <ashrae205.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <heat-source-template.h>
#include <core.h>
#include <rsresistancewaterheatsource.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model {
	namespace rsresistancewaterheatsource_ns {
		inline std::shared_ptr<Courier::Courier> logger;
		void set_logger(std::shared_ptr<Courier::Courier> value);
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
			rsresistancewaterheatsource_ns::ProductInformation product_information;
			bool product_information_is_set;
			const static std::string_view product_information_units;
			const static std::string_view product_information_description;
			const static std::string_view product_information_name;
		};
		struct Performance {
			double input_power;
			double resistance_lockout_temperature_hysteresis;
			bool input_power_is_set;
			bool resistance_lockout_temperature_hysteresis_is_set;
			const static std::string_view input_power_units;
			const static std::string_view resistance_lockout_temperature_hysteresis_units;
			const static std::string_view input_power_description;
			const static std::string_view resistance_lockout_temperature_hysteresis_description;
			const static std::string_view input_power_name;
			const static std::string_view resistance_lockout_temperature_hysteresis_name;
		};
		struct RSRESISTANCEWATERHEATSOURCE : HeatSourceTemplate {
			core_ns::Metadata metadata;
			rsresistancewaterheatsource_ns::Description description;
			rsresistancewaterheatsource_ns::Performance performance;
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
		void from_json(const nlohmann::json& j, RSRESISTANCEWATERHEATSOURCE& x);
		void from_json(const nlohmann::json& j, Description& x);
		void from_json(const nlohmann::json& j, ProductInformation& x);
		void from_json(const nlohmann::json& j, Performance& x);
	}
}
#endif
