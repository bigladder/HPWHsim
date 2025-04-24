#include <RSCONDENSERWATERHEATSOURCE.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace rscondenserwaterheatsource  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		const std::string_view Schema::schema_title = "Condenser Water Heat Source";

		const std::string_view Schema::schema_version = "0.1.0";

		const std::string_view Schema::schema_description = "Schema for ASHRAE 205 annex RSCONDENSERWATERHEATSOURCE: Condenser heat source";

		void from_json(const nlohmann::json& j, ProductInformation& x) {
			json_get<std::string>(j, logger.get(), "manufacturer", x.manufacturer, x.manufacturer_is_set, true);
			json_get<std::string>(j, logger.get(), "model_number", x.model_number, x.model_number_is_set, true);
		}
		void to_json(nlohmann::json& j, const ProductInformation& x) {
			json_set<std::string>(j, logger.get(), "manufacturer", x.manufacturer, x.manufacturer_is_set, true);
			json_set<std::string>(j, logger.get(), "model_number", x.model_number, x.model_number_is_set, true);
		}
		const std::string_view ProductInformation::manufacturer_units = "";

		const std::string_view ProductInformation::manufacturer_description = "Manufacturer name";

		const std::string_view ProductInformation::manufacturer_name = "manufacturer";

		const std::string_view ProductInformation::model_number_units = "";

		const std::string_view ProductInformation::model_number_description = "Model number";

		const std::string_view ProductInformation::model_number_name = "model_number";

		void from_json(const nlohmann::json& j, Description& x) {
			json_get<rscondenserwaterheatsource::ProductInformation>(j, logger.get(), "product_information", x.product_information, x.product_information_is_set, false);
		}
		void to_json(nlohmann::json& j, const Description& x) {
			json_set<rscondenserwaterheatsource::ProductInformation>(j, logger.get(), "product_information", x.product_information, x.product_information_is_set, false);
		}
		const std::string_view Description::product_information_units = "";

		const std::string_view Description::product_information_description = "Data group describing product information";

		const std::string_view Description::product_information_name = "product_information";

		void from_json(const nlohmann::json& j, GridVariables& x) {
			json_get<std::vector<double>>(j, logger.get(), "evaporator_environment_dry_bulb_temperature", x.evaporator_environment_dry_bulb_temperature, x.evaporator_environment_dry_bulb_temperature_is_set, true);
			json_get<std::vector<double>>(j, logger.get(), "heat_source_temperature", x.heat_source_temperature, x.heat_source_temperature_is_set, true);
		}
		void to_json(nlohmann::json& j, const GridVariables& x) {
			json_set<std::vector<double>>(j, logger.get(), "evaporator_environment_dry_bulb_temperature", x.evaporator_environment_dry_bulb_temperature, x.evaporator_environment_dry_bulb_temperature_is_set, true);
			json_set<std::vector<double>>(j, logger.get(), "heat_source_temperature", x.heat_source_temperature, x.heat_source_temperature_is_set, true);
		}
		const std::string_view GridVariables::evaporator_environment_dry_bulb_temperature_units = "K";

		const std::string_view GridVariables::evaporator_environment_dry_bulb_temperature_description = "Dry bulb temperature of the air entering the evaporator coil";

		const std::string_view GridVariables::evaporator_environment_dry_bulb_temperature_name = "evaporator_environment_dry_bulb_temperature";

		const std::string_view GridVariables::heat_source_temperature_units = "K";

		const std::string_view GridVariables::heat_source_temperature_description = "Average water temperature at the heat source";

		const std::string_view GridVariables::heat_source_temperature_name = "heat_source_temperature";

		void from_json(const nlohmann::json& j, LookupVariables& x) {
			json_get<std::vector<double>>(j, logger.get(), "input_power", x.input_power, x.input_power_is_set, true);
			json_get<std::vector<double>>(j, logger.get(), "heating_capacity", x.heating_capacity, x.heating_capacity_is_set, true);
		}
		void to_json(nlohmann::json& j, const LookupVariables& x) {
			json_set<std::vector<double>>(j, logger.get(), "input_power", x.input_power, x.input_power_is_set, true);
			json_set<std::vector<double>>(j, logger.get(), "heating_capacity", x.heating_capacity, x.heating_capacity_is_set, true);
		}
		const std::string_view LookupVariables::input_power_units = "W";

		const std::string_view LookupVariables::input_power_description = "Power draw from the compressor, evaporator fan, and any auxiliary power used by the units controls";

		const std::string_view LookupVariables::input_power_name = "input_power";

		const std::string_view LookupVariables::heating_capacity_units = "W";

		const std::string_view LookupVariables::heating_capacity_description = "Total heat added by the condenser to the adjacent water";

		const std::string_view LookupVariables::heating_capacity_name = "heating_capacity";

		void from_json(const nlohmann::json& j, PerformanceMap& x) {
			json_get<rscondenserwaterheatsource::GridVariables>(j, logger.get(), "grid_variables", x.grid_variables, x.grid_variables_is_set, true);
			json_get<rscondenserwaterheatsource::LookupVariables>(j, logger.get(), "lookup_variables", x.lookup_variables, x.lookup_variables_is_set, true);
		}
		void to_json(nlohmann::json& j, const PerformanceMap& x) {
			json_set<rscondenserwaterheatsource::GridVariables>(j, logger.get(), "grid_variables", x.grid_variables, x.grid_variables_is_set, true);
			json_set<rscondenserwaterheatsource::LookupVariables>(j, logger.get(), "lookup_variables", x.lookup_variables, x.lookup_variables_is_set, true);
		}
		const std::string_view PerformanceMap::grid_variables_units = "";

		const std::string_view PerformanceMap::grid_variables_description = "Data group defining the grid variables for heating performance";

		const std::string_view PerformanceMap::grid_variables_name = "grid_variables";

		const std::string_view PerformanceMap::lookup_variables_units = "";

		const std::string_view PerformanceMap::lookup_variables_description = "Data group defining the lookup variables for heating performance";

		const std::string_view PerformanceMap::lookup_variables_name = "lookup_variables";

		void from_json(const nlohmann::json& j, Performance& x) {
			json_get<rscondenserwaterheatsource::PerformanceMap>(j, logger.get(), "performance_map", x.performance_map, x.performance_map_is_set, true);
			json_get<double>(j, logger.get(), "standby_power", x.standby_power, x.standby_power_is_set, false);
			json_get<rscondenserwaterheatsource::CoilConfiguration>(j, logger.get(), "coil_configuration", x.coil_configuration, x.coil_configuration_is_set, true);
			json_get<double>(j, logger.get(), "maximum_refrigerant_temperature", x.maximum_refrigerant_temperature, x.maximum_refrigerant_temperature_is_set, false);
			json_get<double>(j, logger.get(), "compressor_lockout_temperature_hysteresis", x.compressor_lockout_temperature_hysteresis, x.compressor_lockout_temperature_hysteresis_is_set, false);
			json_get<bool>(j, logger.get(), "use_defrost_map", x.use_defrost_map, x.use_defrost_map_is_set, false);
		}
		void to_json(nlohmann::json& j, const Performance& x) {
			json_set<rscondenserwaterheatsource::PerformanceMap>(j, logger.get(), "performance_map", x.performance_map, x.performance_map_is_set, true);
			json_set<double>(j, logger.get(), "standby_power", x.standby_power, x.standby_power_is_set, false);
			json_set<rscondenserwaterheatsource::CoilConfiguration>(j, logger.get(), "coil_configuration", x.coil_configuration, x.coil_configuration_is_set, true);
			json_set<double>(j, logger.get(), "maximum_refrigerant_temperature", x.maximum_refrigerant_temperature, x.maximum_refrigerant_temperature_is_set, false);
			json_set<double>(j, logger.get(), "compressor_lockout_temperature_hysteresis", x.compressor_lockout_temperature_hysteresis, x.compressor_lockout_temperature_hysteresis_is_set, false);
			json_set<bool>(j, logger.get(), "use_defrost_map", x.use_defrost_map, x.use_defrost_map_is_set, false);
		}
		const std::string_view Performance::performance_map_units = "";

		const std::string_view Performance::performance_map_description = "Performance map";

		const std::string_view Performance::performance_map_name = "performance_map";

		const std::string_view Performance::standby_power_units = "W";

		const std::string_view Performance::standby_power_description = "";

		const std::string_view Performance::standby_power_name = "standby_power";

		const std::string_view Performance::coil_configuration_units = "";

		const std::string_view Performance::coil_configuration_description = "Coil configuration";

		const std::string_view Performance::coil_configuration_name = "coil_configuration";

		const std::string_view Performance::maximum_refrigerant_temperature_units = "K";

		const std::string_view Performance::maximum_refrigerant_temperature_description = "Maximum temperature of the refrigerant entering the condenser";

		const std::string_view Performance::maximum_refrigerant_temperature_name = "maximum_refrigerant_temperature";

		const std::string_view Performance::compressor_lockout_temperature_hysteresis_units = "K";

		const std::string_view Performance::compressor_lockout_temperature_hysteresis_description = "Hysteresis for compressor lockout";

		const std::string_view Performance::compressor_lockout_temperature_hysteresis_name = "compressor_lockout_temperature_hysteresis";

		const std::string_view Performance::use_defrost_map_units = "";

		const std::string_view Performance::use_defrost_map_description = "Use defrost map";

		const std::string_view Performance::use_defrost_map_name = "use_defrost_map";

		void from_json(const nlohmann::json& j, RSCONDENSERWATERHEATSOURCE& x) {
			json_get<core::Metadata>(j, logger.get(), "metadata", x.metadata, x.metadata_is_set, true);
			json_get<rscondenserwaterheatsource::Description>(j, logger.get(), "description", x.description, x.description_is_set, false);
			json_get<rscondenserwaterheatsource::Performance>(j, logger.get(), "performance", x.performance, x.performance_is_set, true);
		}
		void to_json(nlohmann::json& j, const RSCONDENSERWATERHEATSOURCE& x) {
			json_set<core::Metadata>(j, logger.get(), "metadata", x.metadata, x.metadata_is_set, true);
			json_set<rscondenserwaterheatsource::Description>(j, logger.get(), "description", x.description, x.description_is_set, false);
			json_set<rscondenserwaterheatsource::Performance>(j, logger.get(), "performance", x.performance, x.performance_is_set, true);
		}
		const std::string_view RSCONDENSERWATERHEATSOURCE::metadata_units = "";

		const std::string_view RSCONDENSERWATERHEATSOURCE::metadata_description = "Metadata data group";

		const std::string_view RSCONDENSERWATERHEATSOURCE::metadata_name = "metadata";

		const std::string_view RSCONDENSERWATERHEATSOURCE::description_units = "";

		const std::string_view RSCONDENSERWATERHEATSOURCE::description_description = "Data group describing product and rating information";

		const std::string_view RSCONDENSERWATERHEATSOURCE::description_name = "description";

		const std::string_view RSCONDENSERWATERHEATSOURCE::performance_units = "";

		const std::string_view RSCONDENSERWATERHEATSOURCE::performance_description = "Data group containing performance information";

		const std::string_view RSCONDENSERWATERHEATSOURCE::performance_name = "performance";

		void rscondenserwaterheatsource::to_json(nlohmann::json& j, const RSCONDENSERWATERHEATSOURCE& x) {
		}
		void rscondenserwaterheatsource::to_json(nlohmann::json& j, const Description& x) {
		}
		void rscondenserwaterheatsource::to_json(nlohmann::json& j, const ProductInformation& x) {
		}
		void rscondenserwaterheatsource::to_json(nlohmann::json& j, const Performance& x) {
		}
		void rscondenserwaterheatsource::to_json(nlohmann::json& j, const PerformanceMap& x) {
		}
		void rscondenserwaterheatsource::to_json(nlohmann::json& j, const GridVariables& x) {
		}
		void rscondenserwaterheatsource::to_json(nlohmann::json& j, const LookupVariables& x) {
		}
	}
}

