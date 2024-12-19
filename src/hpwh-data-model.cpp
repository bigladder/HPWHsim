
#include <hpwh-data-model.h>

#define JSON_HAS_CPP_17

namespace hpwh_data_model
{
void init(std::shared_ptr<Courier::Courier> logger_in)
{
    hpwh_data_model::ashrae205_ns::logger = logger_in;
    hpwh_data_model::core_ns::logger = logger_in;
    hpwh_data_model::hpwh_sim_input_ns::logger = logger_in;
    hpwh_data_model::rsintegratedwaterheater_ns::logger = logger_in;
    hpwh_data_model::rstank_ns::logger = logger_in;
    hpwh_data_model::rscondenserwaterheatsource_ns::logger = logger_in;
    hpwh_data_model::rsresistancewaterheatsource_ns::logger = logger_in;
    hpwh_data_model::heat_source_configuration_ns::logger = logger_in;
    hpwh_data_model::central_water_heating_system_ns::logger = logger_in;
}
} // namespace hpwh_data_model
