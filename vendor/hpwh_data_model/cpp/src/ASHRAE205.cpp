#include <ASHRAE205.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace ashrae205  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		void from_json(const nlohmann::json& j, LiquidComponent& x) {
			json_get<ashrae205::LiquidConstituent>(j, logger.get(), "liquid_constituent", x.liquid_constituent, x.liquid_constituent_is_set, true);
			json_get<double>(j, logger.get(), "concentration", x.concentration, x.concentration_is_set, false);
		}
		void to_json(nlohmann::json& j, const LiquidComponent& x) {
			json_set<ashrae205::LiquidConstituent>(j, "liquid_constituent", x.liquid_constituent, x.liquid_constituent_is_set);
			json_set<double>(j, "concentration", x.concentration, x.concentration_is_set);
		}
		void from_json(const nlohmann::json& j, LiquidMixture& x) {
			json_get<std::vector<ashrae205::LiquidComponent>>(j, logger.get(), "liquid_components", x.liquid_components, x.liquid_components_is_set, true);
			json_get<ashrae205::ConcentrationType>(j, logger.get(), "concentration_type", x.concentration_type, x.concentration_type_is_set, true);
		}
		void to_json(nlohmann::json& j, const LiquidMixture& x) {
			json_set<std::vector<ashrae205::LiquidComponent>>(j, "liquid_components", x.liquid_components, x.liquid_components_is_set);
			json_set<ashrae205::ConcentrationType>(j, "concentration_type", x.concentration_type, x.concentration_type_is_set);
		}
	}
}

