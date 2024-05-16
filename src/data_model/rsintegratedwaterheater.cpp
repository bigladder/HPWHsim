#include <rsintegratedwaterheater.h>
#include <load-object.h>

namespace hpwh_data_model
{

namespace rsintegratedwaterheater_ns
{

const std::string_view Schema::schema_title = "Integrated Heat-Pump Water Heater";

const std::string_view Schema::schema_version = "0.1.0";

const std::string_view Schema::schema_description =
    "Schema for ASHRAE 205 annex RSINTEGRATEDWATERHEATER: Integrated Heat-Pump Water Heater";

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
    json_get<rsintegratedwaterheater_ns::ProductInformation>(
        j, "product_information", x.product_information, x.product_information_is_set, false);
}
const std::string_view Description::product_information_units = "";

const std::string_view Description::product_information_description =
    "Data group describing product information";

const std::string_view Description::product_information_name = "product_information";

void from_json(const nlohmann::json& j, HeatingLogic& x)
{
    json_get<double>(
        j, "absolute_temperature", x.absolute_temperature, x.absolute_temperature_is_set, true);
    json_get<double>(j,
                     "differential_temperature",
                     x.differential_temperature,
                     x.differential_temperature_is_set,
                     true);
    json_get<std::vector<double>>(
        j, "logic_distribution", x.logic_distribution, x.logic_distribution_is_set, true);
    json_get<ComparisonType>(
        j, "comparison_type", x.comparison_type, x.comparison_type_is_set, true);
    json_get<double>(j,
                     "hysteresis_temperature",
                     x.hysteresis_temperature,
                     x.hysteresis_temperature_is_set,
                     false);
}
const std::string_view HeatingLogic::absolute_temperature_units = "K";

const std::string_view HeatingLogic::differential_temperature_units = "K";

const std::string_view HeatingLogic::logic_distribution_units = "";

const std::string_view HeatingLogic::comparison_type_units = "";

const std::string_view HeatingLogic::hysteresis_temperature_units = "";

const std::string_view HeatingLogic::absolute_temperature_description =
    "Absolute temperature for activation";

const std::string_view HeatingLogic::differential_temperature_description =
    "Temperature difference for activation";

const std::string_view HeatingLogic::logic_distribution_description =
    "Weighted distribution for comparison, by division, in order";

const std::string_view HeatingLogic::comparison_type_description =
    "Result of comparison for activation";

const std::string_view HeatingLogic::hysteresis_temperature_description =
    "Amount to surpass threshold temperature";

const std::string_view HeatingLogic::absolute_temperature_name = "absolute_temperature";

const std::string_view HeatingLogic::differential_temperature_name = "differential_temperature";

const std::string_view HeatingLogic::logic_distribution_name = "logic_distribution";

const std::string_view HeatingLogic::comparison_type_name = "comparison_type";

const std::string_view HeatingLogic::hysteresis_temperature_name = "hysteresis_temperature";

void from_json(const nlohmann::json& j, HeatSourceConfiguration& x)
{
    json_get<HeatSourceType>(
        j, "heat_source_type", x.heat_source_type, x.heat_source_type_is_set, true);

    json_get<HeatSourceType>(
        j, "heat_source_type", x.heat_source_type, x.heat_source_type_is_set, true);
    if (x.heat_source_type == HeatSourceType::RESISTANCE)
    {
        x.heat_source =
            std::make_unique<rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE>();
    }
    if (x.heat_source_type == HeatSourceType::CONDENSER)
    {
        x.heat_source =
            std::make_unique<rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE>();
    }
    if (x.heat_source)
    {
        x.heat_source->from_json(j);
    }

    json_get<std::vector<double>>(
        j, "heat_distribution", x.heat_distribution, x.heat_distribution_is_set, true);
    json_get<std::vector<HeatingLogic>>(
        j, "shut_off_logic", x.shut_off_logic, x.shut_off_logic_is_set, false);
    json_get<std::vector<HeatingLogic>>(
        j, "standby_logic", x.standby_logic, x.standby_logic_is_set, false);
    json_get<double>(j, "maximum_setpoint", x.maximum_setpoint, x.maximum_setpoint_is_set, false);
    json_get<std::vector<HeatingLogic>>(
        j, "turn_on_logic", x.turn_on_logic, x.turn_on_logic_is_set, true);
}
const std::string_view HeatSourceConfiguration::heat_source_type_units = "";

const std::string_view HeatSourceConfiguration::heat_source_units = "";

const std::string_view HeatSourceConfiguration::heat_distribution_units = "";

const std::string_view HeatSourceConfiguration::turn_on_logic_units = "";

const std::string_view HeatSourceConfiguration::shut_off_logic_units = "";

const std::string_view HeatSourceConfiguration::standby_logic_units = "";

const std::string_view HeatSourceConfiguration::maximum_setpoint_units = "K";

const std::string_view HeatSourceConfiguration::heat_source_type_description =
    "Type of heat source";

const std::string_view HeatSourceConfiguration::heat_source_description =
    "A corresponding Standard 205 heat-source representation";

const std::string_view HeatSourceConfiguration::heat_distribution_description =
    "Weighted distribution of heat by division, in order";

const std::string_view HeatSourceConfiguration::turn_on_logic_description = "";

const std::string_view HeatSourceConfiguration::shut_off_logic_description = "";

const std::string_view HeatSourceConfiguration::standby_logic_description =
    "Checks the bottom is below a temperature so the system doesn't short cycle";

const std::string_view HeatSourceConfiguration::maximum_setpoint_description =
    "Maximum setpoint temperature";

const std::string_view HeatSourceConfiguration::heat_source_type_name = "heat_source_type";

const std::string_view HeatSourceConfiguration::heat_source_name = "heat_source";

const std::string_view HeatSourceConfiguration::heat_distribution_name = "heat_distribution";

const std::string_view HeatSourceConfiguration::turn_on_logic_name = "turn_on_logic";

const std::string_view HeatSourceConfiguration::shut_off_logic_name = "shut_off_logic";

const std::string_view HeatSourceConfiguration::standby_logic_name = "standby_logic";

const std::string_view HeatSourceConfiguration::maximum_setpoint_name = "maximum_setpoint";

void from_json(const nlohmann::json& j, Performance& x)
{
    json_get<rstank_ns::RSTANK>(j, "tank", x.tank, x.tank_is_set, true);
    json_get<std::vector<rsintegratedwaterheater_ns::HeatSourceConfiguration>>(
        j,
        "heat_source_configurations",
        x.heat_source_configurations,
        x.heat_source_configurations_is_set,
        false);
    json_get<double>(j, "standby_power", x.standby_power, x.standby_power_is_set, false);
}
const std::string_view Performance::tank_units = "";

const std::string_view Performance::heat_source_configurations_units = "";

const std::string_view Performance::standby_power_units = "";

const std::string_view Performance::tank_description =
    "The corresponding Standard 205 tank representation";

const std::string_view Performance::heat_source_configurations_description = "";

const std::string_view Performance::standby_power_description =
    "Power drawn when system is in standby mode";

const std::string_view Performance::tank_name = "tank";

const std::string_view Performance::heat_source_configurations_name = "heat_source_configurations";

const std::string_view Performance::standby_power_name = "standby_power";

void from_json(const nlohmann::json& j, RSINTEGRATEDWATERHEATER& x)
{
    json_get<core_ns::Metadata>(j, "metadata", x.metadata, x.metadata_is_set, true);
    json_get<rsintegratedwaterheater_ns::Description>(
        j, "description", x.description, x.description_is_set, false);
    json_get<rsintegratedwaterheater_ns::Performance>(
        j, "performance", x.performance, x.performance_is_set, true);
    json_get<double>(j, "standby_power", x.standby_power, x.standby_power_is_set, false);
}
const std::string_view RSINTEGRATEDWATERHEATER::metadata_units = "";

const std::string_view RSINTEGRATEDWATERHEATER::description_units = "";

const std::string_view RSINTEGRATEDWATERHEATER::performance_units = "";

const std::string_view RSINTEGRATEDWATERHEATER::standby_power_units = "";

const std::string_view RSINTEGRATEDWATERHEATER::metadata_description = "Metadata data group";

const std::string_view RSINTEGRATEDWATERHEATER::description_description =
    "Data group describing product and rating information";

const std::string_view RSINTEGRATEDWATERHEATER::performance_description =
    "Data group containing performance information";

const std::string_view RSINTEGRATEDWATERHEATER::standby_power_description =
    "Power drawn when system is in standby mode";

const std::string_view RSINTEGRATEDWATERHEATER::metadata_name = "metadata";

const std::string_view RSINTEGRATEDWATERHEATER::description_name = "description";

const std::string_view RSINTEGRATEDWATERHEATER::performance_name = "performance";

const std::string_view RSINTEGRATEDWATERHEATER::standby_power_name = "standby_power";

} // namespace rsintegratedwaterheater_ns
} // namespace hpwh_data_model
