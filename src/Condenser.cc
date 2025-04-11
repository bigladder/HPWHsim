/*
 * Implementation of class HPWH::Condenser
 */

#include <algorithm>
#include <regex>

// vendor
#include <btwxt/btwxt.h>

#include "HPWH.hh"
#include "HPWHUtils.hh"
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

    j_metadata = cond_in..j_metadata;
    j_productInformation = cond_in.j_productInformation;
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

void arrangeGridVector(std::vector<double>& V)
{
    std::sort(V.begin(), V.end(), [](double a, double b) { return a < b; });

    auto copyV = V;
    V.clear();
    bool first = true;
    double x_prev = 0.;
    for (auto& x : copyV) // skip duplicates
    {
        if (first || (x > x_prev))
            V.push_back(x);
        first = false;
        x_prev = x;
    }
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

void HPWH::Condenser::makeBtwxt()
{
    auto is_integrated = (configuration != CONFIG_EXTERNAL);
    auto is_Mitsubishi = (hpwh->model == MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP);
    auto is_NyleMP =
        ((MODELS_NyleC60A_MP <= hpwh->model) && (hpwh->model <= MODELS_NyleC250A_C_MP));

    std::vector<Btwxt::GridAxis> grid_axes = {};
    std::size_t iAxis = 0;
    {
        auto interpMethod = (is_Mitsubishi || is_NyleMP || is_integrated)
                                ? Btwxt::InterpolationMethod::linear
                                : Btwxt::InterpolationMethod::cubic;
        auto extrapMethod = (is_Mitsubishi || is_NyleMP) ? Btwxt::ExtrapolationMethod::constant
                                                         : Btwxt::ExtrapolationMethod::linear;
        grid_axes.push_back(Btwxt::GridAxis(perfGrid[iAxis],
                                            interpMethod,
                                            extrapMethod,
                                            {-DBL_MAX, DBL_MAX},
                                            "TAir",
                                            get_courier()));
        ++iAxis;
    }

    if (perfGrid.size() > 2)
    {
        auto interpMethod = (is_Mitsubishi) ? Btwxt::InterpolationMethod::linear
                                            : Btwxt::InterpolationMethod::linear;
        auto extrapMethod = (is_Mitsubishi) ? Btwxt::ExtrapolationMethod::constant
                                            : Btwxt::ExtrapolationMethod::linear;
        grid_axes.push_back(Btwxt::GridAxis(perfGrid[iAxis],
                                            interpMethod,
                                            extrapMethod,
                                            {-DBL_MAX, DBL_MAX},
                                            "TOut",
                                            get_courier()));
        ++iAxis;
    }

    {
        auto interpMethod = (is_Mitsubishi || is_NyleMP) ? Btwxt::InterpolationMethod::linear
                                                         : Btwxt::InterpolationMethod::cubic;
        auto extrapMethod = (is_Mitsubishi) ? Btwxt::ExtrapolationMethod::linear
                                            : ((is_NyleMP) ? Btwxt::ExtrapolationMethod::constant
                                                           : Btwxt::ExtrapolationMethod::linear);

        grid_axes.push_back(Btwxt::GridAxis(perfGrid[iAxis],
                                            interpMethod,
                                            extrapMethod,
                                            {-DBL_MAX, DBL_MAX},
                                            "Tin",
                                            get_courier()));
        ++iAxis;
    }

    perfRGI = std::make_shared<Btwxt::RegularGridInterpolator>(Btwxt::RegularGridInterpolator(
        grid_axes, perfGridValues, "RegularGridInterpolator", get_courier()));

    useBtwxtGrid = true;
}

void HPWH::Condenser::from(
    const std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& hs)
{
    if (typeid(hs.get()) ==
        typeid(hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE))
    {
        auto hsp = reinterpret_cast<
            hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE*>(hs.get());
        return from(*hsp);
    }
    else if (typeid(hs.get()) ==
             typeid(hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP))
    {
        auto hsp = reinterpret_cast<hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP*>(
            hs.get());
        return from(*hsp);
    }
}

void HPWH::Condenser::from(
    const hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE& rshs)
{
    metadata_to_json(metadata, rshs);
    productInformation_to_json(rshs, productInformation);
    auto& perf = rshs.performance;
    switch (perf.coil_configuration)
    {
    case hpwh_data_model::rscondenserwaterheatsource::CoilConfiguration::SUBMERGED:
    {
        configuration = COIL_CONFIG::CONFIG_SUBMERGED;
        break;
    }
    case hpwh_data_model::rscondenserwaterheatsource::CoilConfiguration::WRAPPED:
    {
        configuration = COIL_CONFIG::CONFIG_WRAPPED;
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
        // order based on MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP
        if (grid_variables.evaporator_environment_dry_bulb_temperature_is_set)
        {
            std::vector<double> evapTs_F = {};
            evapTs_F.reserve(grid_variables.evaporator_environment_dry_bulb_temperature.size());
            for (auto& T : grid_variables.evaporator_environment_dry_bulb_temperature)
                evapTs_F.push_back(C_TO_F(K_TO_C(T)));
            perfGrid.push_back(evapTs_F);
            maxT = F_TO_C(evapTs_F.back());
            minT = F_TO_C(evapTs_F.front());
        }

        if (grid_variables.heat_source_temperature_is_set)
        {
            std::vector<double> heatSourceTs_F = {};
            heatSourceTs_F.reserve(grid_variables.heat_source_temperature.size());
            for (auto& T : grid_variables.heat_source_temperature)
                heatSourceTs_F.push_back(C_TO_F(K_TO_C(T)));
            perfGrid.push_back(heatSourceTs_F);
        }

        auto& lookup_variables = perf_map.lookup_variables;
        perfGridValues.reserve(2);

        std::size_t nVals = lookup_variables.input_power.size();
        std::vector<double> inputPowers_kW(nVals), cops(nVals);
        for (std::size_t i = 0; i < nVals; ++i)
        {
            inputPowers_kW[i] = lookup_variables.input_power[i] / 1000.;
            cops[i] = lookup_variables.heating_capacity[i] / lookup_variables.input_power[i];
        }

        std::vector<double> inputPowers_Btu_per_h(nVals);
        for (std::size_t i = 0; i < nVals; ++i)
            inputPowers_Btu_per_h[i] = KW_TO_BTUperH(inputPowers_kW[i]);
        perfGridValues.push_back(inputPowers_Btu_per_h);
        perfGridValues.push_back(cops);

        makeBtwxt();
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

void HPWH::Condenser::from(const hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP& rshs)
{
    productInformation_to_json(rshs, productInformation);

    configuration = COIL_CONFIG::CONFIG_EXTERNAL;

    auto& perf = rshs.performance;

    checkFrom(maxSetpoint_C,
              perf.maximum_refrigerant_temperature_is_set,
              K_TO_C(perf.maximum_refrigerant_temperature),
              100.);
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
        // order based on MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP
        if (grid_variables.evaporator_environment_dry_bulb_temperature_is_set)
        {
            std::vector<double> evapTs_F = {};
            evapTs_F.reserve(grid_variables.evaporator_environment_dry_bulb_temperature.size());
            for (auto& T : grid_variables.evaporator_environment_dry_bulb_temperature)
                evapTs_F.push_back(C_TO_F(K_TO_C(T)));
            perfGrid.push_back(evapTs_F);
            maxT = F_TO_C(evapTs_F.back());
            minT = F_TO_C(evapTs_F.front());
        }

        if (grid_variables.condenser_leaving_temperature_is_set)
        {
            std::vector<double> outletTs_F = {};
            outletTs_F.reserve(grid_variables.condenser_leaving_temperature.size());
            for (auto& T : grid_variables.condenser_leaving_temperature)
                outletTs_F.push_back(C_TO_F(K_TO_C(T)));
            perfGrid.push_back(outletTs_F);
        }

        if (grid_variables.condenser_entering_temperature_is_set)
        {
            std::vector<double> heatSourceTs_F = {};
            heatSourceTs_F.reserve(grid_variables.condenser_entering_temperature.size());
            for (auto& T : grid_variables.condenser_entering_temperature)
                heatSourceTs_F.push_back(C_TO_F(K_TO_C(T)));
            perfGrid.push_back(heatSourceTs_F);
        }

        auto& lookup_variables = perf_map.lookup_variables;
        perfGridValues.reserve(2);

        std::size_t nVals = lookup_variables.input_power.size();
        std::vector<double> inputPowers_kW(nVals), cops(nVals);
        for (std::size_t i = 0; i < nVals; ++i)
        {
            inputPowers_kW[i] = lookup_variables.input_power[i] / 1000.;
            cops[i] = lookup_variables.heating_capacity[i] / lookup_variables.input_power[i];
        }

        if (isMultipass)
            perfGridValues.push_back(inputPowers_kW);
        else
        {
            std::vector<double> inputPowers_Btu_per_h(nVals);
            for (std::size_t i = 0; i < nVals; ++i)
                inputPowers_Btu_per_h[i] = KW_TO_BTUperH(inputPowers_kW[i]);
            perfGridValues.push_back(inputPowers_Btu_per_h);
        }

        perfGridValues.push_back(cops);

        makeBtwxt();
    }

    if (perf.use_defrost_map_is_set && perf.use_defrost_map)
    {
        setupDefrostMap();
    }

    if (perf.standby_power_is_set)
    {
        standbyPower_kW = perf.standby_power / 1000.;
    }

    if (hpwh->model == MODELS_NyleC60A_C_MP)
        resDefrost = {4.5, 5.0, 40.0}; // inputPower_KW, constTempLift_dF, onBelowTemp_F;
    else if (hpwh->model == MODELS_NyleC90A_C_MP)
        resDefrost = {5.4, 5.0, 40.0};
    else if (hpwh->model == MODELS_NyleC125A_C_MP)
        resDefrost = {9.0, 5.0, 40.0};
    else if (hpwh->model == MODELS_NyleC185A_C_MP)
        resDefrost = {7.25, 5.0, 40.0};
    else if (hpwh->model == MODELS_NyleC250A_C_MP)
        resDefrost = {18.0, 5.0, 40.0};

    if ((MODELS_NyleC25A_SP <= hpwh->model) && (hpwh->model <= MODELS_NyleC250A_C_SP))
    {
        maxOut_at_LowT.outT_C = F_TO_C(140.);
        maxOut_at_LowT.airT_C = F_TO_C(40.);
    }
}

void HPWH::Condenser::to(std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& hs) const
{
    if (configuration == COIL_CONFIG::CONFIG_EXTERNAL)
    {
        auto p_rshs =
            reinterpret_cast<hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP*>(
                hs.get());
        return to(*p_rshs);
    }
    else
    {
        auto p_rshs = reinterpret_cast<
            hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE*>(hs.get());
        return to(*p_rshs);
    }
}

void HPWH::Condenser::to(
    hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE& rshs) const
{
    productInformation_from_json(productInformation, rshs);

    //
    auto& perf = rshs.performance;
    switch (configuration)
    {
    case COIL_CONFIG::CONFIG_SUBMERGED:
    {
        perf.coil_configuration =
            hpwh_data_model::rscondenserwaterheatsource::CoilConfiguration::SUBMERGED;
        perf.coil_configuration_is_set = true;
        break;
    }
    case COIL_CONFIG::CONFIG_WRAPPED:
    {
        perf.coil_configuration =
            hpwh_data_model::rscondenserwaterheatsource::CoilConfiguration::WRAPPED;
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

    checkTo(C_TO_K(maxSetpoint_C),
            perf.maximum_refrigerant_temperature_is_set,
            perf.maximum_refrigerant_temperature);

    if (useBtwxtGrid)
    {
        auto& map = perf.performance_map;

        auto& grid_vars = map.grid_variables;

        //
        int iElem = 0;
        std::vector<double> envTemps_K = {};
        envTemps_K.reserve(perfGrid[0].size());
        for (auto T : perfGrid[0])
        {
            envTemps_K.push_back(C_TO_K(F_TO_C(T)));
        }
        if (K_TO_C(envTemps_K.front()) > minT)
            envTemps_K.push_back(C_TO_K(minT));
        if (K_TO_C(envTemps_K.back()) < maxT)
            envTemps_K.push_back(C_TO_K(maxT));
        arrangeGridVector(envTemps_K);

        checkTo(envTemps_K,
                grid_vars.evaporator_environment_dry_bulb_temperature_is_set,
                grid_vars.evaporator_environment_dry_bulb_temperature);

        ++iElem;

        //
        std::vector<double> heatSourceTemps_K = {};
        heatSourceTemps_K.reserve(perfGrid[iElem].size());
        for (auto T : perfGrid[iElem])
        {
            heatSourceTemps_K.push_back(C_TO_K(F_TO_C(T)));
        }

        checkTo(heatSourceTemps_K,
                grid_vars.heat_source_temperature_is_set,
                grid_vars.heat_source_temperature);

        //
        auto& lookup_vars = map.lookup_variables;

        std::size_t nVals =
            envTemps_K.size() * heatSourceTemps_K.size(); // perfGridValues[0].size();
        std::vector<double> inputPowers_W(nVals), heatingCapacities_W(nVals);
        std::size_t i = 0;
        for (auto& envTemp_K : envTemps_K)
            for (auto& heatSourceTemp_K : heatSourceTemps_K)
            {
                std::vector target = {C_TO_F(K_TO_C(envTemp_K)), C_TO_F(K_TO_C(heatSourceTemp_K))};
                std::vector<double> result = perfRGI->get_values_at_target(target);

                inputPowers_W[i] = 1000. * BTUperH_TO_KW(result[0]); // from Btu/h
                heatingCapacities_W[i] = result[1] * inputPowers_W[i];
                ++i;
            }

        checkTo(inputPowers_W, lookup_vars.input_power_is_set, lookup_vars.input_power);
        checkTo(
            heatingCapacities_W, lookup_vars.heating_capacity_is_set, lookup_vars.heating_capacity);

        perf.performance_map_is_set = true;
    }
    else // convert to grid
    {
        std::vector<std::vector<double>> tempGrid = {};
        std::vector<std::vector<double>> tempGridValues = {};

        makeGridFromMap(tempGrid, tempGridValues);

        auto& map = perf.performance_map;

        auto& grid_vars = map.grid_variables;
        int iElem = 0;
        checkTo(tempGrid[iElem],
                grid_vars.evaporator_environment_dry_bulb_temperature_is_set,
                grid_vars.evaporator_environment_dry_bulb_temperature);
        ++iElem;

        checkTo(tempGrid[iElem],
                grid_vars.heat_source_temperature_is_set,
                grid_vars.heat_source_temperature);

        auto& lookup_vars = map.lookup_variables;
        checkTo(tempGridValues[0], lookup_vars.input_power_is_set, lookup_vars.input_power);
        checkTo(
            tempGridValues[1], lookup_vars.heating_capacity_is_set, lookup_vars.heating_capacity);

        perf.performance_map_is_set = true;
    }
}

void HPWH::Condenser::to(hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP& rshs) const
{
    productInformation_from_json(rshs, productInformation);

    //
    auto& perf = rshs.performance;
    checkTo(doDefrost, perf.use_defrost_map_is_set, perf.use_defrost_map);

    checkTo(hysteresis_dC,
            perf.compressor_lockout_temperature_hysteresis_is_set,
            perf.compressor_lockout_temperature_hysteresis);

    checkTo(C_TO_K(maxSetpoint_C),
            perf.maximum_refrigerant_temperature_is_set,
            perf.maximum_refrigerant_temperature);

    if (useBtwxtGrid)
    {
        auto& map = perf.performance_map;

        auto& grid_vars = map.grid_variables;

        //
        std::size_t nVals = 1;
        int iElem = 0; // order based on MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP
        std::vector<double> envTemps_K = {};
        std::vector<double> outletTemps_K = {};
        std::vector<double> heatSourceTemps_K = {};
        {
            envTemps_K.reserve(perfGrid[iElem].size());
            for (auto T : perfGrid[iElem])
            {
                envTemps_K.push_back(C_TO_K(F_TO_C(T)));
            }
            if (K_TO_C(envTemps_K.front()) > minT)
                envTemps_K.push_back(C_TO_K(minT));
            if (K_TO_C(envTemps_K.back()) < maxT)
                envTemps_K.push_back(C_TO_K(maxT));
            arrangeGridVector(envTemps_K);

            checkTo(envTemps_K,
                    grid_vars.evaporator_environment_dry_bulb_temperature_is_set,
                    grid_vars.evaporator_environment_dry_bulb_temperature);

            ++iElem;
            nVals *= envTemps_K.size();
        }
        {
            outletTemps_K.reserve(perfGrid[iElem].size());
            for (auto T : perfGrid[iElem])
            {
                outletTemps_K.push_back(C_TO_K(F_TO_C(T)));
            }
            checkTo(outletTemps_K,
                    grid_vars.condenser_leaving_temperature_is_set,
                    grid_vars.condenser_leaving_temperature);

            ++iElem;
            nVals *= outletTemps_K.size();
        }
        {
            heatSourceTemps_K.reserve(perfGrid[iElem].size());
            for (auto T : perfGrid[iElem])
            {
                heatSourceTemps_K.push_back(C_TO_K(F_TO_C(T)));
            }

            checkTo(heatSourceTemps_K,
                    grid_vars.condenser_entering_temperature_is_set,
                    grid_vars.condenser_entering_temperature);
            ++iElem;
            nVals *= heatSourceTemps_K.size();
        }

        std::vector<double> inputPowers_W(nVals), heatingCapacities_W(nVals);
        std::size_t i = 0;
        for (auto& envTemp_K : envTemps_K)
            for (auto& outletTemp_K : outletTemps_K)
                for (auto& heatSourceTemp_K : heatSourceTemps_K)
                {
                    std::vector<double> target = {C_TO_F(K_TO_C(envTemp_K)),
                                                  C_TO_F(K_TO_C(outletTemp_K)),
                                                  C_TO_F(K_TO_C(heatSourceTemp_K))};
                    std::vector<double> result = perfRGI->get_values_at_target(target);

                    if (isMultipass)
                        inputPowers_W[i] = 1000. * result[0]; // from KW
                    else
                        inputPowers_W[i] = 1000. * BTUperH_TO_KW(result[0]); // from Btu/h

                    heatingCapacities_W[i] = result[1] * inputPowers_W[i];
                    ++i;
                }

        auto& lookup_vars = map.lookup_variables;
        checkTo(inputPowers_W, lookup_vars.input_power_is_set, lookup_vars.input_power);
        checkTo(
            heatingCapacities_W, lookup_vars.heating_capacity_is_set, lookup_vars.heating_capacity);

        perf.performance_map_is_set = true;
    }
    else // convert to grid
    {
        std::vector<std::vector<double>> tempGrid = {};
        std::vector<std::vector<double>> tempGridValues = {};

        makeGridFromMap(tempGrid, tempGridValues);

        auto& map = perf.performance_map;

        auto& grid_vars = map.grid_variables;
        int iElem = 0;
        {
            checkTo(tempGrid[iElem],
                    grid_vars.evaporator_environment_dry_bulb_temperature_is_set,
                    grid_vars.evaporator_environment_dry_bulb_temperature);
            ++iElem;
        }
        {
            checkTo(tempGrid[iElem],
                    grid_vars.condenser_leaving_temperature_is_set,
                    grid_vars.condenser_leaving_temperature);
            ++iElem;
        }
        {
            checkTo(tempGrid[iElem],
                    grid_vars.condenser_entering_temperature_is_set,
                    grid_vars.condenser_entering_temperature);
            ++iElem;
        }

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
    double adjCondenserTemp_C =
        condenserTemp_C + secondaryHeatExchanger.coldSideTemperatureOffset_dC;
    double adjOutletT_C = setpointTemp_C + secondaryHeatExchanger.hotSideTemperatureOffset_dC;

    if (useBtwxtGrid)
    {
        if (perfGrid.size() == 2)
        {
            std::vector<double> target {C_TO_F(externalT_C), C_TO_F(adjCondenserTemp_C)};
            btwxtInterp(input_BTUperHr, cop, target);
        }
        if (perfGrid.size() == 3)
        {
            std::vector<double> target {
                C_TO_F(externalT_C), C_TO_F(adjOutletT_C), C_TO_F(adjCondenserTemp_C)};
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
            getCapacityFromMap(externalT_C, adjCondenserTemp_C, adjOutletT_C, input_BTUperHr, cop);
        }
    }

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, C_TO_F(externalT_C));
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
    bool resDefrostHeatingOn = false;
    double adjCondenserTemp_C =
        condenserTemp_C + secondaryHeatExchanger.coldSideTemperatureOffset_dC;
    double adjOutletT_C = hpwh->getSetpoint() + secondaryHeatExchanger.hotSideTemperatureOffset_dC;

    // Check if we have resistance elements to turn on for defrost and add the constant lift.
    if (resDefrost.inputPwr_kW > 0)
    {
        if (externalT_C < F_TO_C(resDefrost.onBelowT_F))
        {
            externalT_C += dF_TO_dC(resDefrost.constTempLift_dF);
            resDefrostHeatingOn = true;
        }
    }

    if (useBtwxtGrid)
    {
        std::vector<double> target {
            C_TO_F(externalT_C), C_TO_F(adjOutletT_C), C_TO_F(adjCondenserTemp_C)};
        btwxtInterp(input_BTUperHr, cop, target);
        input_BTUperHr = KW_TO_BTUperH(input_BTUperHr);
    }
    else
    {
        getCapacityFromMap(externalT_C, adjCondenserTemp_C, adjOutletT_C, input_BTUperHr, cop);
    }

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, C_TO_F(externalT_C));
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

bool HPWH::Condenser::isExternal() const { return (configuration == Condenser::CONFIG_EXTERNAL); }

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
    input_BTUperHr = 0.;
    cop = 0.;

    if (perfMap.size() == 1) // central water-heater systems only
    {
        if ((environmentT_C > F_TO_C(perfMap[0].T_F)) && (extrapolationMethod == EXTRAP_NEAREST))
        {
            environmentT_C = F_TO_C(perfMap[0].T_F);
        }

        if (isMultipass)
        {
            regressedMethodMP(input_BTUperHr,
                              perfMap[0].inputPower_coeffs,
                              C_TO_F(environmentT_C),
                              C_TO_F(heatSourceT_C));
            input_BTUperHr = KW_TO_BTUperH(input_BTUperHr);

            regressedMethodMP(
                cop, perfMap[0].COP_coeffs, C_TO_F(environmentT_C), C_TO_F(heatSourceT_C));
        }
        else
        {
            regressedMethod(input_BTUperHr,
                            perfMap[0].inputPower_coeffs,
                            C_TO_F(environmentT_C),
                            C_TO_F(outletT_C),
                            C_TO_F(heatSourceT_C));
            input_BTUperHr = KW_TO_BTUperH(input_BTUperHr);

            regressedMethod(cop,
                            perfMap[0].COP_coeffs,
                            C_TO_F(environmentT_C),
                            C_TO_F(outletT_C),
                            C_TO_F(heatSourceT_C));
        }
    }
    else if (perfMap.size() > 1) // integrated heat-pump water heaters only
    {
        double COP_T1, COP_T2; // cop at ambient temperatures T1 and T2
        double inputPower_T1_W,
            inputPower_T2_W; // input power at ambient temperatures T1 and T2

        size_t i_prev = 0;
        size_t i_next = 1;

        double environmentT_F = C_TO_F(environmentT_C);
        double heatSourceT_F = C_TO_F(heatSourceT_C);
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
        input_BTUperHr = KW_TO_BTUperH(input_BTUperHr / 1000.); // 1000 converts w to kw;
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
void HPWH::Condenser::makeGridFromMap(std::vector<std::vector<double>>& tempGrid,
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
        { // environment temp
            std::vector<double> testEnvTemps_C = {
                0.,   0.5, 1.,   1.5,    2.,   2.5, 3.,   3.5, 4.,   4.5,       5.,   5.5,
                6.,   6.5, 7.,   7.2223, 7.5,  8.,  8.5,  9.,  9.5,  10.,       10.5, 11.,
                11.5, 12., 12.5, 13.,    13.5, 14., 14.5, 15., 15.5, 15.5555556};

            envTemps_K.push_back(C_TO_K(minT));
            for (auto& T_C : testEnvTemps_C)
            {
                if ((T_C > minT) && (T_C < maxT))
                    envTemps_K.push_back(C_TO_K(T_C));
            }
            envTemps_K.push_back(C_TO_K(maxT));
            arrangeGridVector(envTemps_K);
        }

        { // outlet temp
            if (isMultipass)
                outletTemps_K = {
                    C_TO_K(hpwh->setpoint_C + secondaryHeatExchanger.hotSideTemperatureOffset_dC)};
            else
            {
                double standardOutletT_K =
                    C_TO_K(hpwh->setpoint_C + secondaryHeatExchanger.hotSideTemperatureOffset_dC);
                outletTemps_K.reserve(3);
                outletTemps_K.push_back(standardOutletT_K);

                if (maxOut_at_LowT.airT_C > K_TO_C(envTemps_K.front()))
                    outletTemps_K.push_back(C_TO_K(maxOut_at_LowT.outT_C));

                const double highestTestSetpoint_C = 65.; // from testLargeCompHot
                if (highestTestSetpoint_C < maxSetpoint_C)
                    outletTemps_K.push_back(
                        C_TO_K(highestTestSetpoint_C +
                               secondaryHeatExchanger.hotSideTemperatureOffset_dC));

                arrangeGridVector(outletTemps_K);
            }
        }

        { // heat source temp
            const double minTemp_C = 0.;
            const double maxTemp_C = maxSetpoint_C;
            auto tempRange_dC = maxTemp_C - minTemp_C;
            auto nSteps = static_cast<std::size_t>(std::max(51. * tempRange_dC / 100., 2.));
            auto dHeatSourceT_dC = tempRange_dC / static_cast<double>(nSteps);
            heatSourceTemps_K.reserve(nSteps + 1);
            for (std::size_t i = 0; i <= nSteps; ++i)
            {
                double heatSourceTemp_C = minTemp_C + dHeatSourceT_dC * static_cast<double>(i);
                heatSourceTemps_K.push_back(C_TO_K(heatSourceTemp_C));
            }
            arrangeGridVector(heatSourceTemps_K);
        }

        // heat-source Ts
        tempGrid.reserve(3);
        tempGrid.push_back(envTemps_K);
        tempGrid.push_back(outletTemps_K); // not used
        tempGrid.push_back(heatSourceTemps_K);

        std::size_t nVals = 1;
        for (auto& axis : tempGrid)
            nVals *= axis.size();
        std::vector<double> inputPowers_W(nVals), heatingCapacities_W(nVals);
        std::size_t i = 0;
        double input_BTUperHr, cop;
        for (auto& envTemp_K : envTemps_K)
            for (auto& outletTemp_K : outletTemps_K)
                for (auto& heatSourceTemp_K : heatSourceTemps_K)
                {
                    getCapacityFromMap(K_TO_C(envTemp_K),
                                       K_TO_C(heatSourceTemp_K),
                                       K_TO_C(outletTemp_K),
                                       input_BTUperHr,
                                       cop);
                    inputPowers_W[i] = 1000. * BTUperH_TO_KW(input_BTUperHr);
                    heatingCapacities_W[i] = cop * inputPowers_W[i];
                    ++i;
                }

        // outlet Ts
        tempGridValues.push_back(inputPowers_W);
        tempGridValues.push_back(heatingCapacities_W);
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
                double magPowerCurvature = fabs(perfPoint.inputPower_coeffs[2]);
                double magCOPCurvature = fabs(perfPoint.COP_coeffs[2]);
                maxPowerCurvature =
                    magPowerCurvature > maxPowerCurvature ? magPowerCurvature : maxPowerCurvature;
                maxCOPCurvature =
                    magCOPCurvature > maxCOPCurvature ? magCOPCurvature : maxCOPCurvature;
            }
        }
        envTemps_K.push_back(C_TO_K(maxT));
        tempGrid.push_back(envTemps_K);

        if (configuration == COIL_CONFIG::CONFIG_EXTERNAL)
        {
            tempGrid.push_back(
                {C_TO_K(hpwh->setpoint_C + secondaryHeatExchanger.hotSideTemperatureOffset_dC)});
        }

        // relate to reference values (from AOSmithPHPT60)
        const double minHeatSourceTemp_C = 0.;
        const double maxHeatSourceTemp_C = maxSetpoint_C;
        const double heatSourceTempRange_dC = maxHeatSourceTemp_C - minHeatSourceTemp_C;
        const double heatSourceTempRangeRef_dC = 100. - 0.;
        const double rangeFac = heatSourceTempRange_dC / heatSourceTempRangeRef_dC;

        constexpr double minVals = 2;
        constexpr double refPowerVals = 11;
        constexpr double refCOP_vals = 11;
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

// convert to grid
void HPWH::Condenser::convertMapToGrid()
{

    std::vector<std::vector<double>> tempGrid;
    std::vector<std::vector<double>> tempGridValues;
    makeGridFromMap(tempGrid, tempGridValues);
    perfGrid.reserve(tempGrid.size());
    for (auto& tempGridAxis : tempGrid)
    {
        std::vector<double> gridAxis;
        gridAxis.reserve(tempGridAxis.size());
        for (auto& point : tempGridAxis)
            gridAxis.push_back(C_TO_F(K_TO_C(point)));
        perfGrid.push_back(gridAxis);
    }

    auto is_integrated = (configuration != COIL_CONFIG::CONFIG_EXTERNAL);
    perfGridValues.resize(2);
    perfGridValues[0].reserve(tempGridValues[0].size());
    for (auto& point : tempGridValues[0])
        if (is_integrated)
            perfGridValues[0].push_back(KW_TO_BTUperH(point / 1000.));
        else
        {
            if (isMultipass)
                perfGridValues[0].push_back(point / 1000.); // MP in KW
            else
                perfGridValues[0].push_back(KW_TO_BTUperH(point / 1000.)); // others in Btu/h
        }

    perfGridValues[1].reserve(tempGridValues[1].size());
    for (std::size_t i = 0; i < tempGridValues[1].size(); ++i)
        perfGridValues[1].push_back(tempGridValues[1][i] / tempGridValues[0][i]);

    makeBtwxt();
}
