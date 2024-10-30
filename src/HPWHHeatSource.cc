/*
 * Implementation of class HPWH::HeatSource
 */

#include <algorithm>
#include <regex>

// vendor
#include <btwxt/btwxt.h>

#include "HPWH.hh"
#include "HPWHHeatingLogic.hh"
#include "HPWHHeatSource.hh"
#include "HPWHTank.hh"

// public HPWH::HeatSource functions
HPWH::HeatSource::HeatSource(
    HPWH* hpwh_in,
    const std::shared_ptr<Courier::Courier> courier_in /*std::make_shared<DefaultCourier>()*/,
    const std::string& name_in)
    : Sender("HeatSource", name_in, courier_in)
    , hpwh(hpwh_in)
    , isOn(false)
    , lockedOut(false)
    , backupHeatSource(NULL)
    , companionHeatSource(NULL)
    , followedByHeatSource(NULL)
    , standbyLogic(NULL)
    , minT(-273.15)
    , maxT(100.)
    , maxSetpoint_C(100.)
    , hysteresis_dC(0)
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
    lockedOut = hSource.lockedOut;

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

    condensity = hSource.condensity;

    Tshrinkage_C = hSource.Tshrinkage_C;

    turnOnLogicSet = hSource.turnOnLogicSet;
    shutOffLogicSet = hSource.shutOffLogicSet;
    standbyLogic = hSource.standbyLogic;

    minT = hSource.minT;
    maxT = hSource.maxT;

    hysteresis_dC = hSource.hysteresis_dC;
    maxSetpoint_C = hSource.maxSetpoint_C;

    depressesTemperature = hSource.depressesTemperature;
    airflowFreedom = hSource.airflowFreedom;

    externalInletHeight = hSource.externalInletHeight;
    externalOutletHeight = hSource.externalOutletHeight;

    lowestNode = hSource.lowestNode;

    return *this;
}

void HPWH::HeatSource::from(
    hpwh_data_model::rsintegratedwaterheater_ns::HeatSourceConfiguration& heatsourceconfiguration)
{
    auto& config = heatsourceconfiguration;
    checkFrom(name, config.id_is_set, config.id, std::string("heatsource"));
    setCondensity(config.heat_distribution);

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

void HPWH::HeatSource::to(hpwh_data_model::rsintegratedwaterheater_ns::HeatSourceConfiguration&
                              heatsourceconfiguration) const
{
    heatsourceconfiguration.heat_distribution = condensity;
    heatsourceconfiguration.id = name;

    heatsourceconfiguration.shut_off_logic.resize(shutOffLogicSet.size());
    std::size_t i = 0;
    for (auto& shutOffLogic : shutOffLogicSet)
    {
        shutOffLogic->to(heatsourceconfiguration.shut_off_logic[i]);
        heatsourceconfiguration.shut_off_logic_is_set = true;
        ++i;
    }

    heatsourceconfiguration.turn_on_logic.resize(turnOnLogicSet.size());
    i = 0;
    for (auto& turnOnLogic : turnOnLogicSet)
    {
        turnOnLogic->to(heatsourceconfiguration.turn_on_logic[i]);
        heatsourceconfiguration.turn_on_logic_is_set = true;
        ++i;
    }

    if (standbyLogic != NULL)
    {
        standbyLogic->to(heatsourceconfiguration.standby_logic);
        heatsourceconfiguration.standby_logic_is_set = true;
    }

    if (backupHeatSource != NULL)
        checkTo(backupHeatSource->name,
                heatsourceconfiguration.backup_heat_source_id_is_set,
                heatsourceconfiguration.backup_heat_source_id);

    if (followedByHeatSource != NULL)
        checkTo(followedByHeatSource->name,
                heatsourceconfiguration.followed_by_heat_source_id_is_set,
                heatsourceconfiguration.followed_by_heat_source_id);

    if (companionHeatSource != NULL)
        checkTo(companionHeatSource->name,
                heatsourceconfiguration.companion_heat_source_id_is_set,
                heatsourceconfiguration.companion_heat_source_id);

}

void HPWH::HeatSource::getCapacityFromMap(double environmentT_C,
                                          double heatSourceT_C,
                                          double& input_BTUperHr,
                                          double& cop) const
{
    double environmentT_F = C_TO_F(environmentT_C);
    double heatSourceT_F = C_TO_F(heatSourceT_C);

    input_BTUperHr = 0.;
    cop = 0.;

    if (perfMap.size() > 1)
    {
        double COP_T1, COP_T2; // cop at ambient temperatures T1 and T2
        double inputPower_T1_W,
            inputPower_T2_W; // input power at ambient temperatures T1 and T2

        size_t i_prev = 0;
        size_t i_next = 1;
        for (size_t i = 0; i < perfMap.size(); ++i)
        {
            if (environmentT_F < perfMap[i].T_F)
            {
                if (i == 0)
                {
                    i_prev = 0;
                    i_next = 1;
                }
                else
                {
                    i_prev = i - 1;
                    i_next = i;
                }
                break;
            }
            else
            {
                if (i == perfMap.size() - 1)
                {
                    i_prev = i - 1;
                    i_next = i;
                    break;
                }
            }
        }

        // Calculate COP and Input Power at each of the two reference temperatures
        COP_T1 = perfMap[i_prev].COP_coeffs[0];
        COP_T1 += perfMap[i_prev].COP_coeffs[1] * heatSourceT_F;
        COP_T1 += perfMap[i_prev].COP_coeffs[2] * heatSourceT_F * heatSourceT_F;

        COP_T2 = perfMap[i_next].COP_coeffs[0];
        COP_T2 += perfMap[i_next].COP_coeffs[1] * heatSourceT_F;
        COP_T2 += perfMap[i_next].COP_coeffs[2] * heatSourceT_F * heatSourceT_F;

        inputPower_T1_W = perfMap[i_prev].inputPower_coeffs[0];
        inputPower_T1_W += perfMap[i_prev].inputPower_coeffs[1] * heatSourceT_F;
        inputPower_T1_W += perfMap[i_prev].inputPower_coeffs[2] * heatSourceT_F * heatSourceT_F;

        inputPower_T2_W = perfMap[i_next].inputPower_coeffs[0];
        inputPower_T2_W += perfMap[i_next].inputPower_coeffs[1] * heatSourceT_F;
        inputPower_T2_W += perfMap[i_next].inputPower_coeffs[2] * heatSourceT_F * heatSourceT_F;

        // Interpolate to get COP and input power at the current ambient temperature
        linearInterp(cop, environmentT_F, perfMap[i_prev].T_F, perfMap[i_next].T_F, COP_T1, COP_T2);
        linearInterp(input_BTUperHr,
                     environmentT_F,
                     perfMap[i_prev].T_F,
                     perfMap[i_next].T_F,
                     inputPower_T1_W,
                     inputPower_T2_W);
        input_BTUperHr = KWH_TO_BTU(input_BTUperHr / 1000.0); // 1000 converts w to kw;
    }
}

// convert to grid
void HPWH::HeatSource::convertMapToGrid(std::vector<std::vector<double>>& tempGrid,
                                        std::vector<std::vector<double>>& tempGridValues) const
{
    if (perfMap.size() < 2)
        return;

    tempGrid.reserve(2);       // envT, heatSourceT
    tempGridValues.reserve(2); // inputPower, cop

    double maxPowerCurvature = 0.; // curvature used to determine # of points
    double maxCOPCurvature = 0.;
    std::vector<double> envTemps_K = {};
    envTemps_K.reserve(perfMap.size());
    for (auto& perfPoint : perfMap)
    {
        envTemps_K.push_back(C_TO_K(F_TO_C(perfPoint.T_F)));
        double magPowerCurvature = abs(perfPoint.inputPower_coeffs[2]);
        double magCOPCurvature = abs(perfPoint.COP_coeffs[2]);
        maxPowerCurvature =
            magPowerCurvature > maxPowerCurvature ? magPowerCurvature : maxPowerCurvature;
        maxCOPCurvature = magCOPCurvature > maxCOPCurvature ? magCOPCurvature : maxCOPCurvature;
    }
    tempGrid.push_back(envTemps_K);

    // relate to reference values (from Rheem2020Build50)
    constexpr std::size_t minPoints = 2;
    constexpr std::size_t refPowerPoints = 13;
    constexpr std::size_t refCOP_points = 13;
    constexpr double refPowerCurvature = 0.01571;
    constexpr double refCOP_curvature = 0.0002;

    auto nPowerPoints = static_cast<std::size_t>(
        (maxPowerCurvature / refPowerCurvature) * (refPowerPoints - minPoints) + minPoints);
    auto nCOPPoints = static_cast<std::size_t>(
        (maxCOPCurvature / refCOP_curvature) * (refCOP_points - minPoints) + minPoints);
    std::size_t nPoints = std::max(nPowerPoints, nCOPPoints);

    std::vector<double> heatSourceTemps_C(nPoints);
    {
        double T_C = 0;
        double dT_C = 100. / static_cast<double>(nPoints - 1);
        for (auto& heatSourceTemp_C : heatSourceTemps_C)
        {
            heatSourceTemp_C = T_C;
            T_C += dT_C;
        }
    }

    std::vector<double> heatSourceTemps_K = {};
    heatSourceTemps_K.reserve(heatSourceTemps_C.size());

    for (auto T_C : heatSourceTemps_C)
    {
        heatSourceTemps_K.push_back(C_TO_K(T_C));
    }
    tempGrid.push_back(heatSourceTemps_K);

    std::size_t nVals = envTemps_K.size() * heatSourceTemps_K.size();

    std::vector<double> inputPowers_W(nVals), heatingCapacities_W(nVals);

    std::size_t i = 0;
    double input_BTUperHr, cop;
    for (auto& envTemp_K : envTemps_K)
        for (auto& heatSourceTemp_K : heatSourceTemps_K)
        {
            getCapacityFromMap(K_TO_C(envTemp_K), K_TO_C(heatSourceTemp_K), input_BTUperHr, cop);
            inputPowers_W[i] = 1000. * BTUperH_TO_KW(input_BTUperHr);
            heatingCapacities_W[i] = cop * inputPowers_W[i];
            ++i;
        }

    tempGridValues.push_back(inputPowers_W);
    tempGridValues.push_back(heatingCapacities_W);
}

void HPWH::HeatSource::to(hpwh_data_model::rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE&
                              rscondenserwaterheatsource) const
{
    auto& metadata = rscondenserwaterheatsource.metadata;
    checkTo(hpwh_data_model::ashrae205_ns::SchemaType::RSCONDENSERWATERHEATSOURCE,
            metadata.schema_is_set,
            metadata.schema);

    auto& perf = rscondenserwaterheatsource.performance;

    checkTo(maxSetpoint_C,
            perf.maximum_refrigerant_temperature_is_set,
            perf.maximum_refrigerant_temperature);

    checkTo(hysteresis_dC,
            perf.compressor_lockout_temperature_hysteresis_is_set,
            perf.compressor_lockout_temperature_hysteresis,
            hysteresis_dC != 0.);

    checkTo(C_TO_K(minT), perf.minimum_temperature_is_set, perf.minimum_temperature);

    checkTo(C_TO_K(maxT), perf.maximum_temperature_is_set, perf.maximum_temperature);

    switch (configuration)
    {
    case COIL_CONFIG::CONFIG_SUBMERGED:
    {
        perf.coil_configuration =
            hpwh_data_model::rscondenserwaterheatsource_ns::CoilConfiguration::SUBMERGED;
        perf.coil_configuration_is_set = true;
        break;
    }
    case COIL_CONFIG::CONFIG_WRAPPED:
    {
        perf.coil_configuration =
            hpwh_data_model::rscondenserwaterheatsource_ns::CoilConfiguration::WRAPPED;
        perf.coil_configuration_is_set = true;
        break;
    }
    case COIL_CONFIG::CONFIG_EXTERNAL:
    {
        perf.coil_configuration =
            hpwh_data_model::rscondenserwaterheatsource_ns::CoilConfiguration::EXTERNAL;
        perf.coil_configuration_is_set = true;
        break;
    }
    default:
    {
        break;
    }
    }

    if (useBtwxtGrid)
    {
        auto& map = perf.performance_map;

        auto& grid_vars = map.grid_variables;

        std::vector<double> envTemp_K = {};
        envTemp_K.reserve(perfGrid[0].size());
        for (auto T : perfGrid[0])
        {
            envTemp_K.push_back(C_TO_K(F_TO_C(T)));
        }
        checkTo(envTemp_K,
                grid_vars.evaporator_environment_dry_bulb_temperature_is_set,
                grid_vars.evaporator_environment_dry_bulb_temperature);

        std::vector<double> heatSourceTemp_K = {};
        heatSourceTemp_K.reserve(perfGrid[1].size());
        for (auto T : perfGrid[1])
        {
            heatSourceTemp_K.push_back(C_TO_K(F_TO_C(T)));
        }
        checkTo(heatSourceTemp_K,
                grid_vars.heat_source_temperature_is_set,
                grid_vars.heat_source_temperature);

        auto& lookup_vars = map.lookup_variables;

        std::size_t nVals = perfGridValues[0].size();
        std::vector<double> inputPowers_W(nVals), heatingCapacity_W(nVals);
        for (std::size_t i = 0; i < nVals; ++i)
        {
            inputPowers_W[i] = (1000. * BTUperH_TO_KW(perfGridValues[0][i]));
            heatingCapacity_W[i] = perfGridValues[1][i] * inputPowers_W[i];
        }

        checkTo(inputPowers_W, lookup_vars.input_power_is_set, lookup_vars.input_power);
        checkTo(
            perfGridValues[1], lookup_vars.heating_capacity_is_set, lookup_vars.heating_capacity);

        perf.performance_map_is_set = true;
    }
    else // convert to grid
    {
        std::vector<std::vector<double>> tempGrid = {};
        std::vector<std::vector<double>> tempGridValues = {};

        convertMapToGrid(tempGrid, tempGridValues);

        auto& map = perf.performance_map;

        auto& grid_vars = map.grid_variables;
        checkTo(tempGrid[0],
                grid_vars.evaporator_environment_dry_bulb_temperature_is_set,
                grid_vars.evaporator_environment_dry_bulb_temperature);
        checkTo(tempGrid[1],
                grid_vars.heat_source_temperature_is_set,
                grid_vars.heat_source_temperature);

        auto& lookup_vars = map.lookup_variables;
        checkTo(tempGridValues[0], lookup_vars.input_power_is_set, lookup_vars.input_power);
        checkTo(
            tempGridValues[1], lookup_vars.heating_capacity_is_set, lookup_vars.heating_capacity);

        perf.performance_map_is_set = true;
    }
}

void HPWH::HeatSource::to(hpwh_data_model::rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE&
                              rsresistancewaterheatsource) const
{
    auto& metadata = rsresistancewaterheatsource.metadata;
    checkTo(hpwh_data_model::ashrae205_ns::SchemaType::RSRESISTANCEWATERHEATSOURCE,
            metadata.schema_is_set,
            metadata.schema);

    auto& perf = rsresistancewaterheatsource.performance;
    checkTo(perfMap[0].inputPower_coeffs[0], perf.input_power_is_set, perf.input_power);
}

void HPWH::HeatSource::setCondensity(const std::vector<double>& condensity_in)
{
    condensity = condensity_in;
}

int HPWH::HeatSource::getCondensitySize() const { return static_cast<int>(condensity.size()); }

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

bool HPWH::HeatSource::isLockedOut() const { return lockedOut; }

void HPWH::HeatSource::lockOutHeatSource() { lockedOut = true; }

void HPWH::HeatSource::unlockHeatSource() { lockedOut = false; }

bool HPWH::HeatSource::shouldLockOut(double heatSourceAmbientT_C) const
{

    // if it's already locked out, keep it locked out
    if (isLockedOut())
    {
        return true;
    }
    else
    {
        // when the "external" temperature is too cold - typically used for compressor low temp.
        // cutoffs when running, use hysteresis
        bool lock = false;
        if (isEngaged() && (heatSourceAmbientT_C < minT - hysteresis_dC))
        {
            lock = true;
        }
        // when not running, don't use hysteresis
        else if (!isEngaged() && (heatSourceAmbientT_C < minT))
        {
            lock = true;
        }

        // when the "external" temperature is too warm - typically used for resistance lockout
        // when running, use hysteresis
        if (isEngaged() && (heatSourceAmbientT_C > maxT + hysteresis_dC))
        {
            lock = true;
        }
        // when not running, don't use hysteresis
        else if (!isEngaged() && (heatSourceAmbientT_C > maxT))
        {
            lock = true;
        }

        if (maxedOut())
        {
            lock = true;
            send_warning(fmt::format("lock-out: condenser water temperature above max: {:0.2f}",
                                     maxSetpoint_C));
        }

        return lock;
    }
}

bool HPWH::HeatSource::shouldUnlock(double heatSourceAmbientT_C) const
{

    // if it's already unlocked, keep it unlocked
    if (!isLockedOut())
    {
        return true;
    }
    // if the heat source is capped and can't produce hotter water
    else if (maxedOut())
    {
        return false;
    }
    else
    {
        // when the "external" temperature is no longer too cold or too warm
        // when running, use hysteresis
        bool unlock = false;
        if (isEngaged() && (heatSourceAmbientT_C > minT + hysteresis_dC) &&
            (heatSourceAmbientT_C < maxT - hysteresis_dC))
        {
            unlock = true;
        }
        // when not running, don't use hysteresis
        else if (!isEngaged() && (heatSourceAmbientT_C > minT) && (heatSourceAmbientT_C < maxT))
        {
            unlock = true;
        }
        return unlock;
    }
}

bool HPWH::HeatSource::toLockOrUnlock(double heatSourceAmbientT_C)
{

    if (shouldLockOut(heatSourceAmbientT_C))
    {
        lockOutHeatSource();
    }
    if (shouldUnlock(heatSourceAmbientT_C))
    {
        unlockHeatSource();
    }

    return isLockedOut();
}

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
            if (turnOnLogicSet[i]->description == "standby" && standbyLogic != NULL)
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

bool HPWH::HeatSource::maxedOut() const
{
    bool maxed = false;

    // If the heat source can't produce water at the setpoint and the control logics are saying to
    // shut off
    if (hpwh->setpoint_C > maxSetpoint_C)
    {
        if (hpwh->tank->nodeTs_C[0] >= maxSetpoint_C || shutsOff())
        {
            maxed = true;
        }
    }
    return maxed;
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

double HPWH::HeatSource::heat(double cap_kJ)
{
    std::vector<double> heatDistribution;

    // calcHeatDist takes care of the swooping for wrapped configurations
    calcHeatDist(heatDistribution);

    // set the leftover capacity to 0
    double leftoverCap_kJ = 0.;
    for (int i = hpwh->getNumNodes() - 1; i >= 0; i--)
    {
        double nodeCap_kJ = cap_kJ * heatDistribution[i];
        if (nodeCap_kJ != 0.)
        {
            double heatToAdd_kJ = nodeCap_kJ + leftoverCap_kJ;
            // add leftoverCap to the next run, and keep passing it on
            leftoverCap_kJ = hpwh->addHeatAboveNode(heatToAdd_kJ, i, maxSetpoint_C);
        }
    }
    return leftoverCap_kJ;
}

double HPWH::HeatSource::getTankTemp() const
{

    std::vector<double> resampledTankTemps(getCondensitySize());
    resample(resampledTankTemps, hpwh->tank->nodeTs_C);

    double tankTemp_C = 0.;

    std::size_t j = 0;
    for (auto& resampledNodeTemp : resampledTankTemps)
    {
        tankTemp_C += condensity[j] * resampledNodeTemp;
        // Note that condensity is normalized.
        ++j;
    }

    return tankTemp_C;
}

void HPWH::HeatSource::calcHeatDist(std::vector<double>& heatDistribution)
{
    // Populate the vector of heat distribution
    heatDistribution.resize(hpwh->getNumNodes());
    resampleExtensive(heatDistribution, condensity);
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
