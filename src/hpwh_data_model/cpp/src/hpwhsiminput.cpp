#include <hpwhsiminput.h>
#include <load-object.h>

namespace hpwh_data_model
{

namespace hpwhsiminput_ns
{

void set_logger(std::shared_ptr<Courier::Courier> value) { logger = value; }

const std::string_view Schema::schema_title = "HPWHsim Input";

const std::string_view Schema::schema_version = "0.1.0";

const std::string_view Schema::schema_description =
    "Input required to describe a heat pump water heating system in HPWHsim";

} // namespace hpwhsiminput_ns
} // namespace hpwh_data_model
