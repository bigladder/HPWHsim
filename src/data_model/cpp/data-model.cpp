
#include <data-model.h>


#define JSON_HAS_CPP_17

namespace data_model
{
void init(std::shared_ptr<Courier::Courier> logger_in)
{
    data_model::ashrae205_ns::logger = logger_in;
    data_model::core_ns::logger = logger_in;
    data_model::rsintegratedwaterheater_ns::logger = logger_in;
    data_model::rstank_ns::logger = logger_in;
    data_model::rscondenserwaterheatsource_ns::logger = logger_in;
    data_model::rsresistancewaterheatsource_ns::logger = logger_in;
}
}