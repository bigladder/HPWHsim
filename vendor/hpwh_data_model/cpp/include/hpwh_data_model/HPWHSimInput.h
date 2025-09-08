#ifndef HPWH_SIM_INPUT_H_
#define HPWH_SIM_INPUT_H_
#include <ASHRAE205.h>
#include <RSINTEGRATEDWATERHEATER.h>
#include <CentralWaterHeatingSystem.h>
#include <RSTANK.h>
#include <RSRESISTANCEWATERHEATSOURCE.h>
#include <RSCONDENSERWATERHEATSOURCE.h>
#include <HeatSourceConfiguration.h>
#include <RSAIRTOWATERHEATPUMP.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>
#include <core.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model {
	namespace hpwh_sim_input {
		enum class HPWHSystemType {
			INTEGRATED,
			CENTRAL,
			UNKNOWN
		};
		const static std::unordered_map<HPWHSystemType, enum_info> HPWHSystemType_info {
			{HPWHSystemType::INTEGRATED, {"INTEGRATED", "Integrated", "Integrated Heat Pump Water Heater"}},
			{HPWHSystemType::CENTRAL, {"CENTRAL", "Central", "Integrated Heat Pump Water Heater"}},
			{HPWHSystemType::UNKNOWN, {"UNKNOWN", "None", "None"}}
		};
		inline std::shared_ptr<Courier::Courier> logger;
		void set_logger(std::shared_ptr<Courier::Courier> value);
		struct Schema {
			static constexpr std::string_view schema_title = "HPWHsim Input";
			static constexpr std::string_view schema_version = "0.1.0";
			static constexpr std::string_view schema_description = "Input required to describe a heat pump water heating system in HPWHsim";
		};
		struct HPWHSimInput {
			core::Metadata metadata;
			bool metadata_is_set = false;
			static constexpr std::string_view metadata_units = "";
			static constexpr std::string_view metadata_description = "Metadata data group";
			static constexpr std::string_view metadata_name = "metadata";
			int number_of_nodes;
			bool number_of_nodes_is_set = false;
			static constexpr std::string_view number_of_nodes_units = "";
			static constexpr std::string_view number_of_nodes_description = "Number of tank nodes used for simulation";
			static constexpr std::string_view number_of_nodes_name = "number_of_nodes";
			bool depresses_temperature;
			bool depresses_temperature_is_set = false;
			static constexpr std::string_view depresses_temperature_units = "";
			static constexpr std::string_view depresses_temperature_description = "Depresses space temperature when activated";
			static constexpr std::string_view depresses_temperature_name = "depresses_temperature";
			bool fixed_volume;
			bool fixed_volume_is_set = false;
			static constexpr std::string_view fixed_volume_units = "";
			static constexpr std::string_view fixed_volume_description = "Tank is not allowed to change volume";
			static constexpr std::string_view fixed_volume_name = "fixed_volume";
			double standard_setpoint;
			bool standard_setpoint_is_set = false;
			static constexpr std::string_view standard_setpoint_units = "K";
			static constexpr std::string_view standard_setpoint_description = "Standard setpoint";
			static constexpr std::string_view standard_setpoint_name = "standard_setpoint";
			hpwh_sim_input::HPWHSystemType system_type;
			bool system_type_is_set = false;
			static constexpr std::string_view system_type_units = "";
			static constexpr std::string_view system_type_description = "System Type";
			static constexpr std::string_view system_type_name = "system_type";
			rsintegratedwaterheater::RSINTEGRATEDWATERHEATER integrated_system;
			bool integrated_system_is_set = false;
			static constexpr std::string_view integrated_system_units = "";
			static constexpr std::string_view integrated_system_description = "The corresponding Standard 205 integrated water heater system";
			static constexpr std::string_view integrated_system_name = "integrated_system";
			central_water_heating_system::CentralWaterHeatingSystem central_system;
			bool central_system_is_set = false;
			static constexpr std::string_view central_system_units = "";
			static constexpr std::string_view central_system_description = "The corresponding data group containing Standard 205 system component representations";
			static constexpr std::string_view central_system_name = "central_system";
			bool use_cop_for_condenser_performance;
			bool use_cop_for_condenser_performance_is_set = false;
			static constexpr std::string_view use_cop_for_condenser_performance_units = "";
			static constexpr std::string_view use_cop_for_condenser_performance_description = "Use coefficient-of-performance for condenser computations";
			static constexpr std::string_view use_cop_for_condenser_performance_name = "use_cop_for_condenser_performance";
		};
		NLOHMANN_JSON_SERIALIZE_ENUM (HPWHSystemType, {
			{HPWHSystemType::UNKNOWN, "UNKNOWN"},
			{HPWHSystemType::INTEGRATED, "INTEGRATED"},
			{HPWHSystemType::CENTRAL, "CENTRAL"}
		})
		void from_json(const nlohmann::json& j, HPWHSimInput& x);
		void to_json(nlohmann::json& j, const HPWHSimInput& x);
	}
}
#endif
