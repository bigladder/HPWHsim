
#include <hpwh-data-model.h>

#define JSON_HAS_CPP_17

namespace hpwh_data_model
{
void init(std::shared_ptr<Courier::Courier> logger_in)
{
    hpwh_data_model::ashrae205::logger = logger_in;
    hpwh_data_model::core::logger = logger_in;
    hpwh_data_model::hpwh_sim_input::logger = logger_in;
    hpwh_data_model::rsintegratedwaterheater::logger = logger_in;
    hpwh_data_model::rstank::logger = logger_in;
    hpwh_data_model::rscondenserwaterheatsource::logger = logger_in;
    hpwh_data_model::rsresistancewaterheatsource::logger = logger_in;
    hpwh_data_model::heat_source_configuration::logger = logger_in;
    hpwh_data_model::central_water_heating_system::logger = logger_in;
}
} // namespace hpwh_data_model
