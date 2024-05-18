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
    json_get<ProductInformation>(
        j, "product_information", x.product_information, x.product_information_is_set, false);
}
const std::string_view Description::product_information_units = "";

const std::string_view Description::product_information_description =
    "Data group describing product information";

const std::string_view Description::product_information_name = "product_information";

void from_json(const nlohmann::json& j, HeatingLogic& x)
{
    json_get<HeatingLogicType>(
        j, "heating_logic_type", x.heating_logic_type, x.heating_logic_type_is_set, true);
    if (x.heating_logic_type == HeatingLogicType::SOC_BASED)
    {
        x.heating_logic = std::make_unique<SoCBasedHeatingLogic>();
        if (x.heating_logic)
        {
            x.heating_logic->initialize(j.at("heating_logic"));
        }
    }
    if (x.heating_logic_type == HeatingLogicType::TEMP_BASED)
    {
        x.heating_logic = std::make_unique<TempBasedHeatingLogic>();
        if (x.heating_logic)
        {
            x.heating_logic->initialize(j.at("heating_logic"));
        }
    }
    json_get<ComparisonType>(
        j, "comparison_type", x.comparison_type, x.comparison_type_is_set, true);
}
const std::string_view HeatingLogic::heating_logic_type_units = "";

const std::string_view HeatingLogic::heating_logic_units = "";

const std::string_view HeatingLogic::comparison_type_units = "";

const std::string_view HeatingLogic::heating_logic_type_description = "Type of heating logic";

const std::string_view HeatingLogic::heating_logic_description = "Specific heating logic";

const std::string_view HeatingLogic::comparison_type_description =
    "Result of comparison for activation";

const std::string_view HeatingLogic::heating_logic_type_name = "heating_logic_type";

const std::string_view HeatingLogic::heating_logic_name = "heating_logic";

const std::string_view HeatingLogic::comparison_type_name = "comparison_type";

void from_json(const nlohmann::json& j, HeatSourceConfiguration& x)
{
    json_get<HeatSourceType>(
        j, "heat_source_type", x.heat_source_type, x.heat_source_type_is_set, true);
    if (x.heat_source_type == HeatSourceType::RESISTANCE)
    {
        x.heat_source =
            std::make_unique<rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE>();
        if (x.heat_source)
        {
            x.heat_source->initialize(j.at("heat_source"));
        }
    }
    if (x.heat_source_type == HeatSourceType::CONDENSER)
    {
        x.heat_source =
            std::make_unique<rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE>();
        if (x.heat_source)
        {
            x.heat_source->initialize(j.at("heat_source"));
        }
    }
    json_get<std::vector<double>>(
        j, "heat_distribution", x.heat_distribution, x.heat_distribution_is_set, true);
    json_get<std::vector<HeatingLogic>>(
        j, "turn_on_logic", x.turn_on_logic, x.turn_on_logic_is_set, true);
    json_get<std::vector<HeatingLogic>>(
        j, "shut_off_logic", x.shut_off_logic, x.shut_off_logic_is_set, false);
    json_get<std::vector<HeatingLogic>>(
        j, "standby_logic", x.standby_logic, x.standby_logic_is_set, false);
    json_get<double>(j, "maximum_setpoint", x.maximum_setpoint, x.maximum_setpoint_is_set, false);
    json_get<double>(j,
                     "hysteresis_temperature_difference",
                     x.hysteresis_temperature_difference,
                     x.hysteresis_temperature_difference_is_set,
                     false);
    json_get<bool>(j, "is_vip", x.is_vip, x.is_vip_is_set, false);
}
const std::string_view HeatSourceConfiguration::heat_source_type_units = "";

const std::string_view HeatSourceConfiguration::heat_source_units = "";

const std::string_view HeatSourceConfiguration::heat_distribution_units = "";

const std::string_view HeatSourceConfiguration::turn_on_logic_units = "";

const std::string_view HeatSourceConfiguration::shut_off_logic_units = "";

const std::string_view HeatSourceConfiguration::standby_logic_units = "";

const std::string_view HeatSourceConfiguration::maximum_setpoint_units = "K";

const std::string_view HeatSourceConfiguration::hysteresis_temperature_difference_units = "K";

const std::string_view HeatSourceConfiguration::is_vip_units = "";

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

const std::string_view HeatSourceConfiguration::hysteresis_temperature_difference_description =
    "Hysteresis temperature difference for activation";

const std::string_view HeatSourceConfiguration::is_vip_description =
    "turns on independently of other heat sources.";

const std::string_view HeatSourceConfiguration::heat_source_type_name = "heat_source_type";

const std::string_view HeatSourceConfiguration::heat_source_name = "heat_source";

const std::string_view HeatSourceConfiguration::heat_distribution_name = "heat_distribution";

const std::string_view HeatSourceConfiguration::turn_on_logic_name = "turn_on_logic";

const std::string_view HeatSourceConfiguration::shut_off_logic_name = "shut_off_logic";

const std::string_view HeatSourceConfiguration::standby_logic_name = "standby_logic";

const std::string_view HeatSourceConfiguration::maximum_setpoint_name = "maximum_setpoint";

const std::string_view HeatSourceConfiguration::hysteresis_temperature_difference_name =
    "hysteresis_temperature_difference";

const std::string_view HeatSourceConfiguration::is_vip_name = "is_vip";

void from_json(const nlohmann::json& j, Performance& x)
{
    json_get<rstank_ns::RSTANK>(j, "tank", x.tank, x.tank_is_set, true);
    json_get<std::vector<HeatSourceConfiguration>>(j,
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
    json_get<Description>(j, "description", x.description, x.description_is_set, false);
    json_get<Performance>(j, "performance", x.performance, x.performance_is_set, true);
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

void from_json(const nlohmann::json& j, TempBasedHeatingLogic& x)
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
}
const std::string_view TempBasedHeatingLogic::absolute_temperature_units = "K";

const std::string_view TempBasedHeatingLogic::differential_temperature_units = "K";

const std::string_view TempBasedHeatingLogic::logic_distribution_units = "";

const std::string_view TempBasedHeatingLogic::absolute_temperature_description =
    "Absolute temperature for activation";

const std::string_view TempBasedHeatingLogic::differential_temperature_description =
    "Temperature difference for activation";

const std::string_view TempBasedHeatingLogic::logic_distribution_description =
    "Weighted distribution for comparison, by division, in order";

const std::string_view TempBasedHeatingLogic::absolute_temperature_name = "absolute_temperature";

const std::string_view TempBasedHeatingLogic::differential_temperature_name =
    "differential_temperature";

const std::string_view TempBasedHeatingLogic::logic_distribution_name = "logic_distribution";

void from_json(const nlohmann::json& j, SoCBasedHeatingLogic& x)
{
    json_get<double>(j, "decision_point", x.decision_point, x.decision_point_is_set, true);
    json_get<double>(j,
                     "minimum_useful_temperature",
                     x.minimum_useful_temperature,
                     x.minimum_useful_temperature_is_set,
                     false);
    json_get<double>(
        j, "hysteresis_fraction", x.hysteresis_fraction, x.hysteresis_fraction_is_set, false);
    json_get<bool>(
        j, "uses_constant_mains", x.uses_constant_mains, x.uses_constant_mains_is_set, true);
    json_get<double>(j,
                     "constant_mains_temperature",
                     x.constant_mains_temperature,
                     x.constant_mains_temperature_is_set,
                     true);
}
const std::string_view SoCBasedHeatingLogic::decision_point_units = "";

const std::string_view SoCBasedHeatingLogic::minimum_useful_temperature_units = "";

const std::string_view SoCBasedHeatingLogic::hysteresis_fraction_units = "K";

const std::string_view SoCBasedHeatingLogic::uses_constant_mains_units = "";

const std::string_view SoCBasedHeatingLogic::constant_mains_temperature_units = "K";

const std::string_view SoCBasedHeatingLogic::decision_point_description = "Decision point";

const std::string_view SoCBasedHeatingLogic::minimum_useful_temperature_description =
    "Minimum useful temperature";

const std::string_view SoCBasedHeatingLogic::hysteresis_fraction_description =
    "Hysteresis fraction";

const std::string_view SoCBasedHeatingLogic::uses_constant_mains_description =
    "Uses constant mains";

const std::string_view SoCBasedHeatingLogic::constant_mains_temperature_description =
    "Constant mains temperature";

const std::string_view SoCBasedHeatingLogic::decision_point_name = "decision_point";

const std::string_view SoCBasedHeatingLogic::minimum_useful_temperature_name =
    "minimum_useful_temperature";

const std::string_view SoCBasedHeatingLogic::hysteresis_fraction_name = "hysteresis_fraction";

const std::string_view SoCBasedHeatingLogic::uses_constant_mains_name = "uses_constant_mains";

const std::string_view SoCBasedHeatingLogic::constant_mains_temperature_name =
    "constant_mains_temperature";

void SoCBasedHeatingLogic::initialize(const nlohmann::json& j) { from_json(j, *this); }
void TempBasedHeatingLogic::initialize(const nlohmann::json& j) { from_json(j, *this); }
} // namespace rsintegratedwaterheater_ns
} // namespace hpwh_data_model
