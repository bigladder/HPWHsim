#ifndef RESISTANCE_hh
#define RESISTANCE_hh

#include "HPWH.hh"
#include "HPWHHeatSource.hh"

class HPWH::Resistance : public HPWH::HeatSource
{
  private:
    double power_kW;

  public:
    Resistance(HPWH* hpwh_in = NULL,
               const std::shared_ptr<Courier::Courier> courier = std::make_shared<DefaultCourier>(),
               const std::string& name_in = "resistance");

    Resistance(const Resistance& r_in);

    Resistance& operator=(const Resistance& r_in);

    HEATSOURCE_TYPE typeOfHeatSource() const override { return HPWH::TYPE_resistance; }
    void from(const std::unique_ptr<HeatSourceBase>& rshs_ptr) override;
    void to(std::unique_ptr<HeatSourceBase>& rshs_ptr) const override;
    // void calcHeatDist(std::vector<double>& heatDistribution) override;

    void setup(int node, double Watts, int condensitySize = CONDENSITY_SIZE);

    double getPower_W() { return 1000. * power_kW; }
    void setPower_W(double watts) { power_kW = watts / 1000.; }

    void addHeat(double minutesToRun);
};
#endif
