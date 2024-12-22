/*
 * Implementation of class HPWH::Condenser
 */

#include <algorithm>
#include <regex>

// vendor
#include <btwxt/btwxt.h>

#include "HPWH.hh"
#include "Condenser.hh"

HPWH::Condenser::Condenser(HPWH* hpwh_in,
                           const std::shared_ptr<Courier::Courier> courier,
                           const std::string& name_in)
    : HeatSource(hpwh_in, courier, name_in)
    , hysteresis_dC(0)
    , maxSetpoint_C(100.)
    , useBtwxtGrid(false)
    , extrapolationMethod(EXTRAP_LINEAR)
    , doDefrost(false)
    , maxOut_at_LowT {100, -273.15}
    , secondaryHeatExchanger {0., 0., 0.}
    , standbyPower_kW(0.)
    , configuration(COIL_CONFIG::CONFIG_WRAPPED)
    , isMultipass(true)
{
}

HPWH::Condenser& HPWH::Condenser::operator=(const HPWH::Condenser& cond_in)
{
    HPWH::HeatSource::operator=(cond_in);

    Tshrinkage_C = cond_in.Tshrinkage_C;
    lockedOut = cond_in.lockedOut;

    perfMap = cond_in.perfMap;

    perfGrid = cond_in.perfGrid;
    perfGridValues = cond_in.perfGridValues;
    perfRGI = cond_in.perfRGI;
    useBtwxtGrid = cond_in.useBtwxtGrid;

    defrostMap = cond_in.defrostMap;
    resDefrost = cond_in.resDefrost;

    configuration = cond_in.configuration;
    isMultipass = cond_in.isMultipass;
    mpFlowRate_LPS = cond_in.mpFlowRate_LPS;

    externalInletHeight = cond_in.externalInletHeight;
    externalOutletHeight = cond_in.externalOutletHeight;

    lowestNode = cond_in.lowestNode;
    extrapolationMethod = cond_in.extrapolationMethod;
    secondaryHeatExchanger = cond_in.secondaryHeatExchanger;

    hysteresis_dC = cond_in.hysteresis_dC;
    maxSetpoint_C = cond_in.maxSetpoint_C;

    return *this;
}

bool HPWH::Condenser::toLockOrUnlock(double heatSourceAmbientT_C)
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

bool HPWH::Condenser::maxedOut() const
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

bool HPWH::Condenser::shouldLockOut(double heatSourceAmbientT_C) const
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

bool HPWH::Condenser::shouldUnlock(double heatSourceAmbientT_C) const
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

void HPWH::Condenser::from(const std::unique_ptr<HeatSourceTemplate>& rshs_ptr)
{
    auto cond_ptr = reinterpret_cast<
        hpwh_data_model::rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE*>(
        rshs_ptr.get());

    auto& perf = cond_ptr->performance;
    switch (perf.coil_configuration)
    {
    case hpwh_data_model::rscondenserwaterheatsource_ns::CoilConfiguration::SUBMERGED:
    {
        configuration = COIL_CONFIG::CONFIG_SUBMERGED;
        break;
    }
    case hpwh_data_model::rscondenserwaterheatsource_ns::CoilConfiguration::WRAPPED:
    {
        configuration = COIL_CONFIG::CONFIG_WRAPPED;
        break;
    }
    case hpwh_data_model::rscondenserwaterheatsource_ns::CoilConfiguration::EXTERNAL:
    {
        configuration = COIL_CONFIG::CONFIG_EXTERNAL;
        break;
    }
    default:
    {
        break;
    }
    }

    checkFrom(maxSetpoint_C,
              perf.maximum_refrigerant_temperature_is_set,
              K_TO_C(perf.maximum_refrigerant_temperature),
              100.);
    checkFrom(maxT, perf.maximum_temperature_is_set, K_TO_C(perf.maximum_temperature), 100.);
    checkFrom(minT, perf.minimum_temperature_is_set, K_TO_C(perf.minimum_temperature), -273.15);
    checkFrom(hysteresis_dC,
              perf.compressor_lockout_temperature_hysteresis_is_set,
              perf.compressor_lockout_temperature_hysteresis,
              0.);

    // uses btwxt performance-grid interpolation
    if (perf.performance_map_is_set)
    {
        auto& perf_map = perf.performance_map;

        auto& grid_variables = perf_map.grid_variables;

        perfGrid.reserve(3);

        if(grid_variables.evaporator_environment_dry_bulb_temperature_is_set)
        {
            std::vector<double> evapTs_F = {};
            evapTs_F.reserve(grid_variables.evaporator_environment_dry_bulb_temperature.size());
            for (auto& T : grid_variables.evaporator_environment_dry_bulb_temperature)
                evapTs_F.push_back(C_TO_F(K_TO_C(T)));
            perfGrid.push_back(evapTs_F);
        }

        if(grid_variables.evaporator_environment_dry_bulb_temperature_is_set)
        {
            std::vector<double> heatSourceTs_F = {};
            heatSourceTs_F.reserve(grid_variables.heat_source_temperature.size());
            for (auto& T : grid_variables.heat_source_temperature)
                heatSourceTs_F.push_back(C_TO_F(K_TO_C(T)));
        }

        if(grid_variables.outlet_temperature_is_set)
        {
            std::vector<double> outletTs_F = {};
            outletTs_F.reserve(grid_variables.outlet_temperature.size());
            for (auto& T : grid_variables.outlet_temperature)
                outletTs_F.push_back(C_TO_F(K_TO_C(T)));
        }

        auto& lookup_variables = perf_map.lookup_variables;
        perfGridValues.reserve(2);

        std::size_t nVals = lookup_variables.input_power.size();
        std::vector<double> inputPowers_Btu_per_h(nVals), cops(nVals);
        for (std::size_t i = 0; i < nVals; ++i)
        {
            inputPowers_Btu_per_h[i] = W_TO_BTUperH(lookup_variables.input_power[i]);
            cops[i] = lookup_variables.heating_capacity[i] / lookup_variables.input_power[i];
        }

        perfGridValues.push_back(inputPowers_Btu_per_h);
        perfGridValues.push_back(cops);

        perfRGI = std::make_shared<Btwxt::RegularGridInterpolator>(
            Btwxt::RegularGridInterpolator(perfGrid, perfGridValues));
        useBtwxtGrid = true;

        perfRGI->set_axis_interpolation_method(0, Btwxt::InterpolationMethod::linear);
        perfRGI->set_axis_extrapolation_method(0, Btwxt::ExtrapolationMethod::linear);

        perfRGI->set_axis_extrapolation_method(1, Btwxt::ExtrapolationMethod::linear);
        perfRGI->set_axis_interpolation_method(1, Btwxt::InterpolationMethod::cubic);
    }

    if (perf.use_defrost_map_is_set && perf.use_defrost_map)
    {
        setupDefrostMap();
    }

    if (perf.standby_power_is_set)
    {
        standbyPower_kW = perf.standby_power / 1000.;
    }
}

void HPWH::Condenser::to(std::unique_ptr<HeatSourceTemplate>& rshs_ptr) const
{
    auto cond_ptr = reinterpret_cast<
        hpwh_data_model::rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE*>(
        rshs_ptr.get());

    auto& metadata = cond_ptr->metadata;
    checkTo(hpwh_data_model::ashrae205_ns::SchemaType::RSCONDENSERWATERHEATSOURCE,
            metadata.schema_is_set,
            metadata.schema);

    auto& perf = cond_ptr->performance;
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

    checkTo(hysteresis_dC,
            perf.compressor_lockout_temperature_hysteresis_is_set,
            perf.compressor_lockout_temperature_hysteresis);

    checkTo(C_TO_K(minT), perf.minimum_temperature_is_set, perf.minimum_temperature);
    checkTo(C_TO_K(maxT), perf.maximum_temperature_is_set, perf.maximum_temperature);
    checkTo(C_TO_K(maxSetpoint_C),
            perf.maximum_refrigerant_temperature_is_set,
            perf.maximum_refrigerant_temperature);

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

void HPWH::Condenser::addHeat(double externalT_C, double minutesToRun)
{
    double input_BTUperHr = 0., cap_BTUperHr = 0., cop = 0.;

    switch (configuration)
    {
    case CONFIG_SUBMERGED:
    case CONFIG_WRAPPED:
    {
        // calculate capacity btu/hr, input btu/hr, and cop
        hpwh->condenserInlet_C = getTankTemp();
        getCapacity(externalT_C, getTankTemp(), input_BTUperHr, cap_BTUperHr, cop);

        double cap_kJ = BTU_TO_KJ(cap_BTUperHr * minutesToRun / min_per_hr);

        double leftoverCap_kJ = heat(cap_kJ, maxSetpoint_C);

        // compute actual runtime
        runtime_min = (1. - (leftoverCap_kJ / cap_kJ)) * minutesToRun;

        if (runtime_min < -TOL_MINVALUE)
        {
            send_error(fmt::format("Negative runtime: {:g} min", runtime_min));
        }

        // outlet temperature is the condenser temperature after heat has been added
        hpwh->condenserOutlet_C = getTankTemp();
        break;
    }

    case CONFIG_EXTERNAL:
        // Else the heat source is external. SANCO2 system is only current example
        // capacity is calculated internal to this functio
        //  n, and cap/input_BTUperHr, cop are outputs
        if (isMultipass)
        {
            runtime_min =
                addHeatExternalMP(externalT_C, minutesToRun, cap_BTUperHr, input_BTUperHr, cop);
        }
        else
        {
            runtime_min =
                addHeatExternal(externalT_C, minutesToRun, cap_BTUperHr, input_BTUperHr, cop);
        }
        break;
    }

    // update the input & output energy
    energyInput_kWh += BTU_TO_KWH(input_BTUperHr * runtime_min / min_per_hr);
    energyOutput_kWh += BTU_TO_KWH(cap_BTUperHr * runtime_min / min_per_hr);
}

void HPWH::Condenser::getCapacity(double externalT_C,
                                  double condenserTemp_C,
                                  double setpointTemp_C,
                                  double& input_BTUperHr,
                                  double& cap_BTUperHr,
                                  double& cop)
{
    // Add an offset to the condenser temperature (or incoming coldwater temperature) to approximate
    // a secondary heat exchange in line with the compressor
    double condenserTemp_F =
        C_TO_F(condenserTemp_C + secondaryHeatExchanger.coldSideTemperatureOffest_dC);
    double externalT_F = C_TO_F(externalT_C);
    double outletT_F = C_TO_F(setpointTemp_C + secondaryHeatExchanger.hotSideTemperatureOffset_dC);

    if (useBtwxtGrid)
    {
        if (perfGrid.size() == 2)
        {
            std::vector<double> target {externalT_F, condenserTemp_F};
            btwxtInterp(input_BTUperHr, cop, target);
        }
        if (perfGrid.size() == 3)
        {
            std::vector<double> target {externalT_F, outletT_F, condenserTemp_F};
            btwxtInterp(input_BTUperHr, cop, target);
        }
    }
    else
    {
        if (perfMap.empty())
        { // Avoid using empty perfMap
            input_BTUperHr = 0.;
            cop = 0.;
        }
        else
        {
            getCapacityFromMap(externalT_C,
                               condenserTemp_C +
                                   secondaryHeatExchanger.coldSideTemperatureOffest_dC,
                               F_TO_C(outletT_F),
                               input_BTUperHr,
                               cop);
        }
    }

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, externalT_F);
    }

    cap_BTUperHr = cop * input_BTUperHr;

    // here is where the scaling for flow restriction happens
    // the input power doesn't change, we just scale the cop by a small percentage
    // that is based on the flow rate.  The equation is a fit to three points,
    // measured experimentally - 12 percent reduction at 150 cfm, 10 percent at
    // 200, and 0 at 375. Flow is expressed as fraction of full flow.
    if (airflowFreedom != 1)
    {
        double airflow = 375 * airflowFreedom;
        cop *= 0.00056 * airflow + 0.79;
    }
    if (cop < 0.)
    {
        send_warning("Warning: COP is Negative!");
    }
    if (cop < 1.)
    {
        send_warning("Warning: COP is Less than 1!");
    }
}

void HPWH::Condenser::getCapacityMP(double externalT_C,
                                    double condenserTemp_C,
                                    double& input_BTUperHr,
                                    double& cap_BTUperHr,
                                    double& cop)
{
    double externalT_F, condenserTemp_F;
    bool resDefrostHeatingOn = false;
    condenserTemp_F = C_TO_F(condenserTemp_C + secondaryHeatExchanger.coldSideTemperatureOffest_dC);
    externalT_F = C_TO_F(externalT_C);

    // Check if we have resistance elements to turn on for defrost and add the constant lift.
    if (resDefrost.inputPwr_kW > 0)
    {
        if (externalT_F < resDefrost.onBelowT_F)
        {
            externalT_F += resDefrost.constTempLift_dF;
            resDefrostHeatingOn = true;
        }
    }

    if (useBtwxtGrid)
    {
        std::vector<double> target {externalT_F, condenserTemp_F};
        btwxtInterp(input_BTUperHr, cop, target);
    }
    else
    {
        // Get bounding performance map points for interpolation/extrapolation
        if (externalT_F > perfMap[0].T_F)
        {
            if (extrapolationMethod == EXTRAP_NEAREST)
            {
                externalT_F = perfMap[0].T_F;
            }
        }

        // Const Tair Tin Tair2 Tin2 TairTin
        regressedMethodMP(
            input_BTUperHr, perfMap[0].inputPower_coeffs, externalT_F, condenserTemp_F);
        regressedMethodMP(cop, perfMap[0].COP_coeffs, externalT_F, condenserTemp_F);
    }
    input_BTUperHr = KWH_TO_BTU(input_BTUperHr);

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, externalT_F);
    }

    cap_BTUperHr = cop * input_BTUperHr;

    // For accounting add the resistance defrost to the input energy
    if (resDefrostHeatingOn)
    {
        input_BTUperHr += KW_TO_BTUperH(resDefrost.inputPwr_kW);
    }
}

void HPWH::Condenser::setupDefrostMap(double derate35 /*=0.8865*/)
{
    doDefrost = true;
    defrostMap.reserve(3);
    defrostMap.push_back({17., 1.});
    defrostMap.push_back({35., derate35});
    defrostMap.push_back({47., 1.});
}

void HPWH::Condenser::defrostDerate(double& to_derate, double airT_F) const
{
    if (airT_F <= defrostMap[0].T_F || airT_F >= defrostMap[defrostMap.size() - 1].T_F)
    {
        return; // Air temperature outside bounds of the defrost map. There is no extrapolation
                // here.
    }
    double derate_factor = 1.;
    size_t i_prev = 0;
    for (size_t i = 1; i < defrostMap.size(); ++i)
    {
        if (airT_F <= defrostMap[i].T_F)
        {
            i_prev = i - 1;
            break;
        }
    }
    linearInterp(derate_factor,
                 airT_F,
                 defrostMap[i_prev].T_F,
                 defrostMap[i_prev + 1].T_F,
                 defrostMap[i_prev].derate_fraction,
                 defrostMap[i_prev + 1].derate_fraction);
    to_derate *= derate_factor;
}

void HPWH::Condenser::calcHeatDist(std::vector<double>& heatDistribution)
{
    // Populate the vector of heat distribution
    if (configuration == CONFIG_SUBMERGED)
    {
        HPWH::HeatSource::calcHeatDist(heatDistribution);
    }
    else if (configuration == CONFIG_WRAPPED)
    { // Wrapped around the tank, send through the logistic function
        std::vector<double> tankTs_C;
        hpwh->getTankTemps(tankTs_C);
        calcThermalDist(heatDistribution, Tshrinkage_C, lowestNode, tankTs_C, hpwh->setpoint_C);
    }
}

//-----------------------------------------------------------------------------
///	@brief	Add external heat for a single-pass configuration.
/// @param[in]	externalT_C	        external temperature
///	@param[in]	stepTime_min		heating time available
///	@param[out]	cap_BTUperHr		heating capacity delivered
///	@param[out]	input_BTUperHr		input power drawn
/// @param[out]	cop		            average cop
/// @return	elapsed time (min)
//-----------------------------------------------------------------------------
double HPWH::Condenser::addHeatExternal(double externalT_C,
                                        double stepTime_min,
                                        double& cap_BTUperHr,
                                        double& input_BTUperHr,
                                        double& cop)
{
    input_BTUperHr = 0.;
    cap_BTUperHr = 0.;
    cop = 0.;

    double setpointT_C = std::min(maxSetpoint_C, hpwh->setpoint_C);
    double remainingTime_min = stepTime_min;
    do
    {
        double tempInput_BTUperHr = 0., tempCap_BTUperHr = 0., temp_cop = 0.;
        double& externalOutletT_C = hpwh->tank->nodeTs_C[externalOutletHeight];

        // how much heat is available in remaining time
        getCapacity(externalT_C, externalOutletT_C, tempInput_BTUperHr, tempCap_BTUperHr, temp_cop);

        double heatingPower_kW = BTUperH_TO_KW(tempCap_BTUperHr);

        double targetT_C = setpointT_C;

        // temperature increase
        double deltaT_C = targetT_C - externalOutletT_C;

        if (deltaT_C <= 0.)
        {
            break;
        }

        // maximum heat that can be added in remaining time
        double heatingCapacity_kJ = heatingPower_kW * (remainingTime_min * sec_per_min);

        // heat for outlet node to reach target temperature
        double nodeHeat_kJ = hpwh->tank->nodeCp_kJperC * deltaT_C;

        // assume one node will be heated this pass
        double neededHeat_kJ = nodeHeat_kJ;

        // reduce node fraction to heat if limited by capacity
        double nodeFrac = 1.;
        if (heatingCapacity_kJ < nodeHeat_kJ)
        {
            nodeFrac = heatingCapacity_kJ / nodeHeat_kJ;
            neededHeat_kJ = heatingCapacity_kJ;
        }

        // limit node fraction to heat by comparison criterion
        double fractToShutOff = fractToMeetComparisonExternal();
        if (fractToShutOff < nodeFrac)
        {
            nodeFrac = fractToShutOff;
            neededHeat_kJ = nodeFrac * nodeHeat_kJ;
        }

        // heat for less than remaining time if full capacity will not be used
        double heatingTime_min = remainingTime_min;
        if (heatingCapacity_kJ > neededHeat_kJ)
        {
            heatingTime_min *= neededHeat_kJ / heatingCapacity_kJ;
            remainingTime_min -= heatingTime_min;
        }
        else
        {
            remainingTime_min = 0.;
        }

        // track the condenser temperatures before mixing nodes
        if (isACompressor())
        {
            hpwh->condenserInlet_C += externalOutletT_C * heatingTime_min;
            hpwh->condenserOutlet_C += targetT_C * heatingTime_min;
        }

        // mix with node above from outlet to inlet
        // mix inlet water at target temperature with inlet node
        for (std::size_t nodeIndex = externalOutletHeight;
             static_cast<int>(nodeIndex) <= externalInletHeight;
             ++nodeIndex)
        {
            double& mixT_C = (static_cast<int>(nodeIndex) == externalInletHeight)
                                 ? targetT_C
                                 : hpwh->tank->nodeTs_C[nodeIndex + 1];
            hpwh->tank->nodeTs_C[nodeIndex] =
                (1. - nodeFrac) * hpwh->tank->nodeTs_C[nodeIndex] + nodeFrac * mixT_C;
        }

        hpwh->mixTankInversions();
        hpwh->updateSoCIfNecessary();

        // track outputs weighted by the time run
        // pump power added to approximate a secondary heat exchange in line with the compressor
        input_BTUperHr +=
            (tempInput_BTUperHr + W_TO_BTUperH(secondaryHeatExchanger.extraPumpPower_W)) *
            heatingTime_min;
        cap_BTUperHr += tempCap_BTUperHr * heatingTime_min;
        cop += temp_cop * heatingTime_min;

        hpwh->externalVolumeHeated_L += nodeFrac * hpwh->tank->nodeVolume_L;

        // if there's still time remaining and you haven't heated to the cutoff
        // specified in shutsOff logic, keep heating
    } while ((remainingTime_min > 0.) && (!shutsOff()));

    // divide outputs by sum of weight - the total time ran
    // not remainingTime_min == minutesToRun is possible
    //   must prevent divide by 0 (added 4-11-2023)
    double runTime_min = stepTime_min - remainingTime_min;
    if (runTime_min > 0.)
    {
        input_BTUperHr /= runTime_min;
        cap_BTUperHr /= runTime_min;
        cop /= runTime_min;
        hpwh->condenserInlet_C /= runTime_min;
        hpwh->condenserOutlet_C /= runTime_min;
    }

    // return the time left
    return runTime_min;
}

//-----------------------------------------------------------------------------
///	@brief	Add external heat for a multipass configuration.
/// @param[in]	externalT_C	        external temperature
///	@param[in]	stepTime_min		heating time available
///	@param[out]	cap_BTUperHr		heating capacity delivered
///	@param[out]	input_BTUperHr		input power drawn
/// @param[out]	cop		            average cop
/// @return	elapsed time (min)
//-----------------------------------------------------------------------------
double HPWH::Condenser::addHeatExternalMP(double externalT_C,
                                          double stepTime_min,
                                          double& cap_BTUperHr,
                                          double& input_BTUperHr,
                                          double& cop)
{
    input_BTUperHr = 0.;
    cap_BTUperHr = 0.;
    cop = 0.;

    double remainingTime_min = stepTime_min;
    do
    {
        // find node fraction to heat in remaining time
        double nodeFrac =
            mpFlowRate_LPS * (remainingTime_min * sec_per_min) / hpwh->tank->nodeVolume_L;
        if (nodeFrac > 1.)
        { // heat no more than one node each pass
            nodeFrac = 1.;
        }

        // mix the tank
        // apply nodeFrac as the degree of mixing (formerly 1.0)
        hpwh->mixTankNodes(0, hpwh->getNumNodes(), nodeFrac);

        double tempInput_BTUperHr = 0., tempCap_BTUperHr = 0., temp_cop = 0.;
        double& externalOutletT_C = hpwh->tank->nodeTs_C[externalOutletHeight];

        // find heating capacity
        getCapacityMP(
            externalT_C, externalOutletT_C, tempInput_BTUperHr, tempCap_BTUperHr, temp_cop);

        double heatingPower_kW = BTUperH_TO_KW(tempCap_BTUperHr);

        // temperature increase at this power and flow rate
        double deltaT_C =
            heatingPower_kW / (mpFlowRate_LPS * CPWATER_kJperkgC * DENSITYWATER_kgperL);

        // find target temperature
        double targetT_C = externalOutletT_C + deltaT_C;

        // maximum heat that can be added in remaining time
        double heatingCapacity_kJ = heatingPower_kW * (remainingTime_min * sec_per_min);

        // heat needed to raise temperature of one node by deltaT_C
        double nodeHeat_kJ = hpwh->tank->nodeCp_kJperC * deltaT_C;

        // heat no more than one node this step
        double heatingTime_min = remainingTime_min;
        if (heatingCapacity_kJ > nodeHeat_kJ)
        {
            heatingTime_min *= nodeHeat_kJ / heatingCapacity_kJ;
            remainingTime_min -= heatingTime_min;
        }
        else
        {
            remainingTime_min = 0.;
        }

        // track the condenser temperatures before mixing nodes
        if (isACompressor())
        {
            hpwh->condenserInlet_C += externalOutletT_C * heatingTime_min;
            hpwh->condenserOutlet_C += targetT_C * heatingTime_min;
        }

        // mix with node above from outlet to inlet
        // mix inlet water at target temperature with inlet node
        for (std::size_t nodeIndex = externalOutletHeight;
             static_cast<int>(nodeIndex) <= externalInletHeight;
             ++nodeIndex)
        {
            double& mixT_C = (static_cast<int>(nodeIndex) == externalInletHeight)
                                 ? targetT_C
                                 : hpwh->tank->nodeTs_C[nodeIndex + 1];
            hpwh->tank->nodeTs_C[nodeIndex] =
                (1. - nodeFrac) * hpwh->tank->nodeTs_C[nodeIndex] + nodeFrac * mixT_C;
        }

        hpwh->mixTankInversions();
        hpwh->updateSoCIfNecessary();

        // track outputs weighted by the time run
        // pump power added to approximate a secondary heat exchange in line with the compressor
        input_BTUperHr +=
            (tempInput_BTUperHr + W_TO_BTUperH(secondaryHeatExchanger.extraPumpPower_W)) *
            heatingTime_min;
        cap_BTUperHr += tempCap_BTUperHr * heatingTime_min;
        cop += temp_cop * heatingTime_min;

        hpwh->externalVolumeHeated_L += nodeFrac * hpwh->tank->nodeVolume_L;

        // continue until time expired or cutoff condition met
    } while ((remainingTime_min > 0.) && (!shutsOff()));

    // time elapsed in this function
    double elapsedTime_min = stepTime_min - remainingTime_min;
    if (elapsedTime_min > 0.)
    {
        input_BTUperHr /= elapsedTime_min;
        cap_BTUperHr /= elapsedTime_min;
        cop /= elapsedTime_min;
        hpwh->condenserInlet_C /= elapsedTime_min;
        hpwh->condenserOutlet_C /= elapsedTime_min;
    }

    // return the elapsed time
    return elapsedTime_min;
}

void HPWH::Condenser::btwxtInterp(double& input_BTUperHr, double& cop, std::vector<double>& target)
{
    std::vector<double> result = perfRGI->get_values_at_target(target);
    input_BTUperHr = result[0];
    cop = result[1];
}

bool HPWH::Condenser::isExternalMultipass() const
{
    return isMultipass && (configuration == Condenser::CONFIG_EXTERNAL);
}

void HPWH::Condenser::sortPerformanceMap()
{
    std::sort(perfMap.begin(),
              perfMap.end(),
              [](const PerfPoint& a, const PerfPoint& b) -> bool { return a.T_F < b.T_F; });
}

/*static*/
void HPWH::Condenser::linearInterp(
    double& ynew, double xnew, double x0, double x1, double y0, double y1)
{
    ynew = y0 + (xnew - x0) * (y1 - y0) / (x1 - x0);
}

/*static*/
void HPWH::Condenser::regressedMethod(
    double& ynew, const std::vector<double>& coefficents, double x1, double x2, double x3)
{
    ynew = coefficents[0] + coefficents[1] * x1 + coefficents[2] * x2 + coefficents[3] * x3 +
           coefficents[4] * x1 * x1 + coefficents[5] * x2 * x2 + coefficents[6] * x3 * x3 +
           coefficents[7] * x1 * x2 + coefficents[8] * x1 * x3 + coefficents[9] * x2 * x3 +
           coefficents[10] * x1 * x2 * x3;
}

/*static*/
void HPWH::Condenser::regressedMethodMP(double& ynew,
                                        const std::vector<double>& coefficents,
                                        double x1,
                                        double x2)
{
    // Const Tair Tin Tair2 Tin2 TairTin
    ynew = coefficents[0] + coefficents[1] * x1 + coefficents[2] * x2 + coefficents[3] * x1 * x1 +
           coefficents[4] * x2 * x2 + coefficents[5] * x1 * x2;
}

void HPWH::Condenser::getCapacityFromMap(double environmentT_C,
                                         double heatSourceT_C,
                                         double outletT_C,
                                         double& input_BTUperHr,
                                         double& cop) const
{
    double environmentT_F = C_TO_F(environmentT_C);
    double heatSourceT_F = C_TO_F(heatSourceT_C);
    double outletT_F = C_TO_F(outletT_C);
    input_BTUperHr = 0.;
    cop = 0.;

    bool resDefrostHeatingOn = false;

    // Check if we have resistance elements to turn on for defrost and add the constant lift.
    if (resDefrost.inputPwr_kW > 0)
    {
        if (environmentT_F < resDefrost.onBelowT_F)
        {
            environmentT_F += resDefrost.constTempLift_dF;
            resDefrostHeatingOn = true;
        }
    }

    if (perfMap.size() == 1)
    {
        if ((environmentT_F > perfMap[0].T_F) && (extrapolationMethod == EXTRAP_NEAREST))
        {
            environmentT_F = perfMap[0].T_F;
        }

        if (isMultipass)
        {
            regressedMethodMP(
                input_BTUperHr, perfMap[0].inputPower_coeffs, environmentT_F, heatSourceT_F);
            input_BTUperHr = KWH_TO_BTU(input_BTUperHr);

            regressedMethodMP(cop, perfMap[0].COP_coeffs, environmentT_F, heatSourceT_F);
        }
        else
        {
            regressedMethod(input_BTUperHr,
                            perfMap[0].inputPower_coeffs,
                            environmentT_F,
                            outletT_F,
                            heatSourceT_F);
            input_BTUperHr = KWH_TO_BTU(input_BTUperHr);

            regressedMethod(cop, perfMap[0].COP_coeffs, environmentT_F, outletT_F, heatSourceT_F);
        }
    }
    else if (perfMap.size() > 1)
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

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, environmentT_F);
    }

    // For accounting add the resistance defrost to the input energy
    if (resDefrostHeatingOn)
    {
        input_BTUperHr += KW_TO_BTUperH(resDefrost.inputPwr_kW);
    }
}

void HPWH::Condenser::getCapacityFromMap(double environmentT_C,
                                         double heatSourceT_C,
                                         double& input_BTUperHr,
                                         double& cop) const
{
    return getCapacityFromMap(
        environmentT_C, heatSourceT_C, hpwh->getSetpoint(), input_BTUperHr, cop);
}

// convert to grid
void HPWH::Condenser::convertMapToGrid(std::vector<std::vector<double>>& tempGrid,
                                       std::vector<std::vector<double>>& tempGridValues) const
{
    std::size_t nEnvTempsOrig = perfMap.size();
    if (nEnvTempsOrig < 1)
        return;

    tempGridValues.reserve(2); // inputPower, cop

    std::vector<double> envTemps_K = {};
    std::vector<double> heatSourceTemps_K = {};
    std::vector<double> outletTemps_K = {};
    if (nEnvTempsOrig == 1) // uses regression or regressionMP methods
    {
        auto envTempRange_dC = maxT - minT;
        auto nEnvTs = static_cast<std::size_t>(10 * envTempRange_dC / 100.);
        auto dEnvT_dC = static_cast<double>(envTempRange_dC / nEnvTs);
        envTemps_K.resize(nEnvTs);
        for (std::size_t i = 0; i < nEnvTs; ++i)
        {
            envTemps_K[i] = C_TO_K(minT + static_cast<double>(i) / nEnvTs * dEnvT_dC);
        }

        const double minHeatSourceTemp_C = 0.;
        const double maxHeatSourceTemp_C = maxSetpoint_C;
        auto heatSourceTempRange_dC = maxHeatSourceTemp_C - minHeatSourceTemp_C;
        auto nHeatSourceTs = std::size_t(5 * heatSourceTempRange_dC / 100.);
        auto dHeatSourceT_dC = heatSourceTempRange_dC / static_cast<double>(nHeatSourceTs);
        heatSourceTemps_K.resize(nHeatSourceTs);
        for (std::size_t i = 0; i < nHeatSourceTs; ++i)
        {
            heatSourceTemps_K[i] = C_TO_K(minHeatSourceTemp_C +
                                          static_cast<double>(i) / nHeatSourceTs * dHeatSourceT_dC);
        }

        if (isMultipass)
        {
            tempGrid.reserve(2);
            tempGrid.push_back(envTemps_K);
            tempGrid.push_back(heatSourceTemps_K);
            std::size_t nTotVals = envTemps_K.size() * heatSourceTemps_K.size();
            std::vector<double> inputPowers_W(nTotVals), heatingCapacities_W(nTotVals);
            std::size_t i = 0;
            double input_BTUperHr, cop;
            for (auto& envTemp_K : envTemps_K)
                for (auto& heatSourceTemp_K : heatSourceTemps_K)
                    if (outletTemps_K.empty())
                    {
                        getCapacityFromMap(
                            K_TO_C(envTemp_K), K_TO_C(heatSourceTemp_K), input_BTUperHr, cop);
                        inputPowers_W[i] = 1000. * BTUperH_TO_KW(input_BTUperHr);
                        heatingCapacities_W[i] = cop * inputPowers_W[i];
                        ++i;
                    }

            tempGridValues.push_back(inputPowers_W);
            tempGridValues.push_back(heatingCapacities_W);
        }
        else
        {
            outletTemps_K = heatSourceTemps_K;
            tempGrid.reserve(3);
            tempGrid.push_back(envTemps_K);
            tempGrid.push_back(heatSourceTemps_K);
            tempGrid.push_back(outletTemps_K);
            std::size_t nTotVals =
                envTemps_K.size() * heatSourceTemps_K.size() * outletTemps_K.size();
            std::vector<double> inputPowers_W(nTotVals), heatingCapacities_W(nTotVals);
            std::size_t i = 0;
            double input_BTUperHr, cop;
            for (auto& envTemp_K : envTemps_K)
                for (auto& heatSourceTemp_K : heatSourceTemps_K)
                    for (auto& outletTemp_K : outletTemps_K)
                    {
                        getCapacityFromMap(K_TO_C(envTemp_K),
                                           K_TO_C(outletTemp_K),
                                           K_TO_C(heatSourceTemp_K),
                                           input_BTUperHr,
                                           cop);
                        inputPowers_W[i] = 1000. * BTUperH_TO_KW(input_BTUperHr);
                        heatingCapacities_W[i] = cop * inputPowers_W[i];
                        ++i;
                    }

            tempGridValues.push_back(inputPowers_W);
            tempGridValues.push_back(heatingCapacities_W);
        }
    }
    else
    {
        double maxPowerCurvature = 0.; // curvature used to determine # of points
        double maxCOPCurvature = 0.;
        envTemps_K.reserve(nEnvTempsOrig + 2);
        envTemps_K.push_back(C_TO_K(minT));
        for (auto& perfPoint : perfMap)
        {
            if ((F_TO_C(perfPoint.T_F) > minT) && (F_TO_C(perfPoint.T_F) < maxT))
            {
                envTemps_K.push_back(C_TO_K(F_TO_C(perfPoint.T_F)));
                double magPowerCurvature = abs(perfPoint.inputPower_coeffs[2]);
                double magCOPCurvature = abs(perfPoint.COP_coeffs[2]);
                maxPowerCurvature =
                    magPowerCurvature > maxPowerCurvature ? magPowerCurvature : maxPowerCurvature;
                maxCOPCurvature =
                    magCOPCurvature > maxCOPCurvature ? magCOPCurvature : maxCOPCurvature;
            }
        }
        envTemps_K.push_back(C_TO_K(maxT));

        // relate to reference values (from AOSmithPHPT60)
        const double minHeatSourceTemp_C = 0.;
        const double maxHeatSourceTemp_C = maxSetpoint_C;
        const double heatSourceTempRange_dC = maxHeatSourceTemp_C - minHeatSourceTemp_C;
        const double heatSourceTempRangeRef_dC = 100. - 0.;
        const double rangeFac = heatSourceTempRange_dC / heatSourceTempRangeRef_dC;

        constexpr std::size_t minVals = 2;
        constexpr std::size_t refPowerVals = 11;
        constexpr std::size_t refCOP_vals = 11;
        constexpr double refPowerCurvature = 0.0176;
        constexpr double refCOP_curvature = 0.0002;

        auto nPowerVals = static_cast<std::size_t>(
            rangeFac * (maxPowerCurvature / refPowerCurvature) * (refPowerVals - minVals) +
            minVals);
        auto nCOP_vals = static_cast<std::size_t>(
            rangeFac * (maxCOPCurvature / refCOP_curvature) * (refCOP_vals - minVals) + minVals);
        std::size_t nVals = std::max(nPowerVals, nCOP_vals);
        heatSourceTemps_K.resize(nVals);
        {
            double T_K = C_TO_K(0.);
            double dT_K = heatSourceTempRange_dC / static_cast<double>(nVals - 1);
            for (auto& heatSourceTemp_K : heatSourceTemps_K)
            {
                heatSourceTemp_K = T_K;
                T_K += dT_K;
            }
        }
        // fill grid
        tempGrid.push_back(envTemps_K);
        tempGrid.push_back(heatSourceTemps_K);
        std::size_t nTotVals = envTemps_K.size() * heatSourceTemps_K.size();
        std::vector<double> inputPowers_W(nTotVals), heatingCapacities_W(nTotVals);
        std::size_t i = 0;
        double input_BTUperHr, cop;
        for (auto& envTemp_K : envTemps_K)
            for (auto& heatSourceTemp_K : heatSourceTemps_K)
            {
                if (outletTemps_K.empty())
                {
                    getCapacityFromMap(
                        K_TO_C(envTemp_K), K_TO_C(heatSourceTemp_K), input_BTUperHr, cop);
                    inputPowers_W[i] = 1000. * BTUperH_TO_KW(input_BTUperHr);
                    heatingCapacities_W[i] = cop * inputPowers_W[i];
                    ++i;
                }
            }

        tempGridValues.push_back(inputPowers_W);
        tempGridValues.push_back(heatingCapacities_W);
    }
}
