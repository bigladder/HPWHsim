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
                           std::shared_ptr<Courier::Courier> courier,
                           const std::string& name_in)
    : HeatSource(hpwh_in, courier, name_in)
    , hysteresis_dC(0)
    , maxSetpoint_C(100.)
    , doDefrost(false)
    , maxOut_at_LowT {100, -273.15}
    , secondaryHeatExchanger {0., 0., 0.}
    , standbyPower_kW(0.)
    , configuration(COIL_CONFIG::CONFIG_WRAPPED)
    , isMultipass(true)
    , evaluatePerformance(nullptr)
    , inputPowerScale(1.)
    , COP_scale(1.)
{
}

HPWH::Condenser& HPWH::Condenser::operator=(const HPWH::Condenser& cond_in)
{
    HPWH::HeatSource::operator=(cond_in);

    Tshrinkage_C = cond_in.Tshrinkage_C;
    lockedOut = cond_in.lockedOut;

    perfRGI = cond_in.perfRGI;
    evaluatePerformance = cond_in.evaluatePerformance;

    defrostMap = cond_in.defrostMap;
    resDefrost = cond_in.resDefrost;

    configuration = cond_in.configuration;
    isMultipass = cond_in.isMultipass;
    mpFlowRate_LPS = cond_in.mpFlowRate_LPS;

    externalInletHeight = cond_in.externalInletHeight;
    externalOutletHeight = cond_in.externalOutletHeight;

    lowestNode = cond_in.lowestNode;
    secondaryHeatExchanger = cond_in.secondaryHeatExchanger;

    hysteresis_dC = cond_in.hysteresis_dC;
    maxSetpoint_C = cond_in.maxSetpoint_C;

    description = cond_in.description;
    productInformation = cond_in.productInformation;

    inputPowerScale = cond_in.inputPowerScale;
    COP_scale = cond_in.COP_scale;
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
        constexpr double tolT_C = 1.e-9;
        // when the "external" temperature is too cold - typically used for compressor low temp.
        // cutoffs when running, use hysteresis
        bool lock = false;
        if (isEngaged() && (heatSourceAmbientT_C + tolT_C < minT - hysteresis_dC))
        {
            lock = true;
        }
        // when not running, don't use hysteresis
        else if (!isEngaged() && (heatSourceAmbientT_C + tolT_C < minT))
        {
            lock = true;
        }

        // when the "external" temperature is too warm - typically used for resistance lockout
        // when running, use hysteresis
        if (isEngaged() && (heatSourceAmbientT_C > maxT + hysteresis_dC + tolT_C))
        {
            lock = true;
        }
        // when not running, don't use hysteresis
        else if (!isEngaged() && (heatSourceAmbientT_C > maxT + tolT_C))
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
        constexpr double tolT_C = 1.e-9;
        // when the "external" temperature is no longer too cold or too warm
        // when running, use hysteresis
        bool unlock = false;
        if (isEngaged() && (heatSourceAmbientT_C > minT + hysteresis_dC + tolT_C) &&
            (heatSourceAmbientT_C + tolT_C < maxT - hysteresis_dC))
        {
            unlock = true;
        }
        // when not running, don't use hysteresis
        else if (!isEngaged() && (heatSourceAmbientT_C > minT + tolT_C) &&
                 (heatSourceAmbientT_C + tolT_C < maxT))
        {
            unlock = true;
        }
        return unlock;
    }
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

void trimGridVector(std::vector<double>& V, const double minT, const double maxT)
{
    arrangeGridVector(V);
    auto copyV = V;
    V.clear();
    for (auto& x : copyV) // skip duplicates
    {
        if ((minT <= x) && (x <= maxT))
            V.push_back(x);
    }
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
    const hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE& hs)
{
    description.from(hs);
    productInformation.from(hs);

    auto& perf = hs.performance;

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

        std::vector<std::vector<double>> perfGrid = {};
        std::vector<std::vector<double>> perfGridValues = {};

        perfGrid.reserve(3);
        // order based on hpwh_presets::MODELS::Mitsubishi_QAHV_N136TAU_HPB_SP
        if (grid_variables.evaporator_environment_dry_bulb_temperature_is_set)
        {
            std::vector<double> evapTs_C = {};
            evapTs_C.reserve(grid_variables.evaporator_environment_dry_bulb_temperature.size());
            for (auto& T_K : grid_variables.evaporator_environment_dry_bulb_temperature)
                evapTs_C.push_back(K_TO_C(T_K));
            perfGrid.push_back(evapTs_C);
            maxT = evapTs_C.back();
            minT = evapTs_C.front();
        }

        if (grid_variables.heat_source_temperature_is_set)
        {
            std::vector<double> heatSourceTs_C = {};
            heatSourceTs_C.reserve(grid_variables.heat_source_temperature.size());
            for (auto& T_K : grid_variables.heat_source_temperature)
                heatSourceTs_C.push_back(K_TO_C(T_K));
            perfGrid.push_back(heatSourceTs_C);
        }

        auto& lookup_variables = perf_map.lookup_variables;
        perfGridValues.reserve(2);

        std::size_t nVals = lookup_variables.input_power.size();
        std::vector<double> inputPowers_W(nVals), cops(nVals);
        for (std::size_t i = 0; i < nVals; ++i)
        {
            inputPowers_W[i] = lookup_variables.input_power[i];
            cops[i] = lookup_variables.heating_capacity[i] / lookup_variables.input_power[i];
        }

        perfGridValues.push_back(inputPowers_W);
        perfGridValues.push_back(cops);
        makePerformanceBtwxt(perfGrid, perfGridValues);
    }

    if (perf.use_defrost_map_is_set && perf.use_defrost_map)
    {
        setupDefrostMap();
    }

    if (perf.standby_power_is_set)
    {
        standbyPower_kW = W_TO_KW(perf.standby_power);
    }
}

void HPWH::Condenser::from(const hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP& hs)
{
    description.from(hs);
    productInformation.from(hs);

    configuration = COIL_CONFIG::CONFIG_EXTERNAL;

    auto& perf = hs.performance;

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

        std::vector<std::vector<double>> perfGrid = {};
        std::vector<std::vector<double>> perfGridValues = {};

        auto& grid_variables = perf_map.grid_variables;
        perfGrid.reserve(3);
        // order based on hpwh_presets::MODELS::MITSUBISHI_QAHV_N136TAU_HPB_SP
        if (grid_variables.evaporator_environment_dry_bulb_temperature_is_set)
        {
            std::vector<double> evapTs_C = {};
            evapTs_C.reserve(grid_variables.evaporator_environment_dry_bulb_temperature.size());
            for (auto& T_K : grid_variables.evaporator_environment_dry_bulb_temperature)
                evapTs_C.push_back(K_TO_C(T_K));
            perfGrid.push_back(evapTs_C);
            maxT = evapTs_C.back();
            minT = evapTs_C.front();
        }

        if (grid_variables.condenser_leaving_temperature_is_set)
        {
            std::vector<double> outletTs_C = {};
            outletTs_C.reserve(grid_variables.condenser_leaving_temperature.size());
            for (auto& T_K : grid_variables.condenser_leaving_temperature)
                outletTs_C.push_back(K_TO_C(T_K));
            perfGrid.push_back(outletTs_C);
        }

        if (grid_variables.condenser_entering_temperature_is_set)
        {
            std::vector<double> heatSourceTs_C = {};
            heatSourceTs_C.reserve(grid_variables.condenser_entering_temperature.size());
            for (auto& T_K : grid_variables.condenser_entering_temperature)
                heatSourceTs_C.push_back(K_TO_C(T_K));
            perfGrid.push_back(heatSourceTs_C);
        }

        auto& lookup_variables = perf_map.lookup_variables;
        perfGridValues.reserve(2);

        std::size_t nVals = lookup_variables.input_power.size();
        std::vector<double> inputPowers_W(nVals), cops(nVals);
        for (std::size_t i = 0; i < nVals; ++i)
        {
            inputPowers_W[i] = lookup_variables.input_power[i];
            cops[i] = lookup_variables.heating_capacity[i] / lookup_variables.input_power[i];
        }

        perfGridValues.push_back(inputPowers_W);
        perfGridValues.push_back(cops);

        makePerformanceBtwxt(perfGrid, perfGridValues);
    }

    if (perf.use_defrost_map_is_set && perf.use_defrost_map)
    {
        setupDefrostMap();
    }

    if (perf.standby_power_is_set)
    {
        standbyPower_kW = W_TO_KW(perf.standby_power);
    }

    if (hpwh->model == hpwh_presets::MODELS::NyleC60A_C_MP)
        resDefrost = {4.5, 5.0, 40.0}; // inputPower_KW, constTempLift_dF, onBelowTemp_F;
    else if (hpwh->model == hpwh_presets::MODELS::NyleC90A_C_MP)
        resDefrost = {5.4, 5.0, 40.0};
    else if (hpwh->model == hpwh_presets::MODELS::NyleC125A_C_MP)
        resDefrost = {9.0, 5.0, 40.0};
    else if (hpwh->model == hpwh_presets::MODELS::NyleC185A_C_MP)
        resDefrost = {7.25, 5.0, 40.0};
    else if (hpwh->model == hpwh_presets::MODELS::NyleC250A_C_MP)
        resDefrost = {18.0, 5.0, 40.0};

    if ((hpwh_presets::MODELS::NyleC25A_SP <= hpwh->model) &&
        (hpwh->model <= hpwh_presets::MODELS::NyleC250A_C_SP))
    {
        maxOut_at_LowT.outT_C = F_TO_C(140.);
        maxOut_at_LowT.airT_C = F_TO_C(40.);
    }
}

void HPWH::Condenser::to(std::unique_ptr<hpwh_data_model::ashrae205::HeatSourceTemplate>& hs) const
{
    if (configuration == COIL_CONFIG::CONFIG_EXTERNAL)
    {
        auto hsp = reinterpret_cast<hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP*>(
            hs.get());
        return to(*hsp);
    }
    else
    {
        auto hsp = reinterpret_cast<
            hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE*>(hs.get());
        return to(*hsp);
    }
}
void HPWH::Condenser::to(
    hpwh_data_model::rscondenserwaterheatsource::RSCONDENSERWATERHEATSOURCE& hs) const
{
    generate_metadata<hpwh_data_model::rscondenserwaterheatsource::Schema>(
        hs,
        "RSCONDENSERWATERHEATSOURCE",
        "https://github.com/bigladder/hpwh-data-model/blob/main/schema/"
        "RSCONDENSERWATERHEATSOURCE.schema.yaml");

    description.to(hs);
    productInformation.to(hs);

    auto& perf = hs.performance;
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

    //
    auto& map = perf.performance_map;
    auto& grid_vars = map.grid_variables;
    auto& lookup_vars = map.lookup_variables;

    std::vector<std::vector<double>> perfGrid = {};
    std::vector<std::vector<double>> perfGridValues = {};

    std::vector<double> envTs_K = {};
    std::vector<double> heatSourceTs_K = {};

    std::vector<double> inputPowers_W = {}, heatingCapacities_W = {};

    if (perfRGI)
    {
        for (std::size_t i = 0; i < perfRGI->get_number_of_dimensions(); ++i)
            perfGrid.push_back(perfRGI->get_grid_axis(i).get_values());

        std::size_t nVals = 1;

        std::vector<double>& envTs_C = perfGrid[0];
        envTs_C.push_back(minT);
        envTs_C.push_back(maxT);
        trimGridVector(envTs_C, minT, maxT);

        std::vector<double>& heatSourceTs_C = perfGrid[1];

        int iElem = 0;
        {
            envTs_K.reserve(envTs_C.size());
            for (auto T_C : envTs_C)
            {
                envTs_K.push_back(C_TO_K(T_C));
            }

            ++iElem;
            nVals *= envTs_K.size();
        }
        {
            heatSourceTs_K.reserve(heatSourceTs_C.size());
            for (auto T_C : heatSourceTs_C)
            {
                heatSourceTs_K.push_back(C_TO_K(T_C));
            }

            ++iElem;
            nVals *= heatSourceTs_K.size();
        }

        inputPowers_W.reserve(nVals);
        heatingCapacities_W.reserve(nVals);
        for (auto& envT_C : envTs_C)
            for (auto& heatSourceT_C : heatSourceTs_C)
            {
                auto performance = evaluatePerformance(envT_C, heatSourceT_C);
                inputPowers_W.push_back(performance.inputPower_W);
                heatingCapacities_W.push_back(performance.outputPower_W);
            }
    }
    else // convert evaluatePerformance function to grid
    {
        {
            std::vector<double> envTs_C = {minT, F_TO_C(50.), F_TO_C(67.5), F_TO_C(95.), maxT};
            trimGridVector(envTs_C, minT, maxT);
            for (auto& envT_C : envTs_C)
                envTs_K.push_back(C_TO_K(envT_C));
        }
        {
            constexpr double minVals = 2.;  // retain endpoints only, if no curvature
            constexpr double refVals = 11.; // typical value
            const double rangeFac = (maxSetpoint_C - 0.) / (100. - 0.);
            auto nVals = static_cast<std::size_t>(rangeFac * (refVals - minVals) + minVals);
            double dT_C = (maxSetpoint_C - 0.) / static_cast<double>(nVals - 1);
            std::vector<double> heatSourceTs_C = {};
            for (double T_C = 0.; T_C <= maxSetpoint_C; T_C += dT_C)
            {
                heatSourceTs_K.push_back(C_TO_K(T_C));
            }
        }
        {
            // fill grid values
            std::size_t nTotVals = envTs_K.size() * heatSourceTs_K.size();
            inputPowers_W.reserve(nTotVals);
            heatingCapacities_W.reserve(nTotVals);
            for (auto& envT_K : envTs_K)
                for (auto& heatSourceT_K : heatSourceTs_K)
                {
                    auto performance = evaluatePerformance(K_TO_C(envT_K), K_TO_C(heatSourceT_K));
                    inputPowers_W.push_back(performance.inputPower_W);
                    heatingCapacities_W.push_back(performance.cop * performance.inputPower_W);
                }
        }
    }

    checkTo(envTs_K,
            grid_vars.evaporator_environment_dry_bulb_temperature_is_set,
            grid_vars.evaporator_environment_dry_bulb_temperature);

    checkTo(heatSourceTs_K,
            grid_vars.heat_source_temperature_is_set,
            grid_vars.heat_source_temperature);

    checkTo(inputPowers_W, lookup_vars.input_power_is_set, lookup_vars.input_power);
    checkTo(heatingCapacities_W, lookup_vars.heating_capacity_is_set, lookup_vars.heating_capacity);

    map.grid_variables_is_set = true;
    map.lookup_variables_is_set = true;
    perf.performance_map_is_set = true;

    hs.performance_is_set = true;
}

void HPWH::Condenser::to(hpwh_data_model::rsairtowaterheatpump::RSAIRTOWATERHEATPUMP& hs) const
{
    generate_metadata<hpwh_data_model::rsairtowaterheatpump::Schema>(
        hs,
        "RSAIRTOWATERHEATPUMP",
        "https://github.com/bigladder/hpwh-data-model/blob/main/schema/"
        "RSAIRTOWATERHEATPUMP.schema.yaml");

    description.to(hs);
    productInformation.to(hs);

    //
    auto& perf = hs.performance;
    checkTo(doDefrost, perf.use_defrost_map_is_set, perf.use_defrost_map);

    checkTo(hysteresis_dC,
            perf.compressor_lockout_temperature_hysteresis_is_set,
            perf.compressor_lockout_temperature_hysteresis);

    checkTo(C_TO_K(maxSetpoint_C),
            perf.maximum_refrigerant_temperature_is_set,
            perf.maximum_refrigerant_temperature);

    auto& map = perf.performance_map;
    auto& grid_vars = map.grid_variables;

    std::vector<double> envTs_K = {};
    std::vector<double> outletTs_K = {};
    std::vector<double> heatSourceTs_K = {};

    std::vector<double> inputPowers_W = {}, heatingCapacities_W = {};
    if (perfRGI)
    {
        std::vector<std::vector<double>> perfGrid = {};

        for (std::size_t i = 0; i < perfRGI->get_number_of_dimensions(); ++i)
            perfGrid.push_back(perfRGI->get_grid_axis(i).get_values());

        //
        std::size_t nVals = 1;

        std::vector<double>& envTs_C = perfGrid[0];
        envTs_C.push_back(minT);
        envTs_C.push_back(maxT);
        trimGridVector(envTs_C, minT, maxT);

        std::vector<double>& outletTs_C = perfGrid[1];
        std::vector<double>& heatSourceTs_C = perfGrid[2];

        int iElem = 0;
        {
            envTs_K.reserve(envTs_C.size());
            for (auto T_C : envTs_C)
                envTs_K.push_back(C_TO_K(T_C));

            ++iElem;
            nVals *= envTs_K.size();
        }
        {
            outletTs_K.reserve(outletTs_C.size());
            for (auto T_C : outletTs_C)
                outletTs_K.push_back(C_TO_K(T_C));

            ++iElem;
            nVals *= outletTs_K.size();
        }
        {
            heatSourceTs_K.reserve(perfGrid[iElem].size());
            for (auto T_C : perfGrid[iElem])
                heatSourceTs_K.push_back(C_TO_K(T_C));

            ++iElem;
            nVals *= heatSourceTs_K.size();
        }

        inputPowers_W.reserve(nVals);
        heatingCapacities_W.reserve(nVals);
        for (auto& envT_C : envTs_C)
            for (auto& outletT_C : outletTs_C)
            {
                hpwh->setpoint_C = outletT_C - secondaryHeatExchanger.hotSideTemperatureOffset_dC;
                for (auto& heatSourceT_C : heatSourceTs_C)
                {
                    auto performance = evaluatePerformance(envT_C, heatSourceT_C);
                    inputPowers_W.push_back(performance.inputPower_W);
                    heatingCapacities_W.push_back(performance.outputPower_W);
                }
            }
    }
    else
    {
        { // fill vector of environment temps, selected from tests
            std::vector<double> envTs_C = {minT,   0.,  0.5,  1.,         1.5,  2.,  2.5,  3.,
                                           3.5,    4.,  4.5,  5.,         5.5,  6.,  6.5,  7.,
                                           7.2223, 7.5, 8.,   8.28,       8.5,  9.,  9.5,  10.,
                                           10.5,   11., 11.5, 12.,        12.5, 13., 13.5, 14.,
                                           14.5,   15., 15.5, 15.5555556, 20.,  29., 30.,  maxT};

            trimGridVector(envTs_C, minT, maxT);
            for (auto& envT_C : envTs_C)
                envTs_K.push_back(C_TO_K(envT_C));
        }

        { // fill vector of outlet temps
            std::vector<double> outletTs_C = {hpwh->setpoint_C};
            if (!isMultipass)
            {
                if (!hpwh->isSetpointFixed())
                {
                    outletTs_C.push_back(0.);
                    outletTs_C.push_back(maxSetpoint_C);
                    outletTs_C.push_back(maxOut_at_LowT.outT_C);
                    outletTs_C.push_back(65.);          // from testLargeCompHot
                    outletTs_C.push_back(F_TO_C(120.)); // from dhw_mfsizing.cse
                    trimGridVector(outletTs_C, 0., maxSetpoint_C);
                }
            }
            for (auto& outletT_C : outletTs_C)
                outletTs_K.push_back(
                    C_TO_K(outletT_C + secondaryHeatExchanger.hotSideTemperatureOffset_dC));
        }

        { // fill vector of heat source temps
            auto tempRange_dC = maxSetpoint_C - 0.;
            constexpr double steps_per_degC = 51. / 100.;
            auto nSteps = static_cast<std::size_t>(std::max(steps_per_degC * tempRange_dC, 2.));
            auto dHeatSourceT_dC = tempRange_dC / static_cast<double>(nSteps);
            heatSourceTs_K.reserve(nSteps + 1);
            for (std::size_t i = 0; i <= nSteps; ++i)
            {
                double heatSourceT_C = 0. + dHeatSourceT_dC * static_cast<double>(i);
                heatSourceTs_K.push_back(C_TO_K(heatSourceT_C));
            }
            arrangeGridVector(heatSourceTs_K);
        }
        { // fill grid values
            std::size_t nTotVals = envTs_K.size() * outletTs_K.size() * heatSourceTs_K.size();
            inputPowers_W.reserve(nTotVals);
            heatingCapacities_W.reserve(nTotVals);
            double orig_setpointT_C = hpwh->getSetpoint(UNITS_C);
            for (auto& envT_K : envTs_K)
                for (auto& outletT_K : outletTs_K)
                {
                    hpwh->setSetpoint(K_TO_C(outletT_K) -
                                          secondaryHeatExchanger.hotSideTemperatureOffset_dC,
                                      UNITS_C);
                    for (auto& heatSourceT_K : heatSourceTs_K)
                    {
                        auto performance =
                            evaluatePerformance(K_TO_C(envT_K), K_TO_C(heatSourceT_K));
                        inputPowers_W.push_back(performance.inputPower_W);
                        heatingCapacities_W.push_back(performance.cop * performance.inputPower_W);
                    }
                }
            hpwh->setSetpoint(orig_setpointT_C, UNITS_C);
        }
    }

    checkTo(envTs_K,
            grid_vars.evaporator_environment_dry_bulb_temperature_is_set,
            grid_vars.evaporator_environment_dry_bulb_temperature);

    checkTo(outletTs_K,
            grid_vars.condenser_leaving_temperature_is_set,
            grid_vars.condenser_leaving_temperature);

    checkTo(heatSourceTs_K,
            grid_vars.condenser_entering_temperature_is_set,
            grid_vars.condenser_entering_temperature);

    auto& lookup_vars = map.lookup_variables;
    checkTo(inputPowers_W, lookup_vars.input_power_is_set, lookup_vars.input_power);
    checkTo(heatingCapacities_W, lookup_vars.heating_capacity_is_set, lookup_vars.heating_capacity);

    map.grid_variables_is_set = true;
    map.lookup_variables_is_set = true;
    perf.performance_map_is_set = true;

    hs.performance_is_set = true;
}

void HPWH::Condenser::addHeat(double externalT_C, double minutesToRun)
{
    Performance performance = {0., 0., 0.};

    switch (configuration)
    {
    case CONFIG_SUBMERGED:
    case CONFIG_WRAPPED:
    {
        // calculate capacity btu/hr, input btu/hr, and cop
        hpwh->condenserInlet_C = getTankTemp();
        performance = getPerformance(externalT_C, getTankTemp());
        double cap_kJ = W_TO_KW(performance.outputPower_W) * (60. * minutesToRun);

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
            runtime_min = addHeatExternalMP(externalT_C, minutesToRun, performance);
        }
        else
        {
            runtime_min = addHeatExternal(externalT_C, minutesToRun, performance);
        }
        break;
    }

    // update the input & output energy
    energyInput_kWh += W_TO_KW(performance.inputPower_W) * (runtime_min / min_per_hr);
    energyOutput_kWh += W_TO_KW(performance.outputPower_W) * (runtime_min / min_per_hr);
}

HPWH::Performance HPWH::Condenser::getPerformance(double externalT_C, double condenserT_C) const
{
    bool resDefrostHeatingOn = false;

    // Add an offset to the condenser temperature (or incoming coldwater temperature) to approximate
    // a secondary heat exchange in line with the compressor
    condenserT_C += secondaryHeatExchanger.coldSideTemperatureOffset_dC;

    // Check if we have resistance elements to turn on for defrost and add the constant lift.
    if (resDefrost.inputPwr_kW > 0)
    {
        if (externalT_C < F_TO_C(resDefrost.onBelowT_F))
        {
            externalT_C += dF_TO_dC(resDefrost.constTempLift_dF);
            resDefrostHeatingOn = true;
        }
    }

    auto performance = evaluatePerformance(externalT_C, condenserT_C);
    performance.inputPower_W *= inputPowerScale;
    performance.cop *= COP_scale;
    performance.outputPower_W = performance.cop * performance.inputPower_W;

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(performance.cop, C_TO_F(externalT_C));
        performance.outputPower_W = performance.cop * performance.inputPower_W;
    }

    // here is where the scaling for flow restriction happens
    // the input power doesn't change, we just scale the cop by a small percentage
    // that is based on the flow rate.  The equation is a fit to three points,
    // measured experimentally - 12 percent reduction at 150 cfm, 10 percent at
    // 200, and 0 at 375. Flow is expressed as fraction of full flow.
    if (airflowFreedom != 1)
    {
        double airflow = 375 * airflowFreedom;
        performance.cop *= 0.00056 * airflow + 0.79;
        performance.outputPower_W = performance.cop * performance.inputPower_W;
    }

    // For accounting add the resistance defrost to the input energy
    if (resDefrostHeatingOn)
    {
        performance.inputPower_W += KW_TO_W(resDefrost.inputPwr_kW);
    }

    if (performance.cop < 0.)
    {
        send_warning("Warning: COP is Negative!");
    }
    if (performance.cop < 1.)
    {
        send_warning("Warning: COP is Less than 1!");
    }
    return performance;
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
    HPWH::linearInterp(derate_factor,
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
                                        Performance& netPerformance)
{
    netPerformance = {0., 0., 0.};

    double setpointT_C = std::min(maxSetpoint_C, hpwh->setpoint_C);
    double remainingTime_min = stepTime_min;
    do
    {
        double& externalOutletT_C = hpwh->tank->nodeTs_C[externalOutletHeight];

        // how much heat is available in remaining time
        auto tempPerformance = getPerformance(externalT_C, externalOutletT_C);

        double heatingPower_kW = W_TO_KW(tempPerformance.outputPower_W);

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
        netPerformance.inputPower_W +=
            (tempPerformance.inputPower_W + secondaryHeatExchanger.extraPumpPower_W) *
            heatingTime_min;
        netPerformance.outputPower_W += tempPerformance.outputPower_W * heatingTime_min;
        netPerformance.cop += tempPerformance.cop * heatingTime_min;

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
        netPerformance.inputPower_W /= runTime_min;
        netPerformance.outputPower_W /= runTime_min;
        netPerformance.cop /= runTime_min;
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
                                          Performance& netPerformance)
{
    netPerformance = {0., 0., 0.};

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

        double& externalOutletT_C = hpwh->tank->nodeTs_C[externalOutletHeight];

        // find heating capacity
        auto tempPerformance = getPerformance(externalT_C, externalOutletT_C);

        double heatingPower_kW = W_TO_KW(tempPerformance.outputPower_W);

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
        netPerformance.inputPower_W +=
            (tempPerformance.inputPower_W + secondaryHeatExchanger.extraPumpPower_W) *
            heatingTime_min;
        netPerformance.outputPower_W += tempPerformance.outputPower_W * heatingTime_min;
        netPerformance.cop += tempPerformance.cop * heatingTime_min;

        hpwh->externalVolumeHeated_L += nodeFrac * hpwh->tank->nodeVolume_L;

        // continue until time expired or cutoff condition met
    } while ((remainingTime_min > 0.) && (!shutsOff()));

    // time elapsed in this function
    double elapsedTime_min = stepTime_min - remainingTime_min;
    if (elapsedTime_min > 0.)
    {
        netPerformance.inputPower_W /= elapsedTime_min;
        netPerformance.outputPower_W /= elapsedTime_min;
        netPerformance.cop /= elapsedTime_min;
        hpwh->condenserInlet_C /= elapsedTime_min;
        hpwh->condenserOutlet_C /= elapsedTime_min;
    }

    // return the elapsed time
    return elapsedTime_min;
}

bool HPWH::Condenser::isExternalMultipass() const
{
    return isMultipass && (configuration == Condenser::CONFIG_EXTERNAL);
}

bool HPWH::Condenser::isExternal() const { return (configuration == Condenser::CONFIG_EXTERNAL); }

std::vector<Btwxt::GridAxis>
HPWH::Condenser::setUpGridAxes(const std::vector<std::vector<double>>& perfGrid)
{
    // integrated condensers:
    //   Legacy method used linear interpolation on externalT axis and quadratic evaluation on
    //   condenserT axis, found to be most closely reproduced by cubic interpolation.
    auto is_integrated = (configuration != COIL_CONFIG::CONFIG_EXTERNAL);

    // single-pass condensers:
    //   Legacy method used 3D quadratic polynomials with cross-terms. Cubic interpolation on the
    //   externalT and condenserT axes most closely reproduced the performance curve.
    //   (Linear best reproduced curves on the outletT axis, which typically has few points.)
    //   Mitsubishi model used a grid with linear interpolation, preserved here.
    auto is_Mitsubishi = (hpwh->model == hpwh_presets::MODELS::Mitsubishi_QAHV_N136TAU_HPB_SP);

    // multi-pass condensers:
    //   Legacy method used 2D quadratic polynomials with cross-terms. These were best reproduced
    //   with cubic interpolation. Nyle-MP models used grids with default btwxt methods, preserved
    //   here.
    auto is_NyleMP = ((hpwh_presets::MODELS::NyleC60A_MP <= hpwh->model) &&
                      (hpwh->model <= hpwh_presets::MODELS::NyleC250A_C_MP));

    // Linear extrapolation used in all legacy representation preserved here.

    std::vector<Btwxt::GridAxis> grid_axes = {};
    std::size_t iAxis = 0;
    { // external T
        auto interpMethod = (is_Mitsubishi || is_NyleMP || is_integrated)
                                ? Btwxt::InterpolationMethod::linear
                                : Btwxt::InterpolationMethod::cubic;
        auto extrapMethod = (is_Mitsubishi || is_NyleMP) ? Btwxt::ExtrapolationMethod::constant
                                                         : Btwxt::ExtrapolationMethod::linear;
        grid_axes.push_back(Btwxt::GridAxis(perfGrid[iAxis],
                                            interpMethod,
                                            extrapMethod,
                                            {-DBL_MAX, DBL_MAX},
                                            "ExternalT",
                                            get_courier()));
        ++iAxis;
    }
    if (perfGrid.size() > 2)
    { // condenser outlet T (CWHS only)
        auto interpMethod = (is_Mitsubishi) ? Btwxt::InterpolationMethod::linear
                                            : Btwxt::InterpolationMethod::linear;
        auto extrapMethod = (is_Mitsubishi) ? Btwxt::ExtrapolationMethod::constant
                                            : Btwxt::ExtrapolationMethod::linear;
        grid_axes.push_back(Btwxt::GridAxis(perfGrid[iAxis],
                                            interpMethod,
                                            extrapMethod,
                                            {-DBL_MAX, DBL_MAX},
                                            "CondenserOutletT",
                                            get_courier()));
        ++iAxis;
    }
    { // heat-source T
        auto interpMethod = (is_Mitsubishi || is_NyleMP) ? Btwxt::InterpolationMethod::linear
                                                         : Btwxt::InterpolationMethod::cubic;
        auto extrapMethod = (is_Mitsubishi) ? Btwxt::ExtrapolationMethod::linear
                                            : ((is_NyleMP) ? Btwxt::ExtrapolationMethod::constant
                                                           : Btwxt::ExtrapolationMethod::linear);
        grid_axes.push_back(Btwxt::GridAxis(perfGrid[iAxis],
                                            interpMethod,
                                            extrapMethod,
                                            {-DBL_MAX, DBL_MAX},
                                            "HeatSourceT",
                                            get_courier()));
        ++iAxis;
    }
    return grid_axes;
}

void HPWH::Condenser::makePerformanceBtwxt(const std::vector<std::vector<double>>& perfGrid,
                                           const std::vector<std::vector<double>>& perfGridValues)
{
    auto grid_axes = setUpGridAxes(perfGrid);

    perfRGI = std::make_shared<Btwxt::RegularGridInterpolator>(
        grid_axes, perfGridValues, "RegularGridInterpolator", get_courier());

    /// internal function to form target vector
    std::function<std::vector<double>(double externalT_C, double condenserT_C)>
        getPerformanceTarget;

    if (perfGrid.size() > 2)
    {
        getPerformanceTarget = [this](double externalT_C, double heatSourceT_C)
        {
            return std::vector<double>(
                {externalT_C,
                 hpwh->getSetpoint() + secondaryHeatExchanger.hotSideTemperatureOffset_dC,
                 heatSourceT_C});
        };
    }
    else
    {
        getPerformanceTarget = [](double externalT_C, double heatSourceT_C) {
            return std::vector<double>({externalT_C, heatSourceT_C});
        };
    }
    evaluatePerformance =
        [&perfRGI = perfRGI, getPerformanceTarget](double externalT_C, double heatSourceT_C)
    {
        auto target = getPerformanceTarget(externalT_C, heatSourceT_C);
        std::vector<double> result = perfRGI->get_values_at_target(target);
        Performance performance({result[0], result[1] * result[0], result[1]});
        return performance;
    };
}
