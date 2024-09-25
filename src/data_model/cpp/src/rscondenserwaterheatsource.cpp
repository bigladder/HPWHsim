#include <rscondenserwaterheatsource.h>
#include <load-object.h>

namespace data_model
{

namespace rscondenserwaterheatsource_ns
{

void set_logger(std::shared_ptr<Courier::Courier> value) { logger = value; }

const std::string_view Schema::schema_title = "Condenser Water Heat Source";

const std::string_view Schema::schema_version = "0.1.0";

const std::string_view Schema::schema_description =
    "Schema for ASHRAE 205 annex RSCONDENSERWATERHEATSOURCE: Condenser heat source";

void from_json(const nlohmann::json& j, ProductInformation& x)
{
    json_get<std::string>(
        j, logger.get(), "manufacturer", x.manufacturer, x.manufacturer_is_set, true);
    json_get<std::string>(
        j, logger.get(), "model_number", x.model_number, x.model_number_is_set, true);
}
const std::string_view ProductInformation::manufacturer_units = "";

const std::string_view ProductInformation::model_number_units = "";

const std::string_view ProductInformation::manufacturer_description = "Manufacturer name";

const std::string_view ProductInformation::model_number_description = "Model number";

const std::string_view ProductInformation::manufacturer_name = "manufacturer";

const std::string_view ProductInformation::model_number_name = "model_number";

void from_json(const nlohmann::json& j, Description& x)
{
    json_get<rscondenserwaterheatsource_ns::ProductInformation>(j,
                                                                logger.get(),
                                                                "product_information",
                                                                x.product_information,
                                                                x.product_information_is_set,
                                                                false);
}
const std::string_view Description::product_information_units = "";

const std::string_view Description::product_information_description =
    "Data group describing product information";

const std::string_view Description::product_information_name = "product_information";

void from_json(const nlohmann::json& j, PerformancePoint& x)
{
    json_get<double>(j,
                     logger.get(),
                     "heat_source_temperature",
                     x.heat_source_temperature,
                     x.heat_source_temperature_is_set,
                     true);
    json_get<std::vector<double>>(j,
                                  logger.get(),
                                  "input_power_coefficients",
                                  x.input_power_coefficients,
                                  x.input_power_coefficients_is_set,
                                  true);
    json_get<std::vector<double>>(
        j, logger.get(), "cop_coefficients", x.cop_coefficients, x.cop_coefficients_is_set, true);
}
const std::string_view PerformancePoint::heat_source_temperature_units = "K";

const std::string_view PerformancePoint::input_power_coefficients_units = "";

const std::string_view PerformancePoint::cop_coefficients_units = "";

const std::string_view PerformancePoint::heat_source_temperature_description =
    "Temperature of heat source";

const std::string_view PerformancePoint::input_power_coefficients_description =
    "Input-power polynomial expansion coefficients";

const std::string_view PerformancePoint::cop_coefficients_description =
    "Coefficient-of-performance expansion coefficients";

const std::string_view PerformancePoint::heat_source_temperature_name = "heat_source_temperature";

const std::string_view PerformancePoint::input_power_coefficients_name = "input_power_coefficients";

const std::string_view PerformancePoint::cop_coefficients_name = "cop_coefficients";

void from_json(const nlohmann::json& j, GridVariables& x)
{
    json_get<std::vector<double>>(j,
                                  logger.get(),
                                  "evaporator_environment_temperature",
                                  x.evaporator_environment_temperature,
                                  x.evaporator_environment_temperature_is_set,
                                  true);
    json_get<std::vector<double>>(j,
                                  logger.get(),
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
    json_get<std::vector<double>>(
        j, logger.get(), "input_power", x.input_power, x.input_power_is_set, true);
    json_get<std::vector<double>>(j, logger.get(), "cop", x.cop, x.cop_is_set, true);
}
const std::string_view LookupVariables::input_power_units = "W";

const std::string_view LookupVariables::cop_units = "-";

const std::string_view LookupVariables::input_power_description =
    "Input power (Nevap x Nsrc values)";

const std::string_view LookupVariables::cop_description =
    "Coefficient of performance (Nevap x Nsrc values)";

const std::string_view LookupVariables::input_power_name = "input_power";

const std::string_view LookupVariables::cop_name = "cop";

void from_json(const nlohmann::json& j, PerformanceMap& x)
{
    json_get<rscondenserwaterheatsource_ns::GridVariables>(
        j, logger.get(), "grid_variables", x.grid_variables, x.grid_variables_is_set, true);
    json_get<rscondenserwaterheatsource_ns::LookupVariables>(
        j, logger.get(), "lookup_variables", x.lookup_variables, x.lookup_variables_is_set, true);
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
    json_get<std::vector<rscondenserwaterheatsource_ns::PerformancePoint>>(
        j,
        logger.get(),
        "performance_points",
        x.performance_points,
        x.performance_points_is_set,
        true);
    json_get<rscondenserwaterheatsource_ns::PerformanceMap>(
        j, logger.get(), "performance_map", x.performance_map, x.performance_map_is_set, true);
    json_get<double>(
        j, logger.get(), "standby_power", x.standby_power, x.standby_power_is_set, false);
    json_get<rscondenserwaterheatsource_ns::CoilConfiguration>(j,
                                                               logger.get(),
                                                               "coil_configuration",
                                                               x.coil_configuration,
                                                               x.coil_configuration_is_set,
                                                               true);
    json_get<bool>(
        j, logger.get(), "use_defrost_map", x.use_defrost_map, x.use_defrost_map_is_set, false);
}
const std::string_view Performance::performance_points_units = "";

const std::string_view Performance::performance_map_units = "";

const std::string_view Performance::standby_power_units = "W";

const std::string_view Performance::coil_configuration_units = "";

const std::string_view Performance::use_defrost_map_units = "";

const std::string_view Performance::performance_points_description =
    "Collection of performance points";

const std::string_view Performance::performance_map_description = "Performance map";

const std::string_view Performance::standby_power_description = "";

const std::string_view Performance::coil_configuration_description = "Coil configuration";

const std::string_view Performance::use_defrost_map_description = "Use defrost map";

const std::string_view Performance::performance_points_name = "performance_points";

const std::string_view Performance::performance_map_name = "performance_map";

const std::string_view Performance::standby_power_name = "standby_power";

const std::string_view Performance::coil_configuration_name = "coil_configuration";

const std::string_view Performance::use_defrost_map_name = "use_defrost_map";

void from_json(const nlohmann::json& j, RSCONDENSERWATERHEATSOURCE& x)
{
    json_get<core_ns::Metadata>(j, logger.get(), "metadata", x.metadata, x.metadata_is_set, true);
    json_get<rscondenserwaterheatsource_ns::Description>(
        j, logger.get(), "description", x.description, x.description_is_set, false);
    json_get<rscondenserwaterheatsource_ns::Performance>(
        j, logger.get(), "performance", x.performance, x.performance_is_set, true);
}
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

void RSCONDENSERWATERHEATSOURCE::initialize(const nlohmann::json& j) { from_json(j, *this); }
} // namespace rscondenserwaterheatsource_ns
} // namespace data_model
