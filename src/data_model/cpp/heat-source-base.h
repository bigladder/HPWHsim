#ifndef HEATSOURCEBASE_H_
#define HEATSOURCEBASE_H_
#include <ashrae205.h>

#include <string>
#include <vector>
#include <enum-info.h>
#include <courier/courier.h>
#include <nlohmann/json.hpp>

/// @note  This class has been auto-generated. Local changes will not be saved!

namespace data_model
{
struct HeatSourceBase
{
    virtual void initialize(const nlohmann::json& j) = 0;
    virtual ~HeatSourceBase() = default;
};
} // namespace hpwh_data_model

#endif
