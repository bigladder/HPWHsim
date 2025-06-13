#include <HPWHSimInput.h>
#include <load-object.h>

namespace hpwh_data_model  {

	namespace hpwh_sim_input  {
	
		void set_logger (std::shared_ptr<Courier::Courier> value) { logger = value; }

		void from_json(const nlohmann::json& j, HPWHSimInput& x) {
			json_get<core::Metadata>(j, logger.get(), "metadata", x.metadata, x.metadata_is_set, true);
			json_get<int>(j, logger.get(), "number_of_nodes", x.number_of_nodes, x.number_of_nodes_is_set, false);
			json_get<bool>(j, logger.get(), "depresses_temperature", x.depresses_temperature, x.depresses_temperature_is_set, false);
			json_get<bool>(j, logger.get(), "fixed_volume", x.fixed_volume, x.fixed_volume_is_set, false);
			json_get<double>(j, logger.get(), "standard_setpoint", x.standard_setpoint, x.standard_setpoint_is_set, false);
			json_get<hpwh_sim_input::HPWHSystemType>(j, logger.get(), "system_type", x.system_type, x.system_type_is_set, true);
			json_get<rsintegratedwaterheater::RSINTEGRATEDWATERHEATER>(j, logger.get(), "integrated_system", x.integrated_system, x.integrated_system_is_set, false);
			json_get<central_water_heating_system::CentralWaterHeatingSystem>(j, logger.get(), "central_system", x.central_system, x.central_system_is_set, false);
		}
		void to_json(nlohmann::json& j, const HPWHSimInput& x) {
			json_set<core::Metadata>(j, "metadata", x.metadata, x.metadata_is_set);
			json_set<int>(j, "number_of_nodes", x.number_of_nodes, x.number_of_nodes_is_set);
			json_set<bool>(j, "depresses_temperature", x.depresses_temperature, x.depresses_temperature_is_set);
			json_set<bool>(j, "fixed_volume", x.fixed_volume, x.fixed_volume_is_set);
			json_set<double>(j, "standard_setpoint", x.standard_setpoint, x.standard_setpoint_is_set);
			json_set<hpwh_sim_input::HPWHSystemType>(j, "system_type", x.system_type, x.system_type_is_set);
			json_set<rsintegratedwaterheater::RSINTEGRATEDWATERHEATER>(j, "integrated_system", x.integrated_system, x.integrated_system_is_set);
			json_set<central_water_heating_system::CentralWaterHeatingSystem>(j, "central_system", x.central_system, x.central_system_is_set);
		}
	}
}

