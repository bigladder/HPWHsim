#ifndef HPWH_UNITS_
#define HPWH_UNITS_

#include <unordered_map>
#include <vector>
#include <iterator>

// reference conversion factors
constexpr double s_per_min = 60.;            // s / min
constexpr double min_per_h = 60.;            // min / h
constexpr double in3_per_gal = 231.;         // in^3 / gal (U.S., exact)
constexpr double m_per_in = 0.0254;          // m / in (exact)
constexpr double F_per_C = 1.8;              // degF / degC
constexpr double offsetF = 32.;              // degF offset
constexpr double kJ_per_Btu = 1.05505585262; // kJ / Btu (IT), https://www.unitconverters.net/
constexpr double L_per_m3 = 1000.;           // L / m^3

// useful conversion factors
constexpr double s_per_h = s_per_min * min_per_h;                                  // s / h
constexpr double Btu_per_kJ = 1. / kJ_per_Btu;                                     // Btu / kJ
constexpr double ft_per_m = 1. / m_per_in / 12.;                                   // ft / m
constexpr double gal_per_L = 1.e-3 / in3_per_gal / m_per_in / m_per_in / m_per_in; // gal / L
constexpr double ft3_per_L = ft_per_m * ft_per_m * ft_per_m / 1000.;               // ft^3 / L

// identity
inline double ident(const double x) { return x; }

// time conversion
inline double S_TO_MIN(const double s) { return s / s_per_min; }
inline double S_TO_H(const double s) { return s / s_per_h; }

inline double MIN_TO_S(const double min) { return s_per_min * min; }
inline double H_TO_S(const double h) { return s_per_h * h; }

inline double MIN_TO_H(const double min) { return S_TO_H(MIN_TO_S(min)); }
inline double H_TO_MIN(const double h) { return S_TO_MIN(H_TO_S(h)); }

// temperature conversion
inline double dC_TO_dF(const double dC) { return F_per_C * dC; }
inline double dF_TO_dC(const double dF) { return dF / F_per_C; }

inline double C_TO_F(const double C) { return (F_per_C * C) + offsetF; }
inline double F_TO_C(const double F) { return (F - offsetF) / F_per_C; }

// energy conversion
inline double KJ_TO_KWH(const double kJ) { return kJ / s_per_h; }
inline double KJ_TO_BTU(const double kJ) { return Btu_per_kJ * kJ; }

inline double KWH_TO_KJ(const double kWh) { return kWh * s_per_h; }
inline double BTU_TO_KJ(const double Btu) { return kJ_per_Btu * Btu; }

inline double KWH_TO_BTU(const double kWh) { return KJ_TO_BTU(KWH_TO_KJ(kWh)); }
inline double BTU_TO_KWH(const double Btu) { return KJ_TO_KWH(BTU_TO_KJ(Btu)); }

// power conversion
inline double KW_TO_BTUperH(const double kW) { return Btu_per_kJ * s_per_h * kW; }
inline double KW_TO_W(const double kW) { return 1000. * kW; }
inline double KW_TO_KJperH(const double kW) { return kW * s_per_h; }

inline double BTUperH_TO_KW(const double Btu_per_h) { return kJ_per_Btu * Btu_per_h / s_per_h; }
inline double W_TO_KW(const double W) { return W / 1000.; }
inline double KJperH_TO_KW(const double kJ_per_h) { return kJ_per_h / s_per_h; }

// length conversion
inline double M_TO_FT(const double m) { return ft_per_m * m; }
inline double FT_TO_M(const double ft) { return ft / ft_per_m; }

// area conversion
inline double M2_TO_FT2(const double m2) { return (ft_per_m * ft_per_m * m2); }
inline double FT2_TO_M2(const double ft2) { return (ft2 / ft_per_m / ft_per_m); }

// volume conversion
inline double L_TO_GAL(const double L) { return gal_per_L * L; }
inline double L_TO_M3(const double L) { return L / L_per_m3; }
inline double L_TO_FT3(const double L) { return ft3_per_L * L; }

inline double GAL_TO_L(const double gal) { return gal / gal_per_L; }
inline double M3_TO_L(const double m3) { return L_per_m3 * m3; }
inline double FT3_TO_L(const double ft3) { return ft3 / ft3_per_L; }

// flow-rate conversion
inline double LPS_TO_GPM(const double lps) { return (gal_per_L * lps * s_per_min); }
inline double GPM_TO_LPS(const double gpm) { return (gpm / gal_per_L / s_per_min); }

// UA conversion
inline double KJperHC_TO_BTUperHF(const double UA_kJperhC)
{
    return Btu_per_kJ * UA_kJperhC / F_per_C;
}

inline double BTUperHF_TO_KJperHC(const double UA_BTUperhF)
{
    return F_per_C * UA_BTUperhF / Btu_per_kJ;
}

/// units specifications
namespace Units
{
/// unit-conversion utilities
enum class Mode
{
    Abs,
    Diff
};

template <typename T>
using ConversionPair = std::pair<T, T>;

using ConversionFnc =
    std::pair<std::function<double(const double)>, std::function<double(const double)>>;

template <typename T>
using Conversion = std::pair<T, ConversionFnc>;

template <typename T, Mode mode = Mode::Abs>
struct Converter
{
    struct PairHash
    {
        std::size_t operator()(const std::pair<T, T>& p) const
        {
            auto h1 = static_cast<std::size_t>(p.first);
            auto h2 = static_cast<std::size_t>(p.second);
            return h1 ^ h2;
        }
    };

    using BaseMap =
        std::unordered_map<std::pair<T, T>, std::function<double(const double)>, PairHash>;

    struct ConversionMap
    {
        BaseMap baseMap;
        ConversionMap() : baseMap() {}
        ConversionMap(const std::vector<Conversion<T>>& conversions) : baseMap()
        {
            for (auto pairIter1 = conversions.begin(); pairIter1 != conversions.end(); ++pairIter1)
            {
                auto t1 = pairIter1->first;
                auto& f11 = pairIter1->second.first;
                auto& f12 = pairIter1->second.second;

                baseMap[{t1, t1}] = [](const double x) { return x; };

                for (auto pairIter2 = pairIter1 + 1; pairIter2 != conversions.end(); ++pairIter2)
                {
                    auto t2 = pairIter2->first;
                    auto& f21 = pairIter2->second.first;
                    auto& f22 = pairIter2->second.second;
                    baseMap[{t1, t2}] = [f22, f11](const double x) { return f22(f11(x)); };
                    baseMap[{t2, t1}] = [f12, f21](const double x) { return f12(f21(x)); };
                }
            }
        }

        std::function<double(const double)> operator[](const ConversionPair<T>& conversionPair)
        {
            return baseMap[conversionPair];
        }
        std::function<double(const double)> at(const ConversionPair<T>& conversionPair)
        {
            return baseMap.at(conversionPair);
        }
    };

    static ConversionMap conversionMap;

    static double convert(const double x, const T fromUnits, const T toUnits)
    {
        return conversionMap.at(std::make_pair(fromUnits, toUnits))(x);
    }

    static double convert(const double x, const T fromUnits, const T toUnits, int power)
    {
        if (power > 0)
        {
            return convert(convert(x, fromUnits, toUnits), fromUnits, toUnits, power - 1);
        }
        if (power < 0)
        {
            return convert(convert(x, toUnits, fromUnits), toUnits, fromUnits, power + 1);
        }
        return x;
    }

    static std::vector<double>
    convert(const std::vector<double>& xV, const T fromUnits, const T toUnits)
    {
        std::vector<double> xV_out;
        for (auto& x : xV)
        {
            double y = convert(x, fromUnits, toUnits);
            xV_out.push_back(y);
        }
        return xV_out;
    }

    static std::vector<double>
    convert(const std::vector<double>& xV, const T fromUnits, const T toUnits, int power)
    {
        std::vector<double> xV_out;
        for (auto& x : xV)
        {
            double y = convert(x, fromUnits, toUnits, power);
            xV_out.push_back(y);
        }
        return xV_out;
    }
};

template <class T, Mode mode = Mode::Abs>
static double convert(const double x, const T fromUnits, const T toUnits)
{
    return Converter<T, mode>::convert(x, fromUnits, toUnits);
}

template <class T, Mode mode = Mode::Abs>
static double convert(const double x, const T fromUnits, const T toUnits, int power)
{
    return Converter<T, mode>::convert(x, fromUnits, toUnits, power);
}

template <class T, Mode mode = Mode::Abs>
static std::vector<double>
convert(const std::vector<double>& xV, const T fromUnits, const T toUnits)
{
    return Converter<T, mode>::convert(xV, fromUnits, toUnits);
}

template <class T, Mode mode = Mode::Abs>
static std::vector<double>
convert(const std::vector<double>& xV, const T fromUnits, const T toUnits, int power)
{
    return Converter<T, mode>::convert(xV, fromUnits, toUnits, power);
}

/// units values
template <class T, T units, Mode mode = Mode::Abs>
struct UnitsVal
{
  protected:
    double x;

  public:
    UnitsVal(const double x_in = 0.) : x(x_in) {}

    UnitsVal(const double x_in, const T fromUnits)
        : x(Converter<T, mode>::convert(x_in, fromUnits, units))
    {
    }

    UnitsVal(const double x_in, const T fromUnits, int power)
        : x(Converter<T, mode>::convert(x_in, fromUnits, units, power))
    {
    }

    template <T fromUnits>
    UnitsVal(const UnitsVal<T, fromUnits, mode> unitsVal)
        : x(Converter<T, mode>::convert(unitsVal, fromUnits, units))
    {
    }

    template <T fromUnits>
    UnitsVal(const UnitsVal<T, fromUnits, mode> unitsVal, int power)
        : x(Converter<T, mode>::convert(unitsVal, fromUnits, units, power))
    {
    }

    double to(const T toUnits) const { return Converter<T, mode>::convert(x, units, toUnits); }
    double to(const T toUnits, int power) const
    {
        return Converter<T, mode>::convert(x, units, toUnits, power);
    }

    double operator()(const T toUnits) const { return to(toUnits); }
    double operator()(const T toUnits, int power) const { return to(toUnits, power); }

    operator double() const { return x; }

    double as_double() const { return x; }

    template <T toUnits>
    bool operator==(const UnitsVal<T, toUnits> unitsVal) const
    {
        return (unitsVal(units) == x);
    }

    template <T toUnits>
    bool operator!=(const UnitsVal<T, toUnits> unitsVal) const
    {
        return !(operator==(unitsVal));
    }

    template <T toUnits>
    UnitsVal operator+(const UnitsVal<T, toUnits> unitsVal) const
    {
        return UnitsVal(x + unitsVal(units));
    }

    template <T toUnits, Mode toMode>
    UnitsVal operator+(const UnitsVal<T, toUnits, toMode> unitsVal) const
    {
        return UnitsVal(x + unitsVal(units));
    }

    template <T toUnits>
    UnitsVal operator-(const UnitsVal<T, toUnits> unitsVal) const
    {
        return UnitsVal(x - unitsVal(units));
    }

    template <T toUnits, Mode toMode>
    UnitsVal operator-(const UnitsVal<T, toUnits, toMode> unitsVal) const
    {
        return UnitsVal(x - unitsVal(units));
    }

    template <T toUnits>
    UnitsVal& operator+=(const UnitsVal<T, toUnits> unitsVal)
    {
        return *this = x + unitsVal(units);
    }

    template <T toUnits, Mode toMode>
    UnitsVal& operator+=(const UnitsVal<T, toUnits, toMode> unitsVal)
    {
        return *this = x + unitsVal(units);
    }

    template <T toUnits>
    UnitsVal& operator-=(const UnitsVal<T, toUnits> unitsVal)
    {
        return *this = x - unitsVal(units);
    }

    template <T toUnits, Mode toMode>
    UnitsVal& operator-=(const UnitsVal<T, toUnits, toMode> unitsVal)
    {
        return *this = x - unitsVal(units);
    }

    static std::vector<UnitsVal> convert(const std::vector<UnitsVal<T, units>>& xV, const T toUnits)
    {
        return Converter<T, mode>::convert(xV, units, toUnits);
    }

    static std::vector<UnitsVal>
    convert(const std::vector<UnitsVal<T, units>>& xV, const T toUnits, int power)
    {
        return Converter<T, mode>::convert(xV, units, toUnits, power);
    }
};

/// units vectors
template <class T, T units, Mode mode = Mode::Abs>
struct UnitsVect
{
  public:
    std::vector<UnitsVal<T, units>> fV;

    UnitsVect(const std::vector<double>& xV_in = {})
    {
        fV.clear();
        for (auto x : xV_in)
            fV.push_back(x);
    }

    UnitsVect(const std::vector<double>& xV_in, const T fromUnits)
    {
        fV.clear();
        for (auto x : xV_in)
            fV.push_back(Converter<T, mode>::convert(x, fromUnits, units));
    }

    template <T fromUnits, Mode fromMode = Mode::Abs>
    UnitsVect(const std::vector<UnitsVal<T, fromUnits, fromMode>>& xV_from)
    {
        fV.clear();
        for (auto x : xV_from)
            fV.push_back(Converter<T, mode>::convert(x, fromUnits, units));
    }

    template <T fromUnits>
    UnitsVect(const UnitsVect<T, fromUnits>& fV_from)
    {
        fV.clear();
        for (auto f : fV_from.fV)
            fV.push_back(Converter<T, mode>::convert(f, fromUnits, units));
    }

    operator std::vector<double>() const
    {
        std::vector<double> xV_to;
        for (auto f : fV)
            xV_to.push_back(f);
        return xV_to;
    }

    std::vector<double> as_double() const { return operator std::vector<double>(); }

    std::vector<double> to(const T toUnits) const
    {
        return Converter<T, mode>::convert(operator std::vector<double>(), units, toUnits);
    }

    std::vector<double> to(const T toUnits, int power) const
    {
        return Converter<T, mode>::convert(operator std::vector<double>(), units, toUnits, power);
    }

    std::vector<double> operator()(const T toUnits) const { return to(toUnits); }

    template <T toUnits>
    bool operator==(const UnitsVect<T, toUnits> unitsVect) const
    {
        return (unitsVect(units) == fV(units));
    }

    template <T toUnits>
    bool operator!=(const UnitsVect<T, toUnits> unitsVect) const
    {
        return !(operator==(unitsVect));
    }

    UnitsVal<T, units>& operator[](const std::size_t i) { return fV[i]; }

    UnitsVal<T, units>* begin() { return &fV[0]; }

    UnitsVal<T, units>* end() { return &fV[0] + fV.size(); }
};

/// units pairs, e.g., (Time::h, Time::min)
template <class T, T units1, T units2>
struct UnitsPair
{
  protected:
    std::pair<UnitsVal<T, units1>, UnitsVal<T, units2>> fPair;

  public:
    UnitsPair(const double x1_in, const double x2_in) : fPair({x1_in, x2_in}) {}

    UnitsPair(const UnitsVal<T, units1> unitsVal1, const UnitsVal<T, units2> unitsVal2)
        : fPair({unitsVal1, unitsVal2})
    {
    }

    double to(const T toUnits) const { return fPair.first(toUnits) + fPair.second(toUnits); }
    double operator()(const T toUnits) const { return to(toUnits); }

    operator std::pair<UnitsVal<T, units1>, UnitsVal<T, units2>>() const { return fPair; }

    std::pair<double, double> as_pair() const { return fPair; }

    template <T toUnits>
    bool operator==(const UnitsVal<T, toUnits> unitsVal) const
    {
        return (unitsVal == to(toUnits));
    }

    template <T toUnits>
    bool operator!=(const UnitsVal<T, toUnits> unitsVal) const
    {
        return !(operator==(unitsVal));
    }
};

/* time units */
enum class Time
{
    h,   // hours
    min, // minutes
    s    // seconds
};

/* temperature units */
enum class Temp
{
    C, // celsius
    F  // fahrenheit
};

/* energy units */
enum class Energy
{
    kJ,  // kilojoules
    kWh, // kilowatt hours
    Btu  // british thermal units
};

/* power units */
enum class Power
{
    kW,        // kilowatts
    Btu_per_h, // BTU per hour
    W,         // watts
    kJ_per_h,  // kilojoules per hour
};

/* length units */
enum class Length
{
    m, // meters
    ft // feet
};

/* area units */
enum class Area
{
    m2, // square meters
    ft2 // square feet
};

/* volume units */
enum class Volume
{
    L,   // liters
    gal, // gallons
    m3,  // cubic meters
    ft3  // cubic feet
};

/* flow-rate units */
enum class FlowRate
{
    L_per_s,    // liters per second
    gal_per_min // gallons per minute
};

/* UA units */
enum class UA
{
    kJ_per_hC, // kilojoules per hour degree celsius
    Btu_per_hF // british thermal units per hour degree Fahrenheit
};

/// instantiate conversion maps
template <>
inline Converter<Time>::ConversionMap Converter<Time>::conversionMap(
    {{Time::s, {ident, ident}}, {Time::min, {MIN_TO_S, S_TO_MIN}}, {Time::h, {H_TO_S, S_TO_H}}});

template <>
inline Converter<Temp /*,Mode::Abs*/>::ConversionMap Converter<Temp, Mode::Abs>::conversionMap(
    {{Temp::C, {ident, ident}}, {Temp::F, {F_TO_C, C_TO_F}}});

template <>
inline Converter<Temp, Mode::Diff>::ConversionMap Converter<Temp, Mode::Diff>::conversionMap(
    {{Temp::C, {ident, ident}}, {Temp::F, {dF_TO_dC, dC_TO_dF}}});

template <>
inline Converter<Energy>::ConversionMap
    Converter<Energy>::conversionMap({{Energy::kJ, {ident, ident}},
                                      {Energy::kWh, {KWH_TO_KJ, KJ_TO_KWH}},
                                      {Energy::Btu, {BTU_TO_KJ, KJ_TO_BTU}}});

template <>
inline Converter<Power>::ConversionMap
    Converter<Power>::conversionMap({{Power::kW, {ident, ident}},
                                     {Power::Btu_per_h, {BTUperH_TO_KW, KW_TO_BTUperH}},
                                     {Power::W, {W_TO_KW, KW_TO_W}},
                                     {Power::kJ_per_h, {KJperH_TO_KW, KW_TO_KJperH}}});

template <>
inline Converter<Length>::ConversionMap Converter<Length>::conversionMap(
    {{Length::m, {ident, ident}}, {Length::ft, {FT_TO_M, M_TO_FT}}});

template <>
inline Converter<Area>::ConversionMap Converter<Area>::conversionMap(
    {{Area::m2, {ident, ident}}, {Area::ft2, {FT2_TO_M2, M2_TO_FT2}}});

template <>
inline Converter<Volume>::ConversionMap
    Converter<Volume>::conversionMap({{Volume::L, {ident, ident}},
                                      {Volume::gal, {GAL_TO_L, L_TO_GAL}},
                                      {Volume::m3, {M3_TO_L, L_TO_M3}},
                                      {Volume::ft3, {FT3_TO_L, L_TO_FT3}}});

template <>
inline Converter<UA>::ConversionMap
    Converter<UA>::conversionMap({{UA::kJ_per_hC, {ident, ident}},
                                  {UA::Btu_per_hF, {BTUperHF_TO_KJperHC, KJperHC_TO_BTUperHF}}});

template <>
inline Converter<FlowRate>::ConversionMap Converter<FlowRate>::conversionMap(
    {{FlowRate::L_per_s, {ident, ident}}, {FlowRate::gal_per_min, {GPM_TO_LPS, LPS_TO_GPM}}});

/// units-values partial specializations
template <Time units>
using TimeVal = UnitsVal<Time, units>;

template <Temp units>
using TempVal = UnitsVal<Temp, units>;

template <Temp units>
using TempDiffVal = UnitsVal<Temp, units, Mode::Diff>;

template <Energy units>
using EnergyVal = UnitsVal<Energy, units>;

template <Power units>
using PowerVal = UnitsVal<Power, units>;

template <Length units>
using LengthVal = UnitsVal<Length, units>;

template <Area units>
using AreaVal = UnitsVal<Area, units>;

template <Volume units>
using VolumeVal = UnitsVal<Volume, units>;

template <FlowRate units>
using FlowRateVal = UnitsVal<FlowRate, units>;

template <UA units>
using UAVal = UnitsVal<UA, units>;

/// units-vectors partial specializations
template <Time units>
using TimeVect = UnitsVect<Time, units>;

template <Temp units>
using TempVect = UnitsVect<Temp, units>;

template <Temp units>
using TempDiffVect = UnitsVect<Temp, units, Mode::Diff>;

template <Energy units>
using EnergyVect = UnitsVect<Energy, units>;

template <Power units>
using PowerVect = UnitsVect<Power, units>;

template <Length units>
using LengthVect = UnitsVect<Length, units>;

template <Area units>
using AreaVect = UnitsVect<Area, units>;

template <Volume units>
using VolumeVect = UnitsVect<Volume, units>;

template <FlowRate units>
using FlowRateVect = UnitsVect<FlowRate, units>;

template <UA units>
using UAVect = UnitsVect<UA, units>;

/// units-pair partial specialization
template <Time units1, Time units2>
using TimePair = UnitsPair<Time, units1, units2>;

/// units-values full specializations
typedef TimeVal<Time::s> Time_s;
typedef TimeVal<Time::min> Time_min;
typedef TempVal<Temp::C> Temp_C;
typedef TempDiffVal<Temp::C> TempDiff_C;
typedef EnergyVal<Energy::kJ> Energy_kJ;
typedef PowerVal<Power::kW> Power_kW;
typedef LengthVal<Length::m> Length_m;
typedef AreaVal<Area::m2> Area_m2;
typedef VolumeVal<Volume::L> Volume_L;
typedef FlowRateVal<FlowRate::L_per_s> FlowRate_L_per_s;
typedef UAVal<Units::UA::kJ_per_hC> UA_kJ_per_hC;

/// units-vectors full specializations
typedef TimeVect<Time::s> TimeVect_s;
typedef TimeVect<Time::min> TimeVect_min;
typedef TempVect<Temp::C> TempVect_C;
typedef TempDiffVect<Temp::C> TempDiffVect_C;
typedef EnergyVect<Energy::kJ> EnergyVect_kJ;
typedef PowerVect<Power::kW> PowerVect_kW;
typedef LengthVect<Length::m> LengthVect_m;
typedef AreaVect<Area::m2> AreaVect_m2;
typedef VolumeVect<Volume::L> VolumeVect_L;
typedef FlowRateVect<FlowRate::L_per_s> FlowRateVect_L_per_s;
typedef UAVect<UA::kJ_per_hC> UAVect_kJ_per_hC;

/// units-pair full specialization
typedef TimePair<Time::h, Time::min> Time_h_min;

} // namespace Units

#endif
