#ifndef RSTANK_H_
#define RSTANK_H_
#include <ashrae205.h>
#include <string>
#include <vector>
#include <core.h>
#include <enum-info.h>
#include <courier/courier.h>
#include <nlohmann/json.hpp>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model
{
namespace rstank_ns
{
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
    rstank_ns::ProductInformation product_information;
    bool product_information_is_set;
    const static std::string_view product_information_units;
    const static std::string_view product_information_description;
    const static std::string_view product_information_name;
};
struct Performance
{
    double volume;
    double diameter;
    double ua;
    double fittings_ua;
    double bottom_fraction_of_tank_mixing_on_draw;
    bool fixed_volume;
    int number_of_nodes;
    bool volume_is_set;
    bool diameter_is_set;
    bool ua_is_set;
    bool fittings_ua_is_set;
    bool bottom_fraction_of_tank_mixing_on_draw_is_set;
    bool fixed_volume_is_set;
    bool number_of_nodes_is_set;
    const static std::string_view volume_units;
    const static std::string_view diameter_units;
    const static std::string_view ua_units;
    const static std::string_view fittings_ua_units;
    const static std::string_view bottom_fraction_of_tank_mixing_on_draw_units;
    const static std::string_view fixed_volume_units;
    const static std::string_view number_of_nodes_units;
    const static std::string_view volume_description;
    const static std::string_view diameter_description;
    const static std::string_view ua_description;
    const static std::string_view fittings_ua_description;
    const static std::string_view bottom_fraction_of_tank_mixing_on_draw_description;
    const static std::string_view fixed_volume_description;
    const static std::string_view number_of_nodes_description;
    const static std::string_view volume_name;
    const static std::string_view diameter_name;
    const static std::string_view ua_name;
    const static std::string_view fittings_ua_name;
    const static std::string_view bottom_fraction_of_tank_mixing_on_draw_name;
    const static std::string_view fixed_volume_name;
    const static std::string_view number_of_nodes_name;
};
struct RSTANK
{
    core_ns::Metadata metadata;
    rstank_ns::Description description;
    rstank_ns::Performance performance;
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
void from_json(const nlohmann::json& j, RSTANK& x);
void from_json(const nlohmann::json& j, Description& x);
void from_json(const nlohmann::json& j, ProductInformation& x);
void from_json(const nlohmann::json& j, Performance& x);
} // namespace rstank_ns
} // namespace hpwh_data_model
#endif
