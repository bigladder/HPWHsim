/*
 * Implementation of class HPWH::HeatSource
 */

#include <algorithm>
#include <regex>

// vendor
#include <btwxt/btwxt.h>

#include "HPWH.hh"
#include "HPWHUtils.hh"
#include "HPWHHeatingLogic.hh"
#include "HPWHHeatSource.hh"
#include "Tank.hh"

// public HPWH::HeatSource functions
HPWH::HeatSource::HeatSource(
    HPWH* hpwh_in,
    const std::shared_ptr<Courier::Courier> courier_in /*std::make_shared<DefaultCourier>()*/,
    const std::string& name_in)
    : Sender("HeatSource", name_in, courier_in)
    , lockedOut(false)
    , hpwh(hpwh_in)
    , isOn(false)
    , backupHeatSource(NULL)
    , companionHeatSource(NULL)
    , followedByHeatSource(NULL)
    , standbyLogic(NULL)
    , minT(-273.15)
    , maxT(100.)
    , airflowFreedom(1.0)
    , externalInletHeight(-1)
    , externalOutletHeight(-1)
{
    parent_pointer = hpwh;
}

HPWH::HeatSource::HeatSource(const HeatSource& hSource) : Sender(hSource) { *this = hSource; }

HPWH::HeatSource& HPWH::HeatSource::operator=(const HeatSource& hSource)
{
    if (this == &hSource)
    {
        return *this;
    }
    Sender::operator=(hSource);
    hpwh = hSource.hpwh;

    isOn = hSource.isOn;

    runtime_min = hSource.runtime_min;
    energyInput_kWh = hSource.energyInput_kWh;
    energyOutput_kWh = hSource.energyOutput_kWh;

    isVIP = hSource.isVIP;

    if (hSource.backupHeatSource != NULL || hSource.companionHeatSource != NULL ||
        hSource.followedByHeatSource != NULL)
    {
        send_error("HeatSources cannot be copied if they contain pointers to other HeatSources.");
    }
    else
    {
        companionHeatSource = NULL;
        backupHeatSource = NULL;
        followedByHeatSource = NULL;
    }

    heatDist = hSource.heatDist;

    Tshrinkage_C = hSource.Tshrinkage_C;

    turnOnLogicSet = hSource.turnOnLogicSet;
    shutOffLogicSet = hSource.shutOffLogicSet;
    standbyLogic = hSource.standbyLogic;

    minT = hSource.minT;
    maxT = hSource.maxT;

    depressesTemperature = hSource.depressesTemperature;
    airflowFreedom = hSource.airflowFreedom;

    externalInletHeight = hSource.externalInletHeight;
    externalOutletHeight = hSource.externalOutletHeight;

    lowestNode = hSource.lowestNode;

    return *this;
}

void HPWH::HeatSource::from(
    const hpwh_data_model::heat_source_configuration::HeatSourceConfiguration& hsc)
{
    auto& config = hsc;
    checkFrom(name, config.id_is_set, config.id, std::string("heatsource"));

    if (config.heat_distribution_is_set)
    {
        heatDist = {config.heat_distribution.normalized_height, config.heat_distribution.weight};
    }

    if (config.turn_on_logic_is_set)
    {
        for (auto& turn_on_logic : config.turn_on_logic)
        {
            auto heatingLogic = HeatingLogic::make(turn_on_logic, hpwh);
            if (heatingLogic)
            {
                addTurnOnLogic(heatingLogic);
            }
        }
    }

    if (config.shut_off_logic_is_set)
    {
        for (auto& shut_off_logic : config.shut_off_logic)
        {
            auto heatingLogic = HeatingLogic::make(shut_off_logic, hpwh);
            if (heatingLogic)
            {
                addShutOffLogic(heatingLogic);
            }
        }
    }

    if (config.standby_logic_is_set)
    {
        auto heatingLogic = HeatingLogic::make(config.standby_logic, hpwh);
        if (heatingLogic)
        {
            standbyLogic = std::move(heatingLogic);
        }
    }
}

void HPWH::HeatSource::to(
    hpwh_data_model::heat_source_configuration::HeatSourceConfiguration& hsc) const
{
    std::vector<double> heights = {}, weights = {};
    for (std::size_t i = 0; i < heatDist.size(); ++i)
    {
        heights.push_back(heatDist.unitaryHeight(i));
        weights.push_back(heatDist.unitaryWeight(i));
    }

    hpwh_data_model::heat_source_configuration::WeightedDistribution wd;
    checkTo(heights, wd.normalized_height_is_set, wd.normalized_height);
    checkTo(weights, wd.weight_is_set, wd.weight);
    checkTo(wd, hsc.heat_distribution_is_set, hsc.heat_distribution);

    hsc.id = name;

    hsc.shut_off_logic.resize(shutOffLogicSet.size());
    std::size_t i = 0;
    for (auto& shutOffLogic : shutOffLogicSet)
    {
        shutOffLogic->to(hsc.shut_off_logic[i]);
        hsc.shut_off_logic_is_set = true;
        ++i;
    }

    hsc.turn_on_logic.resize(turnOnLogicSet.size());
    i = 0;
    for (auto& turnOnLogic : turnOnLogicSet)
    {
        turnOnLogic->to(hsc.turn_on_logic[i]);
        hsc.turn_on_logic_is_set = true;
        ++i;
    }

    if (standbyLogic != NULL)
    {
        standbyLogic->to(hsc.standby_logic);
        hsc.standby_logic_is_set = true;
    }

    if (backupHeatSource != NULL)
        checkTo(
            backupHeatSource->name, hsc.backup_heat_source_id_is_set, hsc.backup_heat_source_id);

    if (followedByHeatSource != NULL)
        checkTo(followedByHeatSource->name,
                hsc.followed_by_heat_source_id_is_set,
                hsc.followed_by_heat_source_id);

    if (companionHeatSource != NULL)
        checkTo(companionHeatSource->name,
                hsc.companion_heat_source_id_is_set,
                hsc.companion_heat_source_id);

    switch (typeOfHeatSource())
    {
    case TYPE_compressor:
    {
        if (hpwh->isCompressorExternal())
        {
            hsc.heat_source_type =
                hpwh_data_model::heat_source_configuration::HeatSourceType::AIRTOWATERHEATPUMP;
            hsc.heat_source =
                std::make_unique<hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP>();
        }
        else
        {
            hsc.heat_source_type =
                hpwh_data_model::heat_source_configuration::HeatSourceType::CONDENSER;
            hsc.heat_source = std::make_unique<
                hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE>();
        }
        to(hsc.heat_source);
        break;
    }

    case TYPE_resistance:
    {
        hsc.heat_source_type =
            hpwh_data_model::heat_source_configuration::HeatSourceType::RESISTANCE;
        hsc.heat_source = std::make_unique<
            hpwh_data_model::rsresistancewaterheatsource::RSRESISTANCEWATERHEATSOURCE>();
        to(hsc.heat_source);
        break;
    }

    default:
        return;
        break;
    }
}

void HPWH::HeatSource::setCondensity(const std::vector<double>& condensity_in)
{
    heatDist = {{}, {}};
    // double prev_height = 0.;
    // double prev_weight = 0.;
    auto nCond = condensity_in.size();
    for (std::size_t i = 0; i < nCond; ++i)
    {
        double height = static_cast<double>(i + 1) / nCond;
        double weight = condensity_in[i];
        if (i == nCond - 1)
        {
            heatDist.push_back({height, weight});
            break;
        }
        if (weight != condensity_in[i + 1])
        {
            heatDist.push_back({height, weight});
        }
    }
}

int HPWH::HeatSource::findParent() const
{
    for (int i = 0; i < hpwh->getNumHeatSources(); ++i)
    {
        if (this == hpwh->heatSources[i]->backupHeatSource)
        {
            return i;
        }
    }
    return -1;
}

bool HPWH::HeatSource::isEngaged() const { return isOn; }

void HPWH::HeatSource::engageHeatSource(DRMODES DR_signal)
{
    isOn = true;
    hpwh->isHeating = true;
    if ((companionHeatSource != NULL) && !(companionHeatSource->shutsOff()) &&
        !(companionHeatSource->isEngaged()) &&
        !(hpwh->shouldDRLockOut(companionHeatSource->typeOfHeatSource(), DR_signal)))
    {
        companionHeatSource->engageHeatSource(DR_signal);
    }
}

void HPWH::HeatSource::disengageHeatSource() { isOn = false; }

bool HPWH::HeatSource::shouldHeat() const
{
    // return true if the heat source logic tells it to come on, false if it doesn't,
    // or if an unsepcified selector was used
    bool shouldEngage = false;

    for (int i = 0; i < (int)turnOnLogicSet.size(); i++)
    {
        double average = turnOnLogicSet[i]->getTankValue();
        double comparison = turnOnLogicSet[i]->getComparisonValue();

        if (turnOnLogicSet[i]->compare(average, comparison))
        {
            if (turnOnLogicSet[i]->checksStandby() && standbyLogic != NULL)
            {
                double avgStandby = standbyLogic->getTankValue();
                double comparisonStandby = standbyLogic->getComparisonValue();

                if (turnOnLogicSet[i]->compare(avgStandby, comparisonStandby))
                {
                    shouldEngage = true;
                }
            }
            else
            {
                shouldEngage = true;
            }
        }

        // quit searching the logics if one of them turns it on
        if (shouldEngage)
        {
            // debugging message handling
            break;
        }

    } // end loop over set of logic conditions

    // if everything else wants it to come on, but if it would shut off anyways don't turn it on
    if (shouldEngage && shutsOff())
    {
        shouldEngage = false;
    }

    return shouldEngage;
}

bool HPWH::HeatSource::shutsOff() const
{
    bool shutOff = false;

    if (hpwh->tank->nodeTs_C[0] >= hpwh->setpoint_C)
    {
        shutOff = true;
        return shutOff;
    }

    for (int i = 0; i < (int)shutOffLogicSet.size(); i++)
    {
        double average = shutOffLogicSet[i]->getTankValue();
        double comparison = shutOffLogicSet[i]->getComparisonValue();

        if (shutOffLogicSet[i]->compare(average, comparison))
        {
            shutOff = true;
        }
    }

    return shutOff;
}

double HPWH::HeatSource::fractToMeetComparisonExternal() const
{
    double fracTemp;
    double frac = 1.;

    for (int i = 0; i < (int)shutOffLogicSet.size(); i++)
    {
        fracTemp = shutOffLogicSet[i]->getFractToMeetComparisonExternal();
        frac = fracTemp < frac ? fracTemp : frac;
    }
    return frac;
}

double HPWH::HeatSource::heat(double cap_kJ, const double maxSetpointT_C)
{
    std::vector<double> heatDistribution;

    // calcHeatDist takes care of the swooping for wrapped configurations
    calcHeatDist(heatDistribution);

    // set the leftover capacity to 0
    // auto distPoint = heatDist.rbegin();
    // double totalWeight = heatDist.totalWeight();
    int numNodes = hpwh->getNumNodes();
    double leftoverCap_kJ = 0.;
    for (int i = numNodes - 1; i >= 0; i--)
    {
        double nodeCap_kJ = cap_kJ * heatDistribution[i];
        if (nodeCap_kJ != 0.)
        {
            double heatToAdd_kJ = nodeCap_kJ + leftoverCap_kJ;
            // add leftoverCap to the next run, and keep passing it on
            leftoverCap_kJ = hpwh->addHeatAboveNode(heatToAdd_kJ, i, maxSetpointT_C);
        }
    }
    return leftoverCap_kJ;
}

double HPWH::HeatSource::getTankTemp() const { return hpwh->getAverageTankTemp_C(heatDist); }

void HPWH::HeatSource::calcHeatDist(std::vector<double>& heatDistribution)
{
    // Populate the vector of heat distribution
    int numNodes = hpwh->getNumNodes();
    heatDistribution.resize(numNodes);
    double beginFrac = 0.;
    int i = 0;
    double totalWeight = 0.;
    for (auto& dist : heatDistribution)
    {
        double endFrac = static_cast<double>(i + 1) / numNodes;
        dist = heatDist.normalizedWeight(beginFrac, endFrac);
        beginFrac = endFrac;
        ++i;
        totalWeight += dist;
    }
    for (auto& dist : heatDistribution)
        dist /= totalWeight;
}

void HPWH::HeatSource::addTurnOnLogic(std::shared_ptr<HeatingLogic> logic)
{
    turnOnLogicSet.push_back(logic);
}

void HPWH::HeatSource::addShutOffLogic(std::shared_ptr<HeatingLogic> logic)
{
    shutOffLogicSet.push_back(logic);
}

void HPWH::HeatSource::clearAllTurnOnLogic() { this->turnOnLogicSet.clear(); }

void HPWH::HeatSource::clearAllShutOffLogic() { this->shutOffLogicSet.clear(); }

void HPWH::HeatSource::clearAllLogic()
{
    clearAllTurnOnLogic();
    clearAllShutOffLogic();
}
