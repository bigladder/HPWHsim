#include <rscondenserwaterheatsource.h>
#include <load-object.h>
#include <enum-info.h>

namespace hpwh_data_model
{

namespace rscondenserwaterheatsource_ns
{

const std::string_view Schema::schema_title = "Condenser Water Heat Source";

const std::string_view Schema::schema_version = "0.1.0";

const std::string_view Schema::schema_description =
    "Schema for ASHRAE 205 annex RSCONDENSERWATERHEATSOURCE: Condenser heat source";

void from_json(const nlohmann::json& j, ProductInformation& x)
{
    json_get<std::string>(j, "manufacturer", x.manufacturer, x.manufacturer_is_set, true);
    json_get<std::string>(j, "model_number", x.model_number, x.model_number_is_set, true);
}
const std::string_view ProductInformation::manufacturer_units = "";

const std::string_view ProductInformation::model_number_units = "";

const std::string_view ProductInformation::manufacturer_description = "Manufacturer name";

const std::string_view ProductInformation::model_number_description = "Model number";

const std::string_view ProductInformation::manufacturer_name = "manufacturer";

const std::string_view ProductInformation::model_number_name = "model_number";

void from_json(const nlohmann::json& j, Description& x)
{
    json_get<ProductInformation>(
        j, "product_information", x.product_information, x.product_information_is_set, false);
}
const std::string_view Description::product_information_units = "";

const std::string_view Description::product_information_description =
    "Data group describing product information";

const std::string_view Description::product_information_name = "product_information";

void from_json(const nlohmann::json& j, GridVariables& x)
{
    json_get<std::vector<double>>(j,
                                  "evaporator_environment_temperature",
                                  x.evaporator_environment_temperature,
                                  x.evaporator_environment_temperature_is_set,
                                  true);
    json_get<std::vector<double>>(j,
                                  "heat_source_temperature",
                                  x.heat_source_temperature,
                                  x.heat_source_temperature_is_set,
                                  true);
}
const std::string_view GridVariables::evaporator_environment_temperature_units = "K";

const std::string_view GridVariables::heat_source_temperature_units = "K";

const std::string_view GridVariables::evaporator_environment_temperature_description =
    "Evaporator environment temperatures (Nevap values)";

const std::string_view GridVariables::heat_source_temperature_description =
    "Temperature of heat sources (Nsrc values)";

const std::string_view GridVariables::evaporator_environment_temperature_name =
    "evaporator_environment_temperature";

const std::string_view GridVariables::heat_source_temperature_name = "heat_source_temperature";

void from_json(const nlohmann::json& j, LookupVariables& x)
{
    json_get<std::vector<double>>(j, "input_power", x.input_power, x.input_power_is_set, true);
    json_get<std::vector<double>>(j, "cop", x.cop, x.cop_is_set, true);
}
const std::string_view LookupVariables::input_power_units = "W";

const std::string_view LookupVariables::cop_units = "-";

const std::string_view LookupVariables::input_power_description =
    "Input power - Contains Nevap x Nsrc values";

const std::string_view LookupVariables::cop_description =
    "Coefficient of performance - Contains Nevap x Nsrc values";

const std::string_view LookupVariables::input_power_name = "input_power";

const std::string_view LookupVariables::cop_name = "cop";

void from_json(const nlohmann::json& j, PerformanceMap& x)
{
    json_get<GridVariables>(j, "grid_variables", x.grid_variables, x.grid_variables_is_set, true);
    json_get<LookupVariables>(
        j, "lookup_variables", x.lookup_variables, x.lookup_variables_is_set, true);
}
const std::string_view PerformanceMap::grid_variables_units = "";

const std::string_view PerformanceMap::lookup_variables_units = "";

const std::string_view PerformanceMap::grid_variables_description =
    "Collection of values affecting performance.";

const std::string_view PerformanceMap::lookup_variables_description =
    "Collection of performance metrics.";

const std::string_view PerformanceMap::grid_variables_name = "grid_variables";

const std::string_view PerformanceMap::lookup_variables_name = "lookup_variables";

void from_json(const nlohmann::json& j, Performance& x)
{
    json_get<PerformanceMap>(
        j, "performance_map", x.performance_map, x.performance_map_is_set, true);
    json_get<double>(j, "standby_power", x.standby_power, x.standby_power_is_set, false);
    json_get<CoilConfiguration>(
        j, "coil_configuration", x.coil_configuration, x.coil_configuration_is_set, true);
}
const std::string_view Performance::performance_map_units = "";

const std::string_view Performance::standby_power_units = "W";

const std::string_view Performance::coil_configuration_units = "";

const std::string_view Performance::performance_map_description =
    "Collection of performance values";

const std::string_view Performance::standby_power_description = "";

const std::string_view Performance::coil_configuration_description = "Coil configuration";

const std::string_view Performance::performance_map_name = "performance_map";

const std::string_view Performance::standby_power_name = "standby_power";

const std::string_view Performance::coil_configuration_name = "coil_configuration";

void from_json(const nlohmann::json& j, RSCONDENSERWATERHEATSOURCE& x) { x.from_json(j); }
const std::string_view RSCONDENSERWATERHEATSOURCE::metadata_units = "";

const std::string_view RSCONDENSERWATERHEATSOURCE::description_units = "";

const std::string_view RSCONDENSERWATERHEATSOURCE::performance_units = "";

const std::string_view RSCONDENSERWATERHEATSOURCE::metadata_description = "Metadata data group";

const std::string_view RSCONDENSERWATERHEATSOURCE::description_description =
    "Data group describing product and rating information";

const std::string_view RSCONDENSERWATERHEATSOURCE::performance_description =
    "Data group containing performance information";

const std::string_view RSCONDENSERWATERHEATSOURCE::metadata_name = "metadata";

const std::string_view RSCONDENSERWATERHEATSOURCE::description_name = "description";

const std::string_view RSCONDENSERWATERHEATSOURCE::performance_name = "performance";

void RSCONDENSERWATERHEATSOURCE::from_json(const nlohmann::json& j)
{
    json_get<core_ns::Metadata>(j, "metadata", metadata, metadata_is_set, true);
    json_get<Description>(j, "description", description, description_is_set, false);
    json_get<Performance>(j, "performance", performance, performance_is_set, true);
}
} // namespace rscondenserwaterheatsource_ns
} // namespace hpwh_data_model
