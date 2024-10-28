#ifndef HPWHSIMINPUT_H_
#define HPWHSIMINPUT_H_
#include <ashrae205.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <enum-info.h>
#include <courier/courier.h>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace hpwh_data_model
{
namespace hpwhsiminput_ns
{
inline std::shared_ptr<Courier::Courier> logger;
void set_logger(std::shared_ptr<Courier::Courier> value);
struct Schema
{
    const static std::string_view schema_title;
    const static std::string_view schema_version;
    const static std::string_view schema_description;
};
} // namespace hpwhsiminput_ns
} // namespace hpwh_data_model
#endif
