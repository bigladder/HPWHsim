#ifndef HEATINGLOGICBASE_H_
#define HEATINGLOGICBASE_H_
namespace data_model
{
struct HeatingLogicBase
{
    virtual void initialize(const nlohmann::json& j) = 0;
    virtual ~HeatingLogicBase() = default;
};
} // namespace data_model
#endif
