#ifndef RS_INSTANCE_BASE_H_
#define RS_INSTANCE_BASE_H_

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>

/// @class RSInstanceBase RS_instance_base.h
/// @brief This class is an abstract base for representation specification classes. It handles no
/// resources.

namespace hpwh_data_model
{

class RSInstance
{
  public:
    RSInstance() = default;
    virtual ~RSInstance() = default;
    RSInstance(const RSInstance& other) = default;
    RSInstance& operator=(const RSInstance& other) = default;
    RSInstance(RSInstance&&) = default;
    RSInstance& operator=(RSInstance&&) = default;

    virtual void initialize(const nlohmann::json& j) = 0;
};
} // namespace hpwh_data_model

#endif
