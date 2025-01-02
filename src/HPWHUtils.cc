/*
 * Implementation of static HPWH utility functions
 */

#include "HPWH.hh"
#include "HPWHHeatSource.hh"
#include <fmt/format.h>

#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>

/*static*/
double HPWH::getResampledValue(const std::vector<double>& sampleValues,
                               double beginFraction,
                               double endFraction)
{
    if (beginFraction > endFraction)
        std::swap(beginFraction, endFraction);
    if (beginFraction < 0.)
        beginFraction = 0.;
    if (endFraction > 1.)
        endFraction = 1.;

    double nNodes = static_cast<double>(sampleValues.size());
    auto beginIndex = static_cast<std::size_t>(beginFraction * nNodes);

    double previousFraction = beginFraction;
    double nextFraction = previousFraction;

    double totValueWeight = 0.;
    double totWeight = 0.;
    for (std::size_t index = beginIndex; nextFraction < endFraction; ++index)
    {
        nextFraction = static_cast<double>(index + 1) / nNodes;
        if (nextFraction > endFraction)
        {
            nextFraction = endFraction;
        }
        double weight = nextFraction - previousFraction;
        totValueWeight += weight * sampleValues[index];
        totWeight += weight;
        previousFraction = nextFraction;
    }
    double resampled_value = 0.;
    if (totWeight > 0.)
        resampled_value = totValueWeight / totWeight;
    return resampled_value;
}

/*static*/
void HPWH::resample(std::vector<double>& values, const std::vector<double>& sampleValues)
{
    if (sampleValues.empty())
        return;
    double actualSize = static_cast<double>(values.size());
    double sizeRatio = static_cast<double>(sampleValues.size()) / actualSize;
    auto binSize = static_cast<std::size_t>(1. / sizeRatio);
    double beginFraction = 0., endFraction;
    std::size_t index = 0;
    while (index < actualSize)
    {
        auto value = static_cast<double>(index);
        auto sampleIndex = static_cast<std::size_t>(floor(value * sizeRatio));
        if (sampleIndex + 1. < (value + 1.) * sizeRatio)
        { // General case: no binning possible
            endFraction = static_cast<double>(index + 1) / actualSize;
            values[index] = getResampledValue(sampleValues, beginFraction, endFraction);
            ++index;
        }
        else
        { // Special case: direct copy a single value to a bin
            std::size_t beginIndex = index;
            std::size_t adjustedBinSize = binSize;
            if (binSize > 1)
            { // Find beginning of bin and number to copy
                beginIndex = static_cast<std::size_t>(ceil(sampleIndex / sizeRatio));
                adjustedBinSize = static_cast<std::size_t>(floor((sampleIndex + 1) / sizeRatio) -
                                                           ceil(sampleIndex / sizeRatio));
            }
            std::fill_n(values.begin() + beginIndex, adjustedBinSize, sampleValues[sampleIndex]);
            index = beginIndex + adjustedBinSize;
            endFraction = static_cast<double>(index) / actualSize;
        }
        beginFraction = endFraction;
    }
}

/*static*/
void HPWH::resampleExtensive(std::vector<double>& values, const std::vector<double>& sampleValues)
{
    resample(values, sampleValues);
    double scale = static_cast<double>(sampleValues.size()) / static_cast<double>(values.size());
    for (auto& value : values)
        value *= scale;
}

/*static*/
double HPWH::expitFunc(double x, double offset)
{
    double val;
    val = 1 / (1 + exp(x - offset));
    return val;
}

/*static*/
void HPWH::normalize(std::vector<double>& distribution)
{
    size_t N = distribution.size();

    bool normalization_needed = true;

    // Need to renormalize if negligible elements are zeroed.
    while (normalization_needed)
    {
        normalization_needed = false;
        double sum_tmp = 0.;
        for (size_t i = 0; i < N; i++)
        {
            sum_tmp += distribution[i];
        }
        if (sum_tmp > 0.)
        {
            for (size_t i = 0; i < N; i++)
            {
                distribution[i] /= sum_tmp;
                // this gives a very slight speed improvement (milliseconds per simulated year)
                if (distribution[i] < HPWH::TOL_MINVALUE)
                {
                    if (distribution[i] > 0.)
                    {
                        normalization_needed = true;
                    }
                    distribution[i] = 0.;
                }
            }
        }
        else
        {
            for (size_t i = 0; i < N; i++)
            {
                distribution[i] = 0.;
            }
        }
    }
}

/*static*/
int HPWH::findLowestNode(const std::vector<double>& nodeDist, const int numTankNodes)
{
    int lowest = 0;
    const int distSize = static_cast<int>(nodeDist.size());
    double nodeRatio = static_cast<double>(numTankNodes) / distSize;

    for (auto j = 0; j < distSize; ++j)
    {
        if (nodeDist[j] > 0.)
        {
            lowest = static_cast<int>(nodeRatio * j);
            break;
        }
    }

    return lowest;
}

/*static*/
double HPWH::findShrinkageT_C(const std::vector<double>& nodeDist)
{
    double alphaT_C = 1., betaT_C = 2.;
    double condentropy = 0.;
    for (std::size_t iNode = 0; iNode < nodeDist.size(); ++iNode)
    {
        double dist = nodeDist[iNode];
        if (dist > 0.)
        {
            condentropy -= dist * log(dist);
        }
    }
    // condentropy shifts as ln(# of condensity nodes)
    double size_factor = static_cast<double>(nodeDist.size()) / HPWH::HeatSource::CONDENSITY_SIZE;
    double standard_condentropy = condentropy - log(size_factor);

    return alphaT_C + standard_condentropy * betaT_C;
}

/*static*/
void HPWH::calcThermalDist(std::vector<double>& thermalDist,
                           const double shrinkageT_C,
                           const int lowestNode,
                           const std::vector<double>& nodeT_C,
                           const double setpointT_C)
{

    thermalDist.resize(nodeT_C.size());

    // Populate the vector of heat distribution
    double totDist = 0.;
    for (int i = 0; i < static_cast<int>(nodeT_C.size()); i++)
    {
        double dist = 0.;
        if (i >= lowestNode)
        {
            double Toffset_C = 5.0 / 1.8;   // 5 degF
            double offset = Toffset_C / 1.; // should be dimensionless
            dist = expitFunc((nodeT_C[i] - nodeT_C[lowestNode]) / shrinkageT_C, offset);
            dist *= (setpointT_C - nodeT_C[i]);
            if (dist < 0.)
                dist = 0.;
        }
        thermalDist[i] = dist;
        totDist += dist;
    }

    if (totDist > 0.)
    {
        normalize(thermalDist);
    }
    else
    {
        thermalDist.assign(thermalDist.size(), 1. / static_cast<double>(thermalDist.size()));
    }
}

/*static*/
void HPWH::scaleVector(std::vector<double>& coeffs, const double scaleFactor)
{
    if (scaleFactor != 1.)
    {
        std::transform(coeffs.begin(),
                       coeffs.end(),
                       coeffs.begin(),
                       std::bind(std::multiplies<double>(), std::placeholders::_1, scaleFactor));
    }
}

/*static*/
double HPWH::getChargePerNode(double tCold, double tMix, double tHot)
{
    if (tHot < tMix)
    {
        return 0.;
    }
    return (tHot - tCold) / (tMix - tCold);
}

//
template <typename T>
void setValue(nlohmann::json& j, std::string_view sLabel)
{
    try
    {
        auto& jp = j.at(sLabel);
        if (jp.is_object())
        {
            T value = jp.at("value");
            jp = value;
        }
    }
    catch (...)
    {
    }
}

//
template <typename T>
void setValue(nlohmann::json& j, std::string_view sLabel, std::function<T(T, const std::string&)> f)
{
    try
    {
        auto& jp = j.at(sLabel);
        if (jp.is_object())
        {
            T value = f(jp.at("value"), jp.at("units"));
            jp = value;
        }
    }
    catch (...)
    {
    }
}

//
template <typename T>
void setVector(nlohmann::json& j, std::string_view sLabel)
{
    try
    {
        auto& jp = j.at(sLabel);
        if (jp.is_object())
        {
            std::vector<T> values = {};
            for (auto& value : jp.at("value"))
                values.push_back(value);
            jp = values;
        }
    }
    catch (...)
    {
    }
}

//
template <typename T>
void setVector(nlohmann::json& j,
               std::string_view sLabel,
               std::function<T(T, const std::string&)> f)
{
    try
    {
        auto& jp = j.at(sLabel);
        if (jp.is_object())
        {
            std::vector<T> values = {};
            for (auto& value : jp.at("value"))
                values.push_back(f(value, jp.at("units")));
            jp = values;
        }
    }
    catch (...)
    {
    }
}

double convertTemp_K(double value, const std::string& units)
{
    if (units == "F")
    {
        value = F_TO_K(value);
    }
    else if (units == "C")
    {
        value = C_TO_K(value);
    }
    return value;
}

double convertTempDiff_K(double value, const std::string& units)
{
    if (units == "F")
    {
        value = dF_TO_dC(value);
    }
    else if (units == "C")
    {
    }
    return value;
}

double convertVolume_m3(double value, const std::string& units)
{
    if (units == "L")
    {
        value = value / 1000.;
    }
    else if (units == "gal")
    {
        value = GAL_TO_L(value) / 1000.;
    }
    return value;
}

double convertUA_WperK(double value, const std::string& units)
{
    if (units == "kJ_per_hC")
    {
        value = 1000. * value / 3600.;
    }
    return value;
}

double convertPower_W(double value, const std::string& units)
{
    if (units == "kW")
    {
        value = 1000. * value;
    }
    return value;
}

void setTemp_K(nlohmann::json& j, std::string_view sLabel)
{
    setValue<double>(j, sLabel, &convertTemp_K);
}

void setTempDiff_K(nlohmann::json& j, std::string_view sLabel)
{
    setValue<double>(j, sLabel, &convertTempDiff_K);
}

void setVolume_m3(nlohmann::json& j, std::string_view sLabel)
{
    setValue<double>(j, sLabel, &convertVolume_m3);
}

void setUA_WperK(nlohmann::json& j, std::string_view sLabel)
{
    setValue<double>(j, sLabel, &convertUA_WperK);
}

void setPower_W(nlohmann::json& j, std::string_view sLabel)
{
    setValue<double>(j, sLabel, &convertPower_W);
}

void setPowerCoeffs(nlohmann::json& j, std::string_view sLabel)
{
    setVector<double>(j, sLabel, &convertPower_W);
}

void setCOP_Coeffs(nlohmann::json& j, std::string_view sLabel) { setVector<double>(j, sLabel); }

void setPerformancePoints(nlohmann::json& j, std::string_view sLabel)
{
    try
    {
        auto& jp = j.at(sLabel);
        if (jp.is_array())
        {
            for (auto& point : jp)
            {
                setTemp_K(point, "heat_source_temperature");
                setPowerCoeffs(point, "input_power_coefficients");
                setCOP_Coeffs(point, "cop_coefficients");
            }
        }
    }
    catch (...)
    {
    }
}

void setPerformanceMap(nlohmann::json& j, std::string_view sLabel)
{
    try
    {
        auto& jp = j.at(sLabel);
        setPowerCoeffs(jp, "input_power_coefficients");
        setCOP_Coeffs(jp, "cop_coefficients");
    }
    catch (...)
    {
    }
}

void setTempBasedHeatingLogic(nlohmann::json& j, std::string_view sLabel)
{
    try
    {
        auto& jp = j.at(sLabel);

        setTempDiff_K(jp, "differential_temperature");
        setTemp_K(jp, "absolute_temperature");
    }
    catch (...)
    {
    }
}

void setSoC_BasedHeatingLogic(nlohmann::json& j, std::string_view sLabel)
{
    try
    {
        auto& jp = j.at(sLabel);
        setTemp_K(jp, "minimum_useful_temperature");
    }
    catch (...)
    {
    }
}

void setHeatingLogic(nlohmann::json& j, std::string_view sLabel)
{

    try
    {
        auto& jp = j.at(sLabel);
        if (jp.is_array())
        {
            for (auto& element : jp)
            {
                auto& heating_logic_type = element.at("heating_logic_type");
                if (heating_logic_type == "TEMP_BASED")
                    setTempBasedHeatingLogic(element, "heating_logic");

                if (heating_logic_type == "SOC_BASED")
                    setSoC_BasedHeatingLogic(element, "heating_logic");
            }
        }
        else
        {
            auto& heating_logic_type = jp.at("heating_logic_type");
            if (heating_logic_type == "TEMP_BASED")
                setTempBasedHeatingLogic(jp, "heating_logic");

            if (heating_logic_type == "SOC_BASED")
                setSoC_BasedHeatingLogic(jp, "heating_logic");
        }
    }
    catch (...)
    {
    }
}

void setResistanceHeatSource(nlohmann::json& j)
{
    try
    {
        auto& heat_source_perf = j.at("performance");
        setPower_W(heat_source_perf, "input_power");
    }
    catch (...)
    {
    }
}

void setCondenserHeatSource(nlohmann::json& j)
{
    try
    {
        auto& heat_source_perf = j.at("performance");
        setPerformanceMap(heat_source_perf, "performance_map");
        setPower_W(heat_source_perf, "standby_power");
    }
    catch (...)
    {
    }
}

/*static*/
void HPWH::to_json(const hpwh_data_model::hpwh_sim_input_ns::HPWHSimInput& hsi, nlohmann::json& j)
{
    nlohmann::json j_metadata;
    j_metadata["schema"] = "HPWHSimInput";
    j["metadata"] = j_metadata;

    j["number_of_nodes"] = hsi.number_of_nodes;
    j["fixed_volume"] = hsi.fixed_volume;
    j["depresses_temperature"] = hsi.depresses_temperature;

    switch (hsi.system_type)
    {
    case hpwh_data_model::hpwh_sim_input_ns::HPWHSystemType::CENTRAL:
        j["system_type"] = "CENTRAL";
        to_json(hsi.central_system, j["central_system"]);
        break;

    case hpwh_data_model::hpwh_sim_input_ns::HPWHSystemType::INTEGRATED:
        j["system_type"] = "INTEGRATED";
        to_json(hsi.integrated_system, j["integrated_system"]);
        break;

    default:
        return;
    }
}

/*static*/
void HPWH::to_json(const hpwh_data_model::rsintegratedwaterheater_ns::RSINTEGRATEDWATERHEATER& rswh,
                   nlohmann::json& j)
{
    nlohmann::json j_metadata;
    j_metadata["schema"] = "RSINTEGRATEDWATERHEATER";
    j["metadata"] = j_metadata;

    auto& perf = rswh.performance;
    nlohmann::json j_perf;

    auto& tank = perf.tank;

    nlohmann::json j_tank;
    to_json(tank, j_tank);
    j_perf["tank"] = j_tank;

    nlohmann::json j_heat_source_configs;
    auto& heat_source_configs = perf.heat_source_configurations;
    for (auto& heat_source_config : heat_source_configs)
    {
        nlohmann::json j_heat_source_config;
        j_heat_source_config["id"] = heat_source_config.id;
        j_heat_source_config["heat_distribution"] = heat_source_config.heat_distribution;

        auto& heat_source = heat_source_config.heat_source;
        nlohmann::json j_heat_source;

        if (heat_source_config.heat_source_type ==
            hpwh_data_model::heat_source_configuration_ns::HeatSourceType::RESISTANCE)
        {
            j_heat_source_config["heat_source_type"] = "RESISTANCE";
            auto hs = reinterpret_cast<
                hpwh_data_model::rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE*>(
                heat_source.get());
            to_json(*hs, j_heat_source);
        }
        if (heat_source_config.heat_source_type ==
            hpwh_data_model::heat_source_configuration_ns::HeatSourceType::CONDENSER)
        {
            j_heat_source_config["heat_source_type"] = "CONDENSER";
            auto hs = reinterpret_cast<
                hpwh_data_model::rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE*>(
                heat_source.get());
            to_json(*hs, j_heat_source);
        }
        j_heat_source_config["heat_source"] = j_heat_source;

        if (heat_source_config.turn_on_logic.size() > 0)
        {
            nlohmann::json j_turn_on_logic;
            for (auto& logic : heat_source_config.turn_on_logic)
            {
                nlohmann::json j_logic;
                to_json(logic, j_logic);
                j_turn_on_logic.push_back(j_logic);
            }
            j_heat_source_config["turn_on_logic"] = j_turn_on_logic;
        }

        if (heat_source_config.shut_off_logic.size() > 0)
        {
            nlohmann::json j_shut_off_logic;
            for (auto& logic : heat_source_config.shut_off_logic)
            {
                nlohmann::json j_logic;
                to_json(logic, j_logic);
                j_shut_off_logic.push_back(j_logic);
            }
            j_heat_source_config["shut_off_logic"] = j_shut_off_logic;
        }

        if (heat_source_config.standby_logic_is_set)
        {
            nlohmann::json j_standby_logic;
            to_json(heat_source_config.standby_logic, j_standby_logic);
            j_heat_source_config["standby_logic"] = j_standby_logic;
        }

        if (heat_source_config.backup_heat_source_id_is_set)
        {
            j_heat_source_config["backup_heat_source_id"] =
                heat_source_config.backup_heat_source_id;
        }

        if (heat_source_config.followed_by_heat_source_id_is_set)
        {
            j_heat_source_config["followed_by_heat_source_id"] =
                heat_source_config.followed_by_heat_source_id;
        }

        if (heat_source_config.companion_heat_source_id_is_set)
        {
            j_heat_source_config["companion_heat_source_id"] =
                heat_source_config.companion_heat_source_id;
        }

        j_heat_source_configs.push_back(j_heat_source_config);
    }

    j_perf["heat_source_configurations"] = j_heat_source_configs;

    if (perf.primary_heat_source_id_is_set)
    {
        j_perf["primary_heat_source_id"] = perf.primary_heat_source_id;
    }

    j["performance"] = j_perf;
}

/*static*/
void HPWH::to_json(
    const hpwh_data_model::central_water_heating_system_ns::CentralWaterHeatingSystem& cwhs,
    nlohmann::json& j)
{
    nlohmann::json j_tank;
    to_json(cwhs.tank, j_tank);
    j["tank"] = j_tank;

    nlohmann::json j_heat_source_configs;
    auto& heat_source_configs = cwhs.heat_source_configurations;
    for (auto& heat_source_config : heat_source_configs)
    {
        nlohmann::json j_heat_source_config;
        j_heat_source_config["id"] = heat_source_config.id;
        j_heat_source_config["heat_distribution"] = heat_source_config.heat_distribution;

        auto& heat_source = heat_source_config.heat_source;
        nlohmann::json j_heat_source;

        if (heat_source_config.heat_source_type ==
            hpwh_data_model::heat_source_configuration_ns::HeatSourceType::RESISTANCE)
        {
            j_heat_source_config["heat_source_type"] = "RESISTANCE";
            auto hs = reinterpret_cast<
                hpwh_data_model::rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE*>(
                heat_source.get());
            to_json(*hs, j_heat_source);
        }
        if (heat_source_config.heat_source_type ==
            hpwh_data_model::heat_source_configuration_ns::HeatSourceType::CONDENSER)
        {
            j_heat_source_config["heat_source_type"] = "CONDENSER";
            auto hs = reinterpret_cast<
                hpwh_data_model::rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE*>(
                heat_source.get());
            to_json(*hs, j_heat_source);
        }
        j_heat_source_config["heat_source"] = j_heat_source;

        if (heat_source_config.turn_on_logic.size() > 0)
        {
            nlohmann::json j_turn_on_logic;
            for (auto& logic : heat_source_config.turn_on_logic)
            {
                nlohmann::json j_logic;
                to_json(logic, j_logic);
                j_turn_on_logic.push_back(j_logic);
            }
            j_heat_source_config["turn_on_logic"] = j_turn_on_logic;
        }

        if (heat_source_config.shut_off_logic.size() > 0)
        {
            nlohmann::json j_shut_off_logic;
            for (auto& logic : heat_source_config.shut_off_logic)
            {
                nlohmann::json j_logic;
                to_json(logic, j_logic);
                j_shut_off_logic.push_back(j_logic);
            }
            j_heat_source_config["shut_off_logic"] = j_shut_off_logic;
        }

        if (heat_source_config.standby_logic_is_set)
        {
            nlohmann::json j_standby_logic;
            to_json(heat_source_config.standby_logic, j_standby_logic);
            j_heat_source_config["standby_logic"] = j_standby_logic;
        }

        if (heat_source_config.backup_heat_source_id_is_set)
        {
            j_heat_source_config["backup_heat_source_id"] =
                heat_source_config.backup_heat_source_id;
        }

        if (heat_source_config.followed_by_heat_source_id_is_set)
        {
            j_heat_source_config["followed_by_heat_source_id"] =
                heat_source_config.followed_by_heat_source_id;
        }

        if (heat_source_config.companion_heat_source_id_is_set)
        {
            j_heat_source_config["companion_heat_source_id"] =
                heat_source_config.companion_heat_source_id;
        }

        j_heat_source_configs.push_back(j_heat_source_config);
    }

    j["heat_source_configurations"] = j_heat_source_configs;

    if (cwhs.primary_heat_source_id_is_set)
    {
        j["primary_heat_source_id"] = cwhs.primary_heat_source_id;
    }
    if (cwhs.external_inlet_height_is_set)
    {
        j["external_inlet_height"] = cwhs.external_inlet_height;
    }
    if (cwhs.external_outlet_height_is_set)
    {
        j["external_outlet_height"] = cwhs.external_outlet_height;
    }
    if (cwhs.multipass_flow_rate_is_set)
    {
        j["multipass_flow_rate"] = cwhs.multipass_flow_rate;
    }
    if (cwhs.standard_setpoint_is_set)
    {
        j["standard_setpoint"] = cwhs.standard_setpoint;
    }
}

/*static*/
void HPWH::to_json(const hpwh_data_model::rstank_ns::RSTANK& rstank, nlohmann::json& j)
{
    nlohmann::json j_metadata;
    j_metadata["schema"] = "RSTANK";
    j["metadata"] = j_metadata;

    auto& perf = rstank.performance;
    nlohmann::json j_perf;
    j_perf["volume"] = perf.volume;
    j_perf["ua"] = perf.ua;
    j_perf["fittings_ua"] = perf.fittings_ua;
    j_perf["bottom_fraction_of_tank_mixing_on_draw"] = perf.bottom_fraction_of_tank_mixing_on_draw;

    if (perf.heat_exchanger_effectiveness_is_set)
    {
        j_perf["heat_exchanger_effectiveness"] = perf.heat_exchanger_effectiveness;
    }
    j["performance"] = j_perf;
}

/*static*/
void HPWH::to_json(
    const hpwh_data_model::rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE& rshs,
    nlohmann::json& j)
{
    nlohmann::json j_metadata;
    j_metadata["schema"] = "RSCONDENSERWATERHEATSOURCE";
    j["metadata"] = j_metadata;

    auto& perf = rshs.performance;
    nlohmann::json j_perf;

    if (perf.compressor_lockout_temperature_hysteresis_is_set)
    {
        j_perf["compressor_lockout_temperature_hysteresis"] =
            perf.compressor_lockout_temperature_hysteresis;
    }

    if (perf.maximum_temperature_is_set)
    {
        j_perf["maximum_temperature"] = perf.maximum_temperature;
    }

    if (perf.minimum_temperature_is_set)
    {
        j_perf["minimum_temperature"] = perf.minimum_temperature;
    }

    if (perf.maximum_refrigerant_temperature_is_set)
    {
        j_perf["maximum_refrigerant_temperature"] = perf.maximum_refrigerant_temperature;
    }

    if (perf.performance_map_is_set)
    {
        nlohmann::json j_perf_map;

        auto& grid_vars = perf.performance_map.grid_variables;
        nlohmann::json j_grid_vars;

        if (grid_vars.evaporator_environment_dry_bulb_temperature_is_set)
        {
            j_grid_vars["evaporator_environment_dry_bulb_temperature"] =
                grid_vars.evaporator_environment_dry_bulb_temperature;
        }

        if (grid_vars.heat_source_temperature_is_set)
        {
            j_grid_vars["heat_source_temperature"] = grid_vars.heat_source_temperature;
        }

        if (grid_vars.outlet_temperature_is_set)
        {
            j_grid_vars["outlet_temperature"] = grid_vars.outlet_temperature;
        }
        j_perf_map["grid_variables"] = j_grid_vars;

        auto& lookup_vars = perf.performance_map.lookup_variables;
        std::vector<double> copV;
        copV.reserve(lookup_vars.input_power.size());
        for (std::size_t i = 0; i < lookup_vars.heating_capacity.size(); ++i)
            copV.push_back(lookup_vars.heating_capacity[i] / lookup_vars.input_power[i]);

        nlohmann::json j_lookup_vars;
        j_lookup_vars["input_power"] = lookup_vars.input_power;
        j_lookup_vars["heating_capacity"] = lookup_vars.heating_capacity;
        j_lookup_vars["cop"] = copV;
        j_perf_map["lookup_variables"] = j_lookup_vars;

        j_perf["performance_map"] = j_perf_map;
    }
    if (perf.coil_configuration_is_set)
    {
        switch (perf.coil_configuration)
        {
        case hpwh_data_model::rscondenserwaterheatsource_ns::CoilConfiguration::WRAPPED:
        {
            j_perf["coil_configuration"] = "WRAPPED";
            break;
        }
        case hpwh_data_model::rscondenserwaterheatsource_ns::CoilConfiguration::SUBMERGED:
        {
            j_perf["coil_configuration"] = "SUBMERGED";
            break;
        }
        case hpwh_data_model::rscondenserwaterheatsource_ns::CoilConfiguration::EXTERNAL:
        {
            j_perf["coil_configuration"] = "EXTERNAL";
            break;

        }
        default:
        {
        }
        }
    }

    if (perf.standby_power_is_set)
    {
        j_perf["standby_power"] = perf.standby_power;
    }


    if (perf.use_defrost_map_is_set)
    {
        j_perf["use_defrost_map"] = perf.use_defrost_map;
    }

    j["performance"] = j_perf;
}

/*static*/
void HPWH::to_json(
    const hpwh_data_model::rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE& rshs,
    nlohmann::json& j)
{
    nlohmann::json j_metadata;
    j_metadata["schema"] = "RSRESISTANCEWATERHEATSOURCE";
    j["metadata"] = j_metadata;

    auto& perf = rshs.performance;

    nlohmann::json j_perf;
    j_perf["input_power"] = perf.input_power;

    j["performance"] = j_perf;
}

/*static*/
void HPWH::to_json(
    const hpwh_data_model::heat_source_configuration_ns::StateOfChargeBasedHeatingLogic& socLogic,
    nlohmann::json& j)
{
    j["decision_point"] = socLogic.decision_point;
    j["minimum_useful_temperature"] = socLogic.minimum_useful_temperature;
    j["hysteresis_fraction"] = socLogic.hysteresis_fraction;
    j["hysteresis_fraction"] = socLogic.hysteresis_fraction;
    j["uses_constant_mains"] = socLogic.uses_constant_mains;
    if (socLogic.uses_constant_mains)
    {
        j["constant_mains_temperature"] = socLogic.constant_mains_temperature;
    }
}

/*static*/
void HPWH::to_json(
    const hpwh_data_model::heat_source_configuration_ns::TemperatureBasedHeatingLogic& tempLogic,
    nlohmann::json& j)
{
    if (tempLogic.absolute_temperature_is_set)
    {
        j["absolute_temperature"] = tempLogic.absolute_temperature;
    }
    else
    {
        j["differential_temperature"] = tempLogic.differential_temperature;
    }
    if (tempLogic.temperature_weight_distribution_is_set)
    {
        j["temperature_weight_distribution"] = tempLogic.temperature_weight_distribution;
    }

    if (tempLogic.standby_temperature_location_is_set)
    {
        switch (tempLogic.standby_temperature_location)
        {
        case hpwh_data_model::heat_source_configuration_ns::StandbyTemperatureLocation::TOP_OF_TANK:
        {
            j["standby_temperature_location"] = "TOP_OF_TANK";
            break;
        }
        case hpwh_data_model::heat_source_configuration_ns::StandbyTemperatureLocation::
            BOTTOM_OF_TANK:
        {
            j["standby_temperature_location"] = "BOTTOM_OF_TANK";
        }
        default:
        {
        }
        }
    }
}

/*static*/
void HPWH::to_json(const hpwh_data_model::heat_source_configuration_ns::HeatingLogic& heating_logic,
                   nlohmann::json& j)
{
    if (heating_logic.comparison_type_is_set)
    {
        switch (heating_logic.comparison_type)
        {
        case hpwh_data_model::heat_source_configuration_ns::ComparisonType::GREATER_THAN:
        {
            j["comparison_type"] = "GREATER_THAN";
            break;
        }
        case hpwh_data_model::heat_source_configuration_ns::ComparisonType::LESS_THAN:
        {
            j["comparison_type"] = "LESS_THAN";
            break;
        }
        default:
            break;
        }
    }

    if (heating_logic.heating_logic_type_is_set)
    {
        switch (heating_logic.heating_logic_type)
        {
        case hpwh_data_model::heat_source_configuration_ns::HeatingLogicType::STATE_OF_CHARGE_BASED:
        {
            j["heating_logic_type"] = "StateOfChargeBasedHeatingLogic";
            if (heating_logic.heating_logic_is_set)
            {
                nlohmann::json j_logic;
                to_json(*reinterpret_cast<hpwh_data_model::heat_source_configuration_ns::
                                              StateOfChargeBasedHeatingLogic*>(
                            heating_logic.heating_logic.get()),
                        j_logic);
                j["heating_logic"] = j_logic;
            }
            break;
        }
        case hpwh_data_model::heat_source_configuration_ns::HeatingLogicType::TEMPERATURE_BASED:
        {
            j["heating_logic_type"] = "TEMPERATURE_BASED";
            if (heating_logic.heating_logic_is_set)
            {
                nlohmann::json j_logic;
                to_json(*reinterpret_cast<hpwh_data_model::heat_source_configuration_ns::
                                              TemperatureBasedHeatingLogic*>(
                            heating_logic.heating_logic.get()),
                        j_logic);
                j["heating_logic"] = j_logic;
            }
            break;
        }
        case hpwh_data_model::heat_source_configuration_ns::HeatingLogicType::UNKNOWN:
            break;
        }
    }
}
