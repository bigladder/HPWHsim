/*
 * Implementation of class HPWH::Resistance
 */

#include "HPWH.hh"
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

void HPWH::Resistance::from(const std::unique_ptr<HeatSourceTemplate>& rshs_ptr)
{
    auto res_ptr = reinterpret_cast<
        hpwh_data_model::rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE*>(
        rshs_ptr.get());

    auto& perf = res_ptr->performance;
    power_kW = perf.input_power / 1000.;
}

void HPWH::Resistance::to(std::unique_ptr<HeatSourceTemplate>& rshs_ptr) const
{
    auto res_ptr = reinterpret_cast<
        hpwh_data_model::rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE*>(
        rshs_ptr.get());

    auto& metadata = res_ptr->metadata;
    checkTo(hpwh_data_model::ashrae205_ns::SchemaType::RSRESISTANCEWATERHEATSOURCE,
            metadata.schema_is_set,
            metadata.schema);

    auto& perf = res_ptr->performance;
    checkTo(power_kW, perf.input_power_is_set, perf.input_power);
}

void HPWH::Resistance::setup(int node, double Watts, int condensitySize /* = CONDENSITY_SIZE*/)
{
    isOn = false;
    isVIP = false;
    condensity = std::vector<double>(condensitySize, 0.);
    condensity[node] = 1;
    power_kW = Watts / 1000.;
}

void HPWH::Resistance::addHeat(double minutesToRun)
{
    std::vector<double> heatDistribution;
    calcHeatDist(heatDistribution);

    double cap_kJ = power_kW * (minutesToRun * sec_per_min);
    double leftoverCap_kJ = heat(cap_kJ);

    runtime_min = (1. - (leftoverCap_kJ / cap_kJ)) * minutesToRun;
    if (runtime_min < -TOL_MINVALUE)
    {
        send_error(fmt::format("Negative runtime: {:g} min", runtime_min));
    }

    // update the input & output energy
    energyInput_kWh += power_kW * (runtime_min / min_per_hr);
    energyOutput_kWh += power_kW * (runtime_min / min_per_hr);
}