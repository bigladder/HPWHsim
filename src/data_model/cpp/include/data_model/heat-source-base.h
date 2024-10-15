#ifndef HEATSOURCEBASE_H_
#define HEATSOURCEBASE_H_
struct HeatSourceBase
{
    virtual void initialize(const nlohmann::json& j) = 0;
    virtual ~HeatSourceBase() = default;
};
#endif
