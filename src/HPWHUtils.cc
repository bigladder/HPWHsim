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
        // value = GAL_TO_L(value);
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
void HPWH::fromProto(nlohmann::json& j)
{
    auto& perf = j["performance"];

    auto& tank = perf["tank"];
    {
        auto& tankperf = tank.at("performance");
        setVolume_m3(tankperf, "volume");
        setUA_WperK(tankperf, "ua");
    }

    auto& heat_source_configs = perf.at("heat_source_configurations");
    {
        for (auto& heat_source_config : heat_source_configs)
        {
            auto& heat_source = heat_source_config.at("heat_source");

            if (heat_source_config.at("heat_source_type") == "RESISTANCE")
                setResistanceHeatSource(heat_source);

            if (heat_source_config.at("heat_source_type") == "CONDENSER")
                setCondenserHeatSource(heat_source);

            setTemp_K(heat_source_config, "maximum_temperature");
            setTemp_K(heat_source_config, "minimum_temperature");
            setTemp_K(heat_source_config, "maximum_setpoint");
            setTempDiff_K(heat_source_config, "hysteresis_temperature_difference");

            setHeatingLogic(heat_source_config, "turn_on_logic");
            setHeatingLogic(heat_source_config, "standby_logic");
            setHeatingLogic(heat_source_config, "shut_off_logic");
        }
    }
}
