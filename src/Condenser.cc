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

    return *this;
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
        perfGrid.reserve(2);

        std::vector<double> evapTs_F = {};
        evapTs_F.reserve(grid_variables.evaporator_environment_dry_bulb_temperature.size());
        for (auto& T : grid_variables.evaporator_environment_dry_bulb_temperature)
            evapTs_F.push_back(C_TO_F(K_TO_C(T)));

        std::vector<double> heatSourceTs_F = {};
        evapTs_F.reserve(grid_variables.heat_source_temperature.size());
        for (auto& T : grid_variables.heat_source_temperature)
            heatSourceTs_F.push_back(C_TO_F(K_TO_C(T)));

        perfGrid.push_back(evapTs_F);
        perfGrid.push_back(heatSourceTs_F);

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

        convertMapToGrid(perfMap, tempGrid, tempGridValues);

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

        double leftoverCap_kJ = heat(cap_kJ);

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
    double externalT_F, condenserTemp_F;

    // Add an offset to the condenser temperature (or incoming coldwater temperature) to approximate
    // a secondary heat exchange in line with the compressor
    condenserTemp_F = C_TO_F(condenserTemp_C + secondaryHeatExchanger.coldSideTemperatureOffest_dC);
    externalT_F = C_TO_F(externalT_C);

    double Tout_F = C_TO_F(setpointTemp_C + secondaryHeatExchanger.hotSideTemperatureOffset_dC);

    if (useBtwxtGrid)
    {
        if (perfGrid.size() == 2)
        {
            std::vector<double> target {externalT_F, condenserTemp_F};
            btwxtInterp(input_BTUperHr, cop, target);
        }
        if (perfGrid.size() == 3)
        {
            std::vector<double> target {externalT_F, Tout_F, condenserTemp_F};
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
        else if (perfMap.size() > 1)
        {
            getCapacityFromMap(perfMap,
                               externalT_C,
                               condenserTemp_C +
                                   secondaryHeatExchanger.coldSideTemperatureOffest_dC,
                               input_BTUperHr,
                               cop);
        }
        else
        { // perfMap.size() == 1 or we've got an issue.
            if (externalT_F > perfMap[0].T_F)
            {
                if (extrapolationMethod == EXTRAP_NEAREST)
                {
                    externalT_F = perfMap[0].T_F;
                }
            }

            regressedMethod(
                input_BTUperHr, perfMap[0].inputPower_coeffs, externalT_F, Tout_F, condenserTemp_F);
            input_BTUperHr = KWH_TO_BTU(input_BTUperHr);

            regressedMethod(cop, perfMap[0].COP_coeffs, externalT_F, Tout_F, condenserTemp_F);
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

void HPWH::Condenser::defrostDerate(double& to_derate, double airT_F)
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
    double& ynew, std::vector<double>& coefficents, double x1, double x2, double x3)
{
    ynew = coefficents[0] + coefficents[1] * x1 + coefficents[2] * x2 + coefficents[3] * x3 +
           coefficents[4] * x1 * x1 + coefficents[5] * x2 * x2 + coefficents[6] * x3 * x3 +
           coefficents[7] * x1 * x2 + coefficents[8] * x1 * x3 + coefficents[9] * x2 * x3 +
           coefficents[10] * x1 * x2 * x3;
}

/*static*/
void HPWH::Condenser::regressedMethodMP(double& ynew,
                                        std::vector<double>& coefficents,
                                        double x1,
                                        double x2)
{
    // Const Tair Tin Tair2 Tin2 TairTin
    ynew = coefficents[0] + coefficents[1] * x1 + coefficents[2] * x2 + coefficents[3] * x1 * x1 +
           coefficents[4] * x2 * x2 + coefficents[5] * x1 * x2;
}

/*static*/
void HPWH::Condenser::getCapacityFromMap(const std::vector<HPWH::Condenser::PerfPoint>& perfMap,
                                         double environmentT_C,
                                         double heatSourceT_C,
                                         double& input_BTUperHr,
                                         double& cop)
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
/*static*/
void HPWH::Condenser::convertMapToGrid(const std::vector<HPWH::Condenser::PerfPoint>& perfMap,
                                       std::vector<std::vector<double>>& tempGrid,
                                       std::vector<std::vector<double>>& tempGridValues)
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

    std::vector<double> inputPowers_W = {};
    inputPowers_W.reserve(nVals);

    std::vector<double> cops = {};
    cops.reserve(nVals);

    double input_BTUperHr, cop;
    for (auto& envTemp_K : envTemps_K)
        for (auto& heatSourceTemp_K : heatSourceTemps_K)
        {
            getCapacityFromMap(
                perfMap, K_TO_C(envTemp_K), K_TO_C(heatSourceTemp_K), input_BTUperHr, cop);
            inputPowers_W.push_back(1000. * BTUperH_TO_KW(input_BTUperHr));
            cops.push_back(cop);
        }

    tempGridValues.push_back(inputPowers_W);
    tempGridValues.push_back(cops);
}