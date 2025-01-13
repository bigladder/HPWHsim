#ifndef HPWHHEATINGLOGIC_hh
#define HPWHHEATINGLOGIC_hh

#include "HPWH.hh"

struct HPWH::HeatingLogic
{
  public:
    static const int LOGIC_SIZE =
        12; /**< number of logic nodes associated with temperature-based heating logic */

    std::string description;
    std::function<bool(double, double)> compare;

    HeatingLogic(std::string desc,
                 double decisionPoint_in,
                 HPWH* hpwh_in,
                 std::function<bool(double, double)> c,
                 bool isHTS)
        : description(desc)
        , compare(c)
        , decisionPoint(decisionPoint_in)
        , hpwh(hpwh_in)
        , isEnteringWaterHighTempShutoff(isHTS) {};

    virtual ~HeatingLogic() = default;

    /**< checks that the input is all valid. */
    virtual bool isValid() = 0;
    /**< gets the value for comparing the tank value to, i.e. the target SoC */
    virtual double getComparisonValue() = 0;
    /**< gets the calculated value from the tank, i.e. SoC or tank average of node weights*/
    virtual double getTankValue() = 0;
    /**< function to calculate where the average node for a logic set is. */
    virtual double nodeWeightAvgFract() = 0;
    /**< gets the fraction of a node that has to be heated up to met the turnoff condition*/
    virtual double getFractToMeetComparisonExternal() = 0;

    virtual void setDecisionPoint(double value) = 0;
    double getDecisionPoint() { return decisionPoint; }
    bool getIsEnteringWaterHighTempShutoff() { return isEnteringWaterHighTempShutoff; }

    static std::shared_ptr<HeatingLogic>
    make(const hpwh_data_model::heat_source_configuration_ns::HeatingLogic& logic, HPWH* hpwh);

    virtual void
    to(hpwh_data_model::heat_source_configuration_ns::HeatingLogic& heatingLogic) const = 0;

  protected:
    double decisionPoint;
    HPWH* hpwh;
    bool isEnteringWaterHighTempShutoff;
};

struct HPWH::SoCBasedHeatingLogic : HPWH::HeatingLogic
{
  public:
    SoCBasedHeatingLogic(std::string desc,
                         double decisionPoint,
                         HPWH* hpwh,
                         double hF = -0.05,
                         double tM_C = 43.333,
                         bool constMains = false,
                         double mains_C = 18.333,
                         std::function<bool(double, double)> c = std::less<double>())
        : HeatingLogic(desc, decisionPoint, hpwh, c, false)
        , tempMinUseful_C(tM_C)
        , hysteresisFraction(hF)
        , useCostantMains(constMains)
        , constantMains_C(mains_C) {};
    bool isValid() override;

    double getComparisonValue() override;
    double getTankValue() override;
    double nodeWeightAvgFract() override;
    double getFractToMeetComparisonExternal() override;
    double getMainsT_C();
    double getTempMinUseful_C();
    void setDecisionPoint(double value) override;
    void setConstantMainsTemperature(double mains_C);

    // void to(hpwh_data_model::rsintegratedwaterheater_ns::SoCBasedHeatingLogic& heating_logic);
    void
    to(hpwh_data_model::heat_source_configuration_ns::HeatingLogic& heatingLogic) const override;

  private:
    double tempMinUseful_C;
    double hysteresisFraction;
    bool useCostantMains;
    double constantMains_C;
};

struct HPWH::TempBasedHeatingLogic : HPWH::HeatingLogic
{
  public:
    TempBasedHeatingLogic(std::string desc,
                          std::vector<NodeWeight> n,
                          double decisionPoint,
                          HPWH* hpwh,
                          bool a = false,
                          std::function<bool(double, double)> c = std::less<double>(),
                          bool isHTS = false);

        //: HeatingLogic(desc, decisionPoint, hpwh, c, isHTS), isAbsolute(a), nodeWeights(n)  {}

    TempBasedHeatingLogic(std::string desc,
                          Distribution dist_in,
                          double decisionPoint,
                          HPWH* hpwh,
                          bool a = false,
                          std::function<bool(double, double)> c = std::less<double>(),
                          bool isHTS = false)
        : HeatingLogic(desc, decisionPoint, hpwh, c, isHTS), isAbsolute(a), dist(dist_in) {}

    bool isValid() override;

    double getComparisonValue() override;
    double getTankValue() override;
    double nodeWeightAvgFract() override;
    double getFractToMeetComparisonExternal() override;

    void setDecisionPoint(double value) override;
    void setDecisionPoint(double value, bool absolute);

    void
    to(hpwh_data_model::heat_source_configuration_ns::HeatingLogic& heatingLogic) const override;

    bool isAbsolute;
    //std::vector<NodeWeight> nodeWeights;
    Distribution dist;

  private:
    bool areNodeWeightsValid();
};

#endif
