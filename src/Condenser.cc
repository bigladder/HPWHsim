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
    , evaluatePerformance({})
    , inputPowerScale(1.)
    , COP_scale(1.)
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

    perfGrid = cond_in.perfGrid;
    perfGridValues = cond_in.perfGridValues;
    perfRGI = cond_in.perfRGI;
    perfPolySet = cond_in.perfPolySet;
    useBtwxtGrid = cond_in.useBtwxtGrid;
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

/// create RGI using grid and model data; assign evaluate-performance function
void HPWH::Condenser::makePerformanceBtwxt()
{
    auto is_integrated = (configuration != CONFIG_EXTERNAL);
    auto is_Mitsubishi = (hpwh->model == MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP);
    auto is_NyleMP =
        ((MODELS_NyleC60A_MP <= hpwh->model) && (hpwh->model <= MODELS_NyleC250A_C_MP));

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
        auto extrapMethod = (is_Mitsubishi)
                                ? Btwxt::ExtrapolationMethod::linear
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

    perfRGI = std::make_shared<Btwxt::RegularGridInterpolator>(
        grid_axes, perfGridValues, "RegularGridInterpolator", get_courier());

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

    evaluatePerformance = [this](double externalT_C, double heatSourceT_C)
    {
        auto target = getPerformanceTarget(externalT_C, heatSourceT_C);
        std::vector<double> result = perfRGI->get_values_at_target(target);
        Performance performance({result[0], result[1] * result[0], result[1]});
        return performance;
    };

    useBtwxtGrid = true;
}

void makePerformancePolySet()
{
    getPerformanceTarget = [](double externalT_C, double heatSourceT_C) {
        return std::vector<double>({externalT_C, heatSourceT_C});
    };

    evaluatePerformance = [this](double externalT_C, double heatSourceT_C)
    {
        Performance performance = {0., 0., 0.};

        size_t i_prev = 0;
        size_t i_next = 1;

        double externalT_F = C_TO_F(externalT_C);
        double heatSourceT_F = C_TO_F(heatSourceT_C);
        for (size_t i = 0; i < perfPolySet.size(); ++i)
        {
            if (externalT_F < perfPolySet[i].T_F)
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
                if (i == perfPolySet.size() - 1)
                {
                    i_prev = i - 1;
                    i_next = i;
                    break;
                }
            }
        }

        // Calculate COP and Input Power at each of the two reference temperatures
        double COP_T1 = perfPolySet[i_prev].COP_coeffs[0];
        COP_T1 += perfPolySet[i_prev].COP_coeffs[1] * heatSourceT_F;
        COP_T1 += perfPolySet[i_prev].COP_coeffs[2] * heatSourceT_F * heatSourceT_F;

        double COP_T2 = perfPolySet[i_next].COP_coeffs[0];
        COP_T2 += perfPolySet[i_next].COP_coeffs[1] * heatSourceT_F;
        COP_T2 += perfPolySet[i_next].COP_coeffs[2] * heatSourceT_F * heatSourceT_F;

        double inputPower_T1_W = perfPolySet[i_prev].inputPower_coeffs[0];
        inputPower_T1_W += perfPolySet[i_prev].inputPower_coeffs[1] * heatSourceT_F;
        inputPower_T1_W +=
            perfPolySet[i_prev].inputPower_coeffs[2] * heatSourceT_F * heatSourceT_F;

        double inputPower_T2_W = perfPolySet[i_next].inputPower_coeffs[0];
        inputPower_T2_W += perfPolySet[i_next].inputPower_coeffs[1] * heatSourceT_F;
        inputPower_T2_W +=
            perfPolySet[i_next].inputPower_coeffs[2] * heatSourceT_F * heatSourceT_F;

        // Interpolate to get COP and input power at the current ambient temperature
        Condenser::linearInterp(performance.cop,
                                externalT_F,
                                perfPolySet[i_prev].T_F,
                                perfPolySet[i_next].T_F,
                                COP_T1,
                                COP_T2);
        linearInterp(performance.inputPower_W,
                     externalT_F,
                     perfPolySet[i_prev].T_F,
                     perfPolySet[i_next].T_F,
                     inputPower_T1_W,
                     inputPower_T2_W);
        performance.outputPower_W = performance.cop * performance.inputPower_W;
        return performance;
    };

    useBtwxtGrid = false;
}

void HPWH::Condenser::makeTier3_performance()
{
    perfPolySet = {{50., {187.064124, 1.939747, 0.}, {5.22288834, -0.0243008, 0.}},
                   {67.5, {152.9195905, 2.476598, 0.}, {6.643934986, -0.032373288, 0.}},
                   {95., {99.263895, 3.320221, 0.}, {8.87700829, -0.0450586, 0.}}};

    makePerformancePolySet();
}

void HPWH::Condenser::makeTier4_performance()
{
    perfPolySet = {{50., {126.9, 2.215, 0.}, {6.931, -0.03395, 0.}},
                   {67.5, {116.6, 2.467, 0.}, {8.833, -0.04431, 0.}},
                   {95., {100.4, 2.863, 0.}, {11.822, -0.06059, 0.}}};

    makePerformancePolySet();
}

int HPWH::Condenser::getAmbientT_index(double ambientT_C)
{
    int nPerfPts = static_cast<int>(perfPolySet.size());
    int i0 = 0, i1 = 0;
    for (auto& perfPoint : perfPolySet)
    {
        if (C_TO_F(ambientT_C) < perfPoint.T_F)
            break;
        i0 = i1;
        ++i1;
    }
    double ratio = 0.;
    if ((i1 > i0) && (i1 < nPerfPts))
    {
        ratio = (C_TO_F(ambientT_C) - perfPolySet[i0].T_F) /
                (perfPolySet[i1].T_F - perfPolySet[i0].T_F);
    }
    return (ratio < 0.5) ? i0 : i1;
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

        perfGrid.reserve(3);
        // order based on MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP
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
        makePerformanceBtwxt();
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

        auto& grid_variables = perf_map.grid_variables;
        perfGrid.reserve(3);
        // order based on MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP
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

        makePerformanceBtwxt();
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
        envTemps_K.push_back(C_TO_K(minT));
        envTemps_K.push_back(C_TO_K(maxT));
        trimGridVector(envTemps_K, C_TO_K(minT), C_TO_K(maxT));

        checkTo(envTemps_K,
                grid_vars.evaporator_environment_dry_bulb_temperature_is_set,
                grid_vars.evaporator_environment_dry_bulb_temperature);

        ++iElem;

        //
        std::vector<double> heatSourceTemps_K = {};
        heatSourceTemps_K.reserve(perfGrid[iElem].size());
        for (auto T : perfGrid[iElem])
        {
            heatSourceTemps_K.push_back(C_TO_K(T));
        }

        checkTo(heatSourceTemps_K,
                grid_vars.heat_source_temperature_is_set,
                grid_vars.heat_source_temperature);

        map.grid_variables_is_set = true;

        //
        auto& lookup_vars = map.lookup_variables;

        std::size_t nVals =
            envTemps_K.size() * heatSourceTemps_K.size(); // perfGridValues[0].size();
        std::vector<double> inputPowers_W(nVals), heatingCapacities_W(nVals);
        std::size_t i = 0;
        for (auto& envTemp_K : envTemps_K)
            for (auto& heatSourceTemp_K : heatSourceTemps_K)
            {
                std::vector target = {K_TO_C(envTemp_K), K_TO_C(heatSourceTemp_K)};
                std::vector<double> result = perfRGI->get_values_at_target(target);

                inputPowers_W[i] = result[0];
                heatingCapacities_W[i] = result[1] * inputPowers_W[i];
                ++i;
            }

        checkTo(inputPowers_W, lookup_vars.input_power_is_set, lookup_vars.input_power);
        checkTo(
            heatingCapacities_W, lookup_vars.heating_capacity_is_set, lookup_vars.heating_capacity);

        map.lookup_variables_is_set = true;
        perf.performance_map_is_set = true;
    }
    else // convert legacy IHPWH map to grid
    {
        std::vector<std::vector<double>> tempGrid = {};
        std::vector<std::vector<double>> tempGridValues = {};

        makeGridFromPolySet(tempGrid, tempGridValues);

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

        map.grid_variables_is_set = true;
        map.lookup_variables_is_set = true;
        perf.performance_map_is_set = true;
    }
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
            envTemps_K.reserve(perfGrid.size());
            for (auto T_C : perfGrid[iElem])
            {
                envTemps_K.push_back(C_TO_K(T_C));
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
            for (auto T_C : perfGrid[iElem])
            {
                outletTemps_K.push_back(C_TO_K(T_C));
            }
            checkTo(outletTemps_K,
                    grid_vars.condenser_leaving_temperature_is_set,
                    grid_vars.condenser_leaving_temperature);

            ++iElem;
            nVals *= outletTemps_K.size();
        }
        {
            heatSourceTemps_K.reserve(perfGrid[iElem].size());
            for (auto T_C : perfGrid[iElem])
            {
                heatSourceTemps_K.push_back(C_TO_K(T_C));
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
                    std::vector<double> target = {
                        K_TO_C(envTemp_K), K_TO_C(outletTemp_K), K_TO_C(heatSourceTemp_K)};
                    std::vector<double> result = perfRGI->get_values_at_target(target);

                    inputPowers_W[i] = result[0];
                    heatingCapacities_W[i] = result[1] * inputPowers_W[i];
                    ++i;
                }

        auto& lookup_vars = map.lookup_variables;
        checkTo(inputPowers_W, lookup_vars.input_power_is_set, lookup_vars.input_power);
        checkTo(
            heatingCapacities_W, lookup_vars.heating_capacity_is_set, lookup_vars.heating_capacity);

        map.grid_variables_is_set = true;
        map.lookup_variables_is_set = true;
        perf.performance_map_is_set = true;
    }
    else // convert to grid
    {
        std::vector<std::vector<double>> tempGrid = {};
        std::vector<std::vector<double>> tempGridValues = {};

        makeGridFromPolySet(tempGrid, tempGridValues);

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

        map.grid_variables_is_set = true;
        map.lookup_variables_is_set = true;
        perf.performance_map_is_set = true;
    }
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
        double cap_kJ = (performance.outputPower_W / 1000.) * (60. * minutesToRun);

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
    energyInput_kWh += (performance.inputPower_W / 1000.) * (runtime_min / min_per_hr);
    energyOutput_kWh += (performance.outputPower_W / 1000.) * (runtime_min / min_per_hr);
}

HPWH::Condenser::Performance HPWH::Condenser::getPerformance(double externalT_C,
                                                             double condenserT_C) const
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
        performance.inputPower_W += 1000. * resDefrost.inputPwr_kW;
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

        double heatingPower_kW = tempPerformance.outputPower_W / 1000.;

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

        double heatingPower_kW = tempPerformance.outputPower_W / 1000.;

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

/*static*/
void HPWH::Condenser::linearInterp(
    double& ynew, double xnew, double x0, double x1, double y0, double y1)
{
    ynew = y0 + (xnew - x0) * (y1 - y0) / (x1 - x0);
}

void HPWH::Condenser::sortPerformancePolySet()
{
    std::sort(perfPolySet.begin(),
              perfPolySet.end(),
              [](const PerformancePoly& a, const PerformancePoly& b) -> bool
              { return a.T_F < b.T_F; });
}

//-----------------------------------------------------------------------------
///	@brief	compute a grid representation for performance map of this condenser
//-----------------------------------------------------------------------------
void HPWH::Condenser::makeGridFromPolySet(std::vector<std::vector<double>>& tempGrid,
                                          std::vector<std::vector<double>>& tempGridValues) const
{
    std::size_t nEnvTempsOrig = perfPolySet.size();
    if (nEnvTempsOrig < 1)
        return;

    tempGridValues.reserve(2); // inputPower, cop

    std::vector<double> envTemps_K = {};
    std::vector<double> heatSourceTemps_K = {};
    std::vector<double> outletTemps_K = {};

    // fill envT axis
    double maxPowerCurvature = 0.; // curvature used to determine # of points
    double maxCOPCurvature = 0.;
    envTemps_K.reserve(nEnvTempsOrig + 2); // # of map entries, plus endpoints
    envTemps_K.push_back(C_TO_K(minT));
    for (auto& perfPoly : perfPolySet)
    {
        if ((F_TO_C(perfPoly.T_F) > minT) && (F_TO_C(perfPoly.T_F) < maxT))
        {
            envTemps_K.push_back(C_TO_K(F_TO_C(perfPoly.T_F)));
            double magPowerCurvature = fabs(perfPoly.inputPower_coeffs[2]);
            double magCOPCurvature = fabs(perfPoly.COP_coeffs[2]);
            maxPowerCurvature =
                magPowerCurvature > maxPowerCurvature ? magPowerCurvature : maxPowerCurvature;
            maxCOPCurvature = magCOPCurvature > maxCOPCurvature ? magCOPCurvature : maxCOPCurvature;
        }
    }
    envTemps_K.push_back(C_TO_K(maxT));
    tempGrid.push_back(envTemps_K);

    // fill outletT axis, if external (only used by Sanco)
    if (configuration == COIL_CONFIG::CONFIG_EXTERNAL)
    {
        tempGrid.push_back(
            {C_TO_K(hpwh->setpoint_C + secondaryHeatExchanger.hotSideTemperatureOffset_dC)});
    }

    const double minHeatSourceTemp_C = 0.; // none specified in HPWH
    const double maxHeatSourceTemp_C = maxSetpoint_C;
    const double heatSourceTempRange_dC = maxHeatSourceTemp_C - minHeatSourceTemp_C;
    const double heatSourceTempRangeRef_dC = 100. - 0.;
    const double rangeFac = heatSourceTempRange_dC / heatSourceTempRangeRef_dC;

    constexpr double minVals = 2.; // retain endpoints only, if no curvature

    // relate to reference values (from AOSmithPHPT60)
    constexpr double refPowerVals = 11.;
    constexpr double refCOP_vals = 11.;
    constexpr double refPowerCurvature = 0.0176;
    constexpr double refCOP_curvature = 0.0002;

    // find # of values needed along heat-sourceT axis for inputPower and COP;
    // take the larger one
    auto nPowerVals = static_cast<std::size_t>(
        rangeFac * (maxPowerCurvature / refPowerCurvature) * (refPowerVals - minVals) + minVals);
    auto nCOP_vals = static_cast<std::size_t>(
        rangeFac * (maxCOPCurvature / refCOP_curvature) * (refCOP_vals - minVals) + minVals);
    std::size_t nVals = std::max(nPowerVals, nCOP_vals);

    // fill heat-sourceT axis
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

    // fill grid values
    std::size_t nTotVals = envTemps_K.size() * heatSourceTemps_K.size();
    std::vector<double> inputPowers_W(nTotVals), heatingCapacities_W(nTotVals);
    std::size_t i = 0;
    for (auto& envTemp_K : envTemps_K)
        for (auto& heatSourceTemp_K : heatSourceTemps_K)
        {
            if (outletTemps_K.empty())
            {
                auto performance = evaluatePerformance(K_TO_C(envTemp_K), K_TO_C(heatSourceTemp_K));
                inputPowers_W[i] = performance.inputPower_W;
                heatingCapacities_W[i] = performance.outputPower_W;
                ++i;
            }
        }

    tempGridValues.push_back(inputPowers_W);
    tempGridValues.push_back(heatingCapacities_W);
}
