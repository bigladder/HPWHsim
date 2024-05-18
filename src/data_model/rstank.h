#ifndef RSTANK_H_
#define RSTANK_H_
#include <ashrae205.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <core.h>
#include <enum-info.h>

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
    bool fixed_volume;
    int number_of_nodes;
    bool do_conduction;
    bool mixes_on_draw;
    bool do_inversion_mixing;
    double bottom_fraction_of_tank_mixing_on_draw;
    bool has_heat_exchanger;
    double heat_exchange_effectiveness;
    bool volume_is_set;
    bool diameter_is_set;
    bool ua_is_set;
    bool fittings_ua_is_set;
    bool fixed_volume_is_set;
    bool number_of_nodes_is_set;
    bool do_conduction_is_set;
    bool mixes_on_draw_is_set;
    bool do_inversion_mixing_is_set;
    bool bottom_fraction_of_tank_mixing_on_draw_is_set;
    bool has_heat_exchanger_is_set;
    bool heat_exchange_effectiveness_is_set;
    const static std::string_view volume_units;
    const static std::string_view diameter_units;
    const static std::string_view ua_units;
    const static std::string_view fittings_ua_units;
    const static std::string_view fixed_volume_units;
    const static std::string_view number_of_nodes_units;
    const static std::string_view do_conduction_units;
    const static std::string_view mixes_on_draw_units;
    const static std::string_view do_inversion_mixing_units;
    const static std::string_view bottom_fraction_of_tank_mixing_on_draw_units;
    const static std::string_view has_heat_exchanger_units;
    const static std::string_view heat_exchange_effectiveness_units;
    const static std::string_view volume_description;
    const static std::string_view diameter_description;
    const static std::string_view ua_description;
    const static std::string_view fittings_ua_description;
    const static std::string_view fixed_volume_description;
    const static std::string_view number_of_nodes_description;
    const static std::string_view do_conduction_description;
    const static std::string_view mixes_on_draw_description;
    const static std::string_view do_inversion_mixing_description;
    const static std::string_view bottom_fraction_of_tank_mixing_on_draw_description;
    const static std::string_view has_heat_exchanger_description;
    const static std::string_view heat_exchange_effectiveness_description;
    const static std::string_view volume_name;
    const static std::string_view diameter_name;
    const static std::string_view ua_name;
    const static std::string_view fittings_ua_name;
    const static std::string_view fixed_volume_name;
    const static std::string_view number_of_nodes_name;
    const static std::string_view do_conduction_name;
    const static std::string_view mixes_on_draw_name;
    const static std::string_view do_inversion_mixing_name;
    const static std::string_view bottom_fraction_of_tank_mixing_on_draw_name;
    const static std::string_view has_heat_exchanger_name;
    const static std::string_view heat_exchange_effectiveness_name;
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
