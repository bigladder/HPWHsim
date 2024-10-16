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
        setPerformancePoints(heat_source_perf, "performance_points");
        setPerformanceMap(heat_source_perf, "performance_map");
        setPower_W(heat_source_perf, "standby_power");
    }
    catch (...)
    {
    }
}

/*static*/
void HPWH::to_json(const data_model::rsintegratedwaterheater_ns::RSINTEGRATEDWATERHEATER& rswh,
                   nlohmann::json& j)
{
    nlohmann::json j_metadata;
    j_metadata["schema"] = "RSINTEGRATEDWATERHEATER";
    j["metadata"] = j_metadata;

    auto& perf = rswh.performance;
    nlohmann::json j_perf;

    auto& tank = perf.tank;

    if (perf.fixed_setpoint_is_set && perf.fixed_setpoint)
    {
        j_perf["fixed_setpoint"] = true;
    }

    nlohmann::json j_tank;
    to_json(tank, j_tank);
    j_perf["tank"] = j_tank;

    nlohmann::json j_heat_source_configs;
    auto& heat_source_configs = perf.heat_source_configurations;
    for (auto& heat_source_config : heat_source_configs)
    {
        nlohmann::json j_heat_source_config;
        j_heat_source_config["label"] = heat_source_config.label;
        j_heat_source_config["heat_distribution"] = heat_source_config.heat_distribution;

        auto& heat_source = heat_source_config.heat_source;
        nlohmann::json j_heat_source;

        if (heat_source_config.heat_source_type ==
            data_model::rsintegratedwaterheater_ns::HeatSourceType::RESISTANCE)
        {
            j_heat_source_config["heat_source_type"] = "RESISTANCE";
            auto hs = reinterpret_cast<
                data_model::rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE*>(
                heat_source.get());
            to_json(*hs, j_heat_source);
        }
        if (heat_source_config.heat_source_type ==
            data_model::rsintegratedwaterheater_ns::HeatSourceType::CONDENSER)
        {
            j_heat_source_config["heat_source_type"] = "CONDENSER";
            auto hs = reinterpret_cast<
                data_model::rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE*>(
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

        if (heat_source_config.backup_heat_source_label_is_set)
        {
            j_heat_source_config["backup_heat_source_label"] =
                heat_source_config.backup_heat_source_label;
        }

        if (heat_source_config.followed_by_heat_source_label_is_set)
        {
            j_heat_source_config["followed_by_heat_source_label"] =
                heat_source_config.followed_by_heat_source_label;
        }

        if (heat_source_config.companion_heat_source_label_is_set)
        {
            j_heat_source_config["companion_heat_source_label"] =
                heat_source_config.companion_heat_source_label;
        }

        if (heat_source_config.hysteresis_temperature_difference_is_set)
        {
            j_heat_source_config["hysteresis_temperature_difference"] =
                heat_source_config.hysteresis_temperature_difference;
        }

        if (heat_source_config.maximum_temperature_is_set)
        {
            j_heat_source_config["maximum_temperature"] = heat_source_config.maximum_temperature;
        }

        if (heat_source_config.minimum_temperature_is_set)
        {
            j_heat_source_config["minimum_temperature"] = heat_source_config.minimum_temperature;
        }

        if (heat_source_config.maximum_setpoint_is_set)
        {
            j_heat_source_config["maximum_setpoint"] = heat_source_config.maximum_setpoint;
        }

        if (heat_source_config.is_vip_is_set)
        {
            j_heat_source_config["is_vip"] = heat_source_config.is_vip;
        }

        j_heat_source_configs.push_back(j_heat_source_config);
    }

    j_perf["heat_source_configurations"] = j_heat_source_configs;
    j["performance"] = j_perf;
}

/*static*/
void HPWH::to_json(const data_model::rstank_ns::RSTANK& rstank, nlohmann::json& j)
{
    nlohmann::json j_metadata;
    j_metadata["schema"] = "RSTANK";
    j["metadata"] = j_metadata;

    auto& perf = rstank.performance;
    nlohmann::json j_perf;
    j_perf["number_of_nodes"] = perf.number_of_nodes;
    j_perf["volume"] = perf.volume;
    j_perf["ua"] = perf.ua;
    j_perf["fittings_ua"] = perf.fittings_ua;
    j_perf["bottom_fraction_of_tank_mixing_on_draw"] = perf.bottom_fraction_of_tank_mixing_on_draw;
    j_perf["fixed_volume"] = perf.fixed_volume;

    if (perf.heat_exchanger_effectiveness_is_set)
    {
        j_perf["heat_exchanger_effectiveness"] = perf.heat_exchanger_effectiveness;
    }
    j["performance"] = j_perf;
}

/*static*/
void HPWH::to_json(
    const data_model::rscondenserwaterheatsource_ns::RSCONDENSERWATERHEATSOURCE& rshs,
    nlohmann::json& j)
{
    nlohmann::json j_metadata;
    j_metadata["schema"] = "RSCONDENSERWATERHEATSOURCE";
    j["metadata"] = j_metadata;

    auto& perf = rshs.performance;
    nlohmann::json j_perf;
    if (perf.performance_map_is_set)
    {
        nlohmann::json j_perf_map;

        auto& grid_vars = perf.performance_map.grid_variables;
        nlohmann::json j_grid_vars;

        j_grid_vars["evaporator_environment_temperature"] =
            grid_vars.evaporator_environment_temperature;
        j_grid_vars["heat_source_temperature"] = grid_vars.heat_source_temperature;
        j_perf_map["grid_variables"] = j_grid_vars;

        auto& lookup_vars = perf.performance_map.lookup_variables;
        nlohmann::json j_lookup_vars;
        j_lookup_vars["input_power"] = lookup_vars.input_power;
        j_lookup_vars["cop"] = lookup_vars.cop;
        j_perf_map["lookup_variables"] = j_lookup_vars;

        j_perf["performance_map"] = j_perf_map;
    }
    if (perf.coil_configuration_is_set)
    {
        switch (perf.coil_configuration)
        {
        case data_model::rscondenserwaterheatsource_ns::CoilConfiguration::WRAPPED:
        {
            j_perf["coil_configuration"] = "WRAPPED";
            break;
        }
        case data_model::rscondenserwaterheatsource_ns::CoilConfiguration::SUBMERGED:
        {
            j_perf["coil_configuration"] = "SUBMERGED";
            break;
        }
        case data_model::rscondenserwaterheatsource_ns::CoilConfiguration::EXTERNAL:
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

    j["performance"] = j_perf;
}

/*static*/
void HPWH::to_json(
    const data_model::rsresistancewaterheatsource_ns::RSRESISTANCEWATERHEATSOURCE& rshs,
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
void HPWH::to_json(const data_model::rsintegratedwaterheater_ns::SoCBasedHeatingLogic& socLogic,
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
void HPWH::to_json(const data_model::rsintegratedwaterheater_ns::TempBasedHeatingLogic& tempLogic,
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
    if (tempLogic.logic_distribution_is_set)
    {
        j["logic_distribution"] = tempLogic.logic_distribution;
    }
    if (tempLogic.activates_standby_is_set)
    {
        j["activates_standby"] = true;
    }
    if (tempLogic.tank_node_specification_is_set)
    {
        switch (tempLogic.tank_node_specification)
        {
        case data_model::rsintegratedwaterheater_ns::TankNodeSpecification::TOP_NODE:
        {
            j["tank_node_specification"] = "TOP_NODE";
            break;
        }
        case data_model::rsintegratedwaterheater_ns::TankNodeSpecification::BOTTOM_NODE:
        {
            j["tank_node_specification"] = "BOTTOM_NODE";
        }
        default:
        {
        }
        }
    }
}

/*static*/
void HPWH::to_json(const data_model::rsintegratedwaterheater_ns::HeatingLogic& heating_logic,
                   nlohmann::json& j)
{
    if (heating_logic.comparison_type_is_set)
    {
        switch (heating_logic.comparison_type)
        {
        case data_model::rsintegratedwaterheater_ns::ComparisonType::GREATER_THAN:
        {
            j["comparison_type"] = "GREATER_THAN";
            break;
        }
        case data_model::rsintegratedwaterheater_ns::ComparisonType::LESS_THAN:
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
        case data_model::rsintegratedwaterheater_ns::HeatingLogicType::SOC_BASED:
        {
            j["heating_logic_type"] = "SOC_BASED";
            if (heating_logic.heating_logic_is_set)
            {
                nlohmann::json j_logic;
                to_json(*reinterpret_cast<
                            data_model::rsintegratedwaterheater_ns::SoCBasedHeatingLogic*>(
                            heating_logic.heating_logic.get()),
                        j_logic);
                j["heating_logic"] = j_logic;
            }
            break;
        }
        case data_model::rsintegratedwaterheater_ns::HeatingLogicType::TEMP_BASED:
        {
            j["heating_logic_type"] = "TEMP_BASED";
            if (heating_logic.heating_logic_is_set)
            {
                nlohmann::json j_logic;
                to_json(*reinterpret_cast<
                            data_model::rsintegratedwaterheater_ns::TempBasedHeatingLogic*>(
                            heating_logic.heating_logic.get()),
                        j_logic);
                j["heating_logic"] = j_logic;
            }
            break;
        }
        case data_model::rsintegratedwaterheater_ns::HeatingLogicType::UNKNOWN:
            break;
        }
    }
}

/*static*/
void HPWH::HeatSource::linearInterp(
    double& ynew, double xnew, double x0, double x1, double y0, double y1) const
{
    ynew = y0 + (xnew - x0) * (y1 - y0) / (x1 - x0);
}

/*static*/
void HPWH::HeatSource::regressedMethod(
    double& ynew, std::vector<double>& coefficents, double x1, double x2, double x3)
{
    ynew = coefficents[0] + coefficents[1] * x1 + coefficents[2] * x2 + coefficents[3] * x3 +
           coefficents[4] * x1 * x1 + coefficents[5] * x2 * x2 + coefficents[6] * x3 * x3 +
           coefficents[7] * x1 * x2 + coefficents[8] * x1 * x3 + coefficents[9] * x2 * x3 +
           coefficents[10] * x1 * x2 * x3;
}

/*static*/
void HPWH::HeatSource::regressedMethodMP(double& ynew,
                                         std::vector<double>& coefficents,
                                         double x1,
                                         double x2)
{
    // Const Tair Tin Tair2 Tin2 TairTin
    ynew = coefficents[0] + coefficents[1] * x1 + coefficents[2] * x2 + coefficents[3] * x1 * x1 +
           coefficents[4] * x2 * x2 + coefficents[5] * x1 * x2;
}

/*static*/
void HPWH::HeatSource::getCapacityFromMap(std::vector<HPWH::HeatSource::PerfPoint>& perfMap,
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
void HPWH::HeatSource::convertMapToGrid(const std::vector<HPWH::HeatSource::PerfPoint>& perfMap,
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
            getCapacityFromMap(K_TO_C(envTemp_K), K_TO_C(heatSourceTemp_K), input_BTUperHr, cop);
            inputPowers_W.push_back(1000. * BTUperH_TO_KW(input_BTUperHr));
            cops.push_back(cop);
        }

    tempGridValues.push_back(inputPowers_W);
    tempGridValues.push_back(cops);
}
