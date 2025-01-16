#ifndef RSINTEGRATEDWATERHEATER_H_
#define RSINTEGRATEDWATERHEATER_H_
#include <ashrae205.h>
#include <rstank.h>
#include <rsresistancewaterheatsource.h>
#include <rscondenserwaterheatsource.h>
#include <heatsourceconfiguration.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <core.h>
#include <rsintegratedwaterheater.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model
{
namespace rsintegratedwaterheater_ns
{
inline std::shared_ptr<Courier::Courier> logger;
void set_logger(std::shared_ptr<Courier::Courier> value);
struct Schema
{
    const static std::string_view schema_title;
    const static std::string_view schema_version;
    const static std::string_view schema_description;
};
struct ProductInformation
{
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
struct Description
{
    rsintegratedwaterheater_ns::ProductInformation product_information;
    bool product_information_is_set;
    const static std::string_view product_information_units;
    const static std::string_view product_information_description;
    const static std::string_view product_information_name;
};
struct Performance
{
    rstank_ns::RSTANK tank;
    std::vector<heat_source_configuration_ns::HeatSourceConfiguration> heat_source_configurations;
    std::string primary_heat_source_id;
    double standby_power;
    bool tank_is_set;
    bool heat_source_configurations_is_set;
    bool primary_heat_source_id_is_set;
    bool standby_power_is_set;
    const static std::string_view tank_units;
    const static std::string_view heat_source_configurations_units;
    const static std::string_view primary_heat_source_id_units;
    const static std::string_view standby_power_units;
    const static std::string_view tank_description;
    const static std::string_view heat_source_configurations_description;
    const static std::string_view primary_heat_source_id_description;
    const static std::string_view standby_power_description;
    const static std::string_view tank_name;
    const static std::string_view heat_source_configurations_name;
    const static std::string_view primary_heat_source_id_name;
    const static std::string_view standby_power_name;
};
struct RSINTEGRATEDWATERHEATER
{
    core_ns::Metadata metadata;
    rsintegratedwaterheater_ns::Description description;
    rsintegratedwaterheater_ns::Performance performance;
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
void from_json(const nlohmann::json& j, RSINTEGRATEDWATERHEATER& x);
void from_json(const nlohmann::json& j, Description& x);
void from_json(const nlohmann::json& j, ProductInformation& x);
void from_json(const nlohmann::json& j, Performance& x);
} // namespace rsintegratedwaterheater_ns
} // namespace hpwh_data_model
#endif
