#ifndef HEATINGLOGICBASE_H_
#define HEATINGLOGICBASE_H_
#include <ashrae205.h>

#include <string>
#include <vector>
#include <enum-info.h>
#include <courier/courier.h>
#include <nlohmann/json.hpp>

/// @note  This class has been auto-generated. Local changes will not be saved!
namespace hpwh_data_model
{
struct HeatingLogicBase
{
    virtual void initialize(const nlohmann::json& j) = 0;
    virtual ~HeatingLogicBase() = default;
};
}

#endif
