#include <ashrae205.h>
#include <load-object.h>

namespace data_model
{

namespace ashrae205_ns
{

void set_logger(std::shared_ptr<Courier::Courier> value) { logger = std::move(value); }

const std::string_view Schema::schema_title = "ASHRAE 205";

const std::string_view Schema::schema_version = "0.2.0";

const std::string_view Schema::schema_description = "Base schema for ASHRAE 205 representations";

void from_json(const nlohmann::json& j, LiquidComponent& x)
{
    json_get<ashrae205_ns::LiquidConstituent>(j,
                                              logger.get(),
                                              "liquid_constituent",
                                              x.liquid_constituent,
                                              x.liquid_constituent_is_set,
                                              true);
    json_get<double>(
        j, logger.get(), "concentration", x.concentration, x.concentration_is_set, false);
}
const std::string_view LiquidComponent::liquid_constituent_units = "";

const std::string_view LiquidComponent::concentration_units = "";

const std::string_view LiquidComponent::liquid_constituent_description =
    "Substance of this component of the mixture";

const std::string_view LiquidComponent::concentration_description =
    "Concentration of this component of the mixture";

const std::string_view LiquidComponent::liquid_constituent_name = "liquid_constituent";

const std::string_view LiquidComponent::concentration_name = "concentration";

void from_json(const nlohmann::json& j, LiquidMixture& x)
{
    json_get<std::vector<ashrae205_ns::LiquidComponent>>(j,
                                                         logger.get(),
                                                         "liquid_components",
                                                         x.liquid_components,
                                                         x.liquid_components_is_set,
                                                         true);
    json_get<ashrae205_ns::ConcentrationType>(j,
                                              logger.get(),
                                              "concentration_type",
                                              x.concentration_type,
                                              x.concentration_type_is_set,
                                              true);
}
const std::string_view LiquidMixture::liquid_components_units = "";

const std::string_view LiquidMixture::concentration_type_units = "";

const std::string_view LiquidMixture::liquid_components_description =
    "An array of all liquid components within the liquid mixture";

const std::string_view LiquidMixture::concentration_type_description =
    "Defines whether concentration is defined on a volume or mass basis";

const std::string_view LiquidMixture::liquid_components_name = "liquid_components";

const std::string_view LiquidMixture::concentration_type_name = "concentration_type";

} // namespace ashrae205_ns
} // namespace data_model
