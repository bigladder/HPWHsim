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
    findProductInformation();
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

//-----------------------------------------------------------------------------
///	@brief	Set resistance product information based on HPWH model
/// @note	Add entries, as needed
//-----------------------------------------------------------------------------
void HPWH::Resistance::findProductInformation()
{
    switch (hpwh->getModel())
    {
    case MODELS_LG_APHWC50: // supress empty-switch warning
        break;
    default:
        break;
    }
}

void HPWH::Resistance::from(
    const std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& hs)
{
    auto hsp = reinterpret_cast<
        hpwh_data_model::rsresistancewaterheatsource::RSRESISTANCEWATERHEATSOURCE*>(hs.get());

    if (hsp->description_is_set)
    {
        auto& desc = hsp->description;
        if (desc.product_information_is_set)
        {
            auto& info = desc.product_information;
            productInformation.manufacturer = {info.manufacturer, info.manufacturer_is_set};
            productInformation.model_number = {info.model_number, info.model_number_is_set};
        }
    }

    auto& perf = hsp->performance;
    power_kW = perf.input_power / 1000.;
}

void HPWH::Resistance::to(std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& hs) const
{
    auto hsp = reinterpret_cast<
        hpwh_data_model::rsresistancewaterheatsource::RSRESISTANCEWATERHEATSOURCE*>(hs.get());

    auto& metadata = hsp->metadata;
    checkTo(std::string("RSRESISTANCEWATERHEATSOURCE"),
            metadata.schema_name_is_set,
            metadata.schema_name);

    // description/product_information
    auto& desc = hsp->description;
    auto& prod_info = desc.product_information;

    prod_info.manufacturer_is_set = productInformation.manufacturer.isSet();
    prod_info.manufacturer = productInformation.manufacturer();

    prod_info.model_number_is_set = productInformation.model_number.isSet();
    prod_info.model_number = productInformation.model_number();

    bool prod_info_is_set = prod_info.manufacturer_is_set || prod_info.model_number_is_set;
    checkTo(prod_info, desc.product_information_is_set, desc.product_information, prod_info_is_set);

    bool desc_is_set = prod_info_is_set;
    checkTo(desc, hsp->description_is_set, hsp->description, desc_is_set);

    //
    auto& perf = hsp->performance;
    checkTo(1000. * power_kW, perf.input_power_is_set, perf.input_power);
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
