#ifndef HEATINGLOGICBASE_H_
#define HEATINGLOGICBASE_H_
struct HeatingLogicBase {
    virtual void initialize(const nlohmann::json& j) = 0;
    virtual ~HeatingLogicBase() = default;
};
#endif
