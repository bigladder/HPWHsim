#include <ashrae205.h>
#include <rsresistancewaterheatsource.h>
#include <load-object.h>

namespace hpwh_data_model
{

namespace rsresistancewaterheatsource_ns
{

const std::string_view Schema::schema_title = "Resistance Water Heat Source";

const std::string_view Schema::schema_version = "0.1.0";

const std::string_view Schema::schema_description =
    "Schema for ASHRAE 205 annex RSRESISTANCEWATERHEATSOURCE: Resistance heat source";

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
    json_get<rsresistancewaterheatsource_ns::ProductInformation>(
        j, "product_information", x.product_information, x.product_information_is_set, false);
}
const std::string_view Description::product_information_units = "";

const std::string_view Description::product_information_description =
    "Data group describing product information";

const std::string_view Description::product_information_name = "product_information";

void from_json(const nlohmann::json& j, Performance& x)
{
    json_get<double>(j, "input_power", x.input_power, x.input_power_is_set, true);
}
const std::string_view Performance::input_power_units = "W";

const std::string_view Performance::input_power_description = "Input power";

const std::string_view Performance::input_power_name = "input_power";

void from_json(const nlohmann::json& j, RSRESISTANCEWATERHEATSOURCE& x)
{
    json_get<core_ns::Metadata>(j, "metadata", x.metadata, x.metadata_is_set, true);
    json_get<rsresistancewaterheatsource_ns::Description>(
        j, "description", x.description, x.description_is_set, false);
    json_get<rsresistancewaterheatsource_ns::Performance>(
        j, "performance", x.performance, x.performance_is_set, true);
}
const std::string_view RSRESISTANCEWATERHEATSOURCE::metadata_units = "";

const std::string_view RSRESISTANCEWATERHEATSOURCE::description_units = "";

const std::string_view RSRESISTANCEWATERHEATSOURCE::performance_units = "";

const std::string_view RSRESISTANCEWATERHEATSOURCE::metadata_description = "Metadata data group";

const std::string_view RSRESISTANCEWATERHEATSOURCE::description_description =
    "Data group describing product and rating information";

const std::string_view RSRESISTANCEWATERHEATSOURCE::performance_description =
    "Data group containing performance information";

const std::string_view RSRESISTANCEWATERHEATSOURCE::metadata_name = "metadata";

const std::string_view RSRESISTANCEWATERHEATSOURCE::description_name = "description";

const std::string_view RSRESISTANCEWATERHEATSOURCE::performance_name = "performance";

void RSRESISTANCEWATERHEATSOURCE::initialize(const nlohmann::json& j)
{
    json_get<core_ns::Metadata>(j, "metadata", metadata, metadata_is_set, true);
    json_get<Description>(j, "description", description, description_is_set, false);
    json_get<Performance>(j, "performance", performance, performance_is_set, true);
}
} // namespace rsresistancewaterheatsource_ns
} // namespace hpwh_data_model
