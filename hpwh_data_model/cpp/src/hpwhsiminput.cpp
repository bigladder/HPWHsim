#include <hpwhsiminput.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace hpwh_sim_input_ns  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		const std::string_view Schema::schema_title = "HPWHsim Input";

		const std::string_view Schema::schema_version = "0.1.0";

		const std::string_view Schema::schema_description = "Input required to describe a heat pump water heating system in HPWHsim";

		void from_json(const nlohmann::json& j, HPWHSimInput& x) {
			json_get<int>(j, logger.get(), "number_of_nodes", x.number_of_nodes, x.number_of_nodes_is_set, false);
			json_get<bool>(j, logger.get(), "depresses_temperature", x.depresses_temperature, x.depresses_temperature_is_set, false);
			json_get<bool>(j, logger.get(), "fixed_volume", x.fixed_volume, x.fixed_volume_is_set, false);
			json_get<hpwh_sim_input_ns::HPWHSystemType>(j, logger.get(), "system_type", x.system_type, x.system_type_is_set, true);
			json_get<rsintegratedwaterheater_ns::RSINTEGRATEDWATERHEATER>(j, logger.get(), "integrated_system", x.integrated_system, x.integrated_system_is_set, false);
			json_get<central_water_heating_system_ns::CentralWaterHeatingSystem>(j, logger.get(), "central_system", x.central_system, x.central_system_is_set, true);
		}
		const std::string_view HPWHSimInput::number_of_nodes_units = "";

		const std::string_view HPWHSimInput::depresses_temperature_units = "";

		const std::string_view HPWHSimInput::fixed_volume_units = "";

		const std::string_view HPWHSimInput::system_type_units = "";

		const std::string_view HPWHSimInput::integrated_system_units = "";

		const std::string_view HPWHSimInput::central_system_units = "";

		const std::string_view HPWHSimInput::number_of_nodes_description = "Number of tank nodes used for simulation";

		const std::string_view HPWHSimInput::depresses_temperature_description = "Depresses space temperature when activated";

		const std::string_view HPWHSimInput::fixed_volume_description = "";

		const std::string_view HPWHSimInput::system_type_description = "System Type";

		const std::string_view HPWHSimInput::integrated_system_description = "The corresponding Standard 205 integrated water heater system";

		const std::string_view HPWHSimInput::central_system_description = "The corresponding data group containing Standard 205 system component representations";

		const std::string_view HPWHSimInput::number_of_nodes_name = "number_of_nodes";

		const std::string_view HPWHSimInput::depresses_temperature_name = "depresses_temperature";

		const std::string_view HPWHSimInput::fixed_volume_name = "fixed_volume";

		const std::string_view HPWHSimInput::system_type_name = "system_type";

		const std::string_view HPWHSimInput::integrated_system_name = "integrated_system";

		const std::string_view HPWHSimInput::central_system_name = "central_system";

	}
}

