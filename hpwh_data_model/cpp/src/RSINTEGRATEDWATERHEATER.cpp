#include <RSINTEGRATEDWATERHEATER.h>
#include <load-object.h>

namespace hpwh_data_model
{

namespace rsintegratedwaterheater
{

void set_logger(std::shared_ptr<Courier::Courier> value) { logger = value; }

const std::string_view Schema::schema_title = "Integrated Heat-Pump Water Heater";

const std::string_view Schema::schema_version = "0.1.0";

const std::string_view Schema::schema_description =
    "Schema for ASHRAE 205 annex RSINTEGRATEDWATERHEATER: Integrated Heat-Pump Water Heater";

void from_json(const nlohmann::json& j, ProductInformation& x)
{
    json_get<std::string>(
        j, logger.get(), "manufacturer", x.manufacturer, x.manufacturer_is_set, true);
    json_get<std::string>(
        j, logger.get(), "model_number", x.model_number, x.model_number_is_set, true);
}
const std::string_view ProductInformation::manufacturer_units = "";

const std::string_view ProductInformation::manufacturer_description = "Manufacturer name";

const std::string_view ProductInformation::manufacturer_name = "manufacturer";

const std::string_view ProductInformation::model_number_units = "";

const std::string_view ProductInformation::model_number_description = "Model number";

const std::string_view ProductInformation::model_number_name = "model_number";

void from_json(const nlohmann::json& j, Description& x)
{
    json_get<rsintegratedwaterheater::ProductInformation>(j,
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

void from_json(const nlohmann::json& j, Performance& x)
{
    json_get<rstank::RSTANK>(j, logger.get(), "tank", x.tank, x.tank_is_set, true);
    json_get<std::vector<heat_source_configuration::HeatSourceConfiguration>>(
        j,
        logger.get(),
        "heat_source_configurations",
        x.heat_source_configurations,
        x.heat_source_configurations_is_set,
        false);
    json_get<std::string>(j,
                          logger.get(),
                          "primary_heat_source_id",
                          x.primary_heat_source_id,
                          x.primary_heat_source_id_is_set,
                          false);
    json_get<double>(
        j, logger.get(), "standby_power", x.standby_power, x.standby_power_is_set, false);
}
const std::string_view Performance::tank_units = "";

const std::string_view Performance::tank_description =
    "The corresponding Standard 205 tank representation";

const std::string_view Performance::tank_name = "tank";

const std::string_view Performance::heat_source_configurations_units = "";

const std::string_view Performance::heat_source_configurations_description =
    "Describes how the heat sources are configured within the tank";

const std::string_view Performance::heat_source_configurations_name = "heat_source_configurations";

const std::string_view Performance::primary_heat_source_id_units = "";

const std::string_view Performance::primary_heat_source_id_description =
    "Turns on independently of other heat sources";

const std::string_view Performance::primary_heat_source_id_name = "primary_heat_source_id";

const std::string_view Performance::standby_power_units = "";

const std::string_view Performance::standby_power_description =
    "Power drawn when system is in standby mode";

const std::string_view Performance::standby_power_name = "standby_power";

void from_json(const nlohmann::json& j, RSINTEGRATEDWATERHEATER& x)
{
    json_get<core::Metadata>(j, logger.get(), "metadata", x.metadata, x.metadata_is_set, true);
    json_get<rsintegratedwaterheater::Description>(
        j, logger.get(), "description", x.description, x.description_is_set, false);
    json_get<rsintegratedwaterheater::Performance>(
        j, logger.get(), "performance", x.performance, x.performance_is_set, true);
}
const std::string_view RSINTEGRATEDWATERHEATER::metadata_units = "";

const std::string_view RSINTEGRATEDWATERHEATER::metadata_description = "Metadata data group";

const std::string_view RSINTEGRATEDWATERHEATER::metadata_name = "metadata";

const std::string_view RSINTEGRATEDWATERHEATER::description_units = "";

const std::string_view RSINTEGRATEDWATERHEATER::description_description =
    "Data group describing product and rating information";

const std::string_view RSINTEGRATEDWATERHEATER::description_name = "description";

const std::string_view RSINTEGRATEDWATERHEATER::performance_units = "";

const std::string_view RSINTEGRATEDWATERHEATER::performance_description =
    "Data group containing performance information";

const std::string_view RSINTEGRATEDWATERHEATER::performance_name = "performance";

} // namespace rsintegratedwaterheater
} // namespace hpwh_data_model
