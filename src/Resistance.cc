/*
 * Implementation of class HPWH::Resistance
 */

#include "HPWH.hh"
#include "HPWHUtils.hh"
#include "Resistance.hh"

HPWH::Resistance::Resistance(HPWH* hpwh_in,
                             const std::shared_ptr<Courier::Courier> courier_in,
                             const std::string& name_in)
    : HPWH::HeatSource(hpwh_in, courier_in, name_in), power_kW(0.)
{
}

HPWH::Resistance::Resistance(const Resistance& r_in) : HeatSource(r_in), power_kW(r_in.power_kW) {}

HPWH::Resistance& HPWH::Resistance::operator=(const HPWH::Resistance& r_in)
{
    if (this == &r_in)
    {
        return *this;
    }

    HeatSource::operator=(r_in);
    power_kW = r_in.power_kW;

    return *this;
}

void HPWH::Resistance::from(
    const std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& rshs_ptr)
{
    auto res_ptr = reinterpret_cast<
        hpwh_data_model::rsresistancewaterheatsource::RSRESISTANCEWATERHEATSOURCE*>(rshs_ptr.get());

    auto& perf = res_ptr->performance;
    power_kW = perf.input_power / 1000.;
}

void HPWH::Resistance::to(
    std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& rshs_ptr) const
{
    auto res_ptr = reinterpret_cast<
        hpwh_data_model::rsresistancewaterheatsource::RSRESISTANCEWATERHEATSOURCE*>(rshs_ptr.get());

    generate_metadata<hpwh_data_model::rstank::Schema>(
        *res_ptr,
        "RSRESISTANCEWATERHEATSOURCE",
        "https://github.com/bigladder/hpwh-data-model/blob/main/schema/"
        "RSRESISTANCEWATERHEATSOURCE.schema.yaml");
    auto& perf = res_ptr->performance;
    checkTo(1000. * power_kW, perf.input_power_is_set, perf.input_power);

    res_ptr->performance_is_set = true;
}

void HPWH::Resistance::setup(int node, double Watts, int condensitySize /* = CONDENSITY_SIZE*/)
{
    isOn = false;
    isVIP = false;

    double dnode = 1. / condensitySize;
    double beginFrac = dnode * node;
    double endFrac = dnode * (node + 1.);
    heatDist = {};
    if (beginFrac > 0.)
        heatDist.push_back({beginFrac, 0.});
    heatDist.push_back({endFrac, 1.});
    if (endFrac < 1.)
        heatDist.push_back({1., 0.});
    power_kW = Watts / 1000.;
}

void HPWH::Resistance::addHeat(double minutesToRun)
{
    std::vector<double> heatDistribution;
    calcHeatDist(heatDistribution);

    double cap_kJ = power_kW * (minutesToRun * sec_per_min);
    double leftoverCap_kJ = heat(cap_kJ, 100.);

    runtime_min = (1. - (leftoverCap_kJ / cap_kJ)) * minutesToRun;
    if (runtime_min < -TOL_MINVALUE)
    {
        send_error(fmt::format("Negative runtime: {:g} min", runtime_min));
    }

    // update the input & output energy
    energyInput_kWh += power_kW * (runtime_min / min_per_hr);
    energyOutput_kWh += power_kW * (runtime_min / min_per_hr);
}

bool HPWH::Resistance::toLockOrUnlock()
{
    if (isLockedOut())
    {
        lockOutHeatSource();
    }
    if (!isLockedOut())
    {
        unlockHeatSource();
    }

    return isLockedOut();
}
