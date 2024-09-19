#ifndef HEATSOURCEBASE_H_
#define HEATSOURCEBASE_H_
namespace data_model
{
struct HeatSourceBase
{
    virtual void initialize(const nlohmann::json& j) = 0;
    virtual ~HeatSourceBase() = default;
};
} // namespace data_model

#endif
