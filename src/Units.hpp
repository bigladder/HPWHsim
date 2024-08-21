#ifndef _UNITS_
#define _UNITS_

#include <iterator>
#include <unordered_map>
#include <vector>

#include "Unity.hpp"

namespace Unity
{
/* time units */
enum class Time
{
    h,   // hours
    min, // minutes
    s    // seconds
} inline h = Time::h, min = Time::min, s = Time::s;

/* temperature units */
enum class Temp
{
    C, // celsius
    F, // fahrenheit
    K, // kelvin
} inline C = Temp::C, F = Temp::F, K = Temp::K;

/* temperature-difference units */
enum class Temp_d
{
    C, // celsius
    F, // fahrenheit
    K, // kelvin
} inline dC = Temp_d::C, dF = Temp_d::F, dK = Temp_d::K;

/* length units */
enum class Length
{
    m, // meters
    ft // feet
} inline m = Length::m, ft = Length::ft;

/* area units */
enum class Area
{
    m2, // square meters
    ft2 // square feet
} inline m2 = Area::m2, ft2 = Area::ft2;

/* volume units */
enum class Volume
{
    L,   // liters
    gal, // gallons
    m3,  // cubic meters
    ft3  // cubic feet
} inline L = Volume::L, gal = Volume::gal, m3 = Volume::m3, ft3 = Volume::ft3;

/* energy units */
enum class Energy
{
    kJ,  // kilojoules
    kWh, // kilowatt-hours
    Btu, // british thermal units
    J,   // joules
    Wh   // watt-hours
} inline kJ = Energy::kJ, kWh = Energy::kWh, Btu = Energy::Btu, J = Energy::J, Wh = Energy::Wh;

/* power units */
enum class Power
{
    kW,        // kilowatts
    Btu_per_h, // BTU per hour
    W,         // watts
    kJ_per_h,  // kilojoules per hour
} inline kW = Power::kW, Btu_per_h = Power::Btu_per_h, W = Power::W, kJ_per_h = Power::kJ_per_h;

/* flow-rate units */
enum class FlowRate
{
    L_per_s,    // liters per second
    gal_per_min // gallons per minute
} inline L_per_s = FlowRate::L_per_s, gal_per_min = FlowRate::gal_per_min;

/* UA units */
enum class UA
{
    kJ_per_hC,  // kilojoules per hour degree celsius
    Btu_per_hF, // british thermal units per hour degree Fahrenheit
    W_per_C
} inline kJ_per_hC = UA::kJ_per_hC, Btu_per_hF = UA::Btu_per_hF, W_per_C = UA::W_per_C;

/* RFactor units */
enum class RFactor
{
    m2C_per_W,
    ft2hF_per_Btu
} inline m2C_per_W = RFactor::m2C_per_W, ft2hF_per_Btu = RFactor::ft2hF_per_Btu;

/* Cp units */
enum class Cp
{
    kJ_per_C
    // Btu_per_F
} inline kJ_per_C = Cp::kJ_per_C;

/// reference transform factors
constexpr double s_per_min = 60.;            // s / min
constexpr double min_per_h = 60.;            // min / h
constexpr double in3_per_gal = 231.;         // in^3 / gal (U.S., exact)
constexpr double m_per_in = 0.0254;          // m / in (exact)
constexpr double F_per_C = 1.8;              // degF / degC
constexpr double offsetC_F = 32.;            // degF offset
constexpr double offsetC_K = 273.15;         // K offset
constexpr double kJ_per_Btu = 1.05505585262; // kJ / Btu (IT), https://www.unitconverters.net/
constexpr double L_per_m3 = 1000.;           // L / m^3

/// derived transform factors
constexpr double s_per_h = s_per_min * min_per_h;                                  // s / h
constexpr double ft_per_m = 1. / m_per_in / 12.;                                   // ft / m
constexpr double gal_per_L = 1.e-3 / in3_per_gal / m_per_in / m_per_in / m_per_in; // gal / L
constexpr double ft3_per_L = ft_per_m * ft_per_m * ft_per_m / 1000.;               // ft^3 / L

/// transform maps
template <>
inline Scaler<Time>::ScaleMap Scaler<Time>::scaleMap(s, {{min, 1. / s_per_min}, {h, 1. / s_per_h}});

template <>
inline Scaler<Length>::ScaleMap Scaler<Length>::scaleMap(m, {{ft, ft_per_m}});

template <>
inline Scaler<Temp_d>::ScaleMap Scaler<Temp_d>::scaleMap(dC, {{dF, F_per_C}, {dK, 1.}});

template <>
inline ScaleOffseter<Temp>::ScaleOffsetMap
    ScaleOffseter<Temp>::scaleOffsetMap(C, {{F, {F_per_C, offsetC_F}}, {K, {1., offsetC_K}}});

template <>
inline Scaler<Energy>::ScaleMap Scaler<Energy>::scaleMap(
    kJ, {{kWh, scale(s, h)}, {Btu, 1. / kJ_per_Btu}, {J, 1000.}, {Wh, 1000. * scale(s, h)}});

template <>
inline Scaler<Power>::ScaleMap Scaler<Power>::scaleMap(
    kW, {{Btu_per_h, scale(kJ, Btu) / scale(s, h)}, {W, 1000.}, {kJ_per_h, 1. / scale(s, h)}});

template <>
inline Scaler<Area>::ScaleMap Scaler<Area>::scaleMap(m2,
                                                     {{ft2, Scale(std::pow(scale(m, ft), 2.))}});

template <>
inline Scaler<Volume>::ScaleMap
    Scaler<Volume>::scaleMap(L, {{gal, gal_per_L}, {m3, 1. / L_per_m3}, {ft3, ft3_per_L}});

template <>
inline Scaler<UA>::ScaleMap Scaler<UA>::scaleMap(kJ_per_hC,
                                                 {{Btu_per_hF, scale(kJ, Btu) * scale(dC, dF)},
                                                  {W_per_C, scale(kJ_per_h, W)}});

template <>
inline Scaler<RFactor>::ScaleMap Scaler<RFactor>::scaleMap(
    m2C_per_W, {{ft2hF_per_Btu, scale(m2, ft2) * scale(dC, dF) / scale(W, Btu_per_h)}});

template <>
inline Scaler<FlowRate>::ScaleMap
    Scaler<FlowRate>::scaleMap(L_per_s, {{gal_per_min, scale(L, gal) / scale(s, min)}});

template <>
inline Scaler<Cp>::ScaleMap Scaler<Cp>::scaleMap(kJ_per_C, {});

/// units-values partial specializations
template <Time units>
using TimeVal = ScaleVal<Time, units>;
template <Temp units>
using TempVal = ScaleOffsetVal<Temp, units>;
template <Temp_d units>
using Temp_dVal = ScaleVal<Temp_d, units>;
template <Energy units>
using EnergyVal = ScaleVal<Energy, units>;
template <Power units>
using PowerVal = ScaleVal<Power, units>;
template <Length units>
using LengthVal = ScaleVal<Length, units>;
template <Area units>
using AreaVal = ScaleVal<Area, units>;
template <Volume units>
using VolumeVal = ScaleVal<Volume, units>;
template <FlowRate units>
using FlowRateVal = ScaleVal<FlowRate, units>;
template <UA units>
using UAVal = ScaleVal<UA, units>;
template <UA units>
using RFactorVal = ScaleVal<UA, units>;

/// units-pair partial specialization
template <Time units0, Time units1>
using TimePair = ScalePair<Time, units0, units1>;

/// units-vectors partial specialization
template <Time units>
using TimeVect = ScaleVect<Time, units>;
template <Temp units>
using TempVect = ScaleOffsetVect<Temp, units>;
template <Energy units>
using EnergyVect = ScaleVect<Energy, units>;
template <Power units>
using PowerVect = ScaleVect<Power, units>;

} // namespace Unity

namespace Units = Unity;

#endif
