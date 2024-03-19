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
inline double MIN_TO_S(const double min) { return s_per_min * min; }

inline double S_TO_H(const double s) { return s / s_per_h; }
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
inline double KWH_TO_KJ(const double kWh) { return kWh * s_per_h; }

inline double KJ_TO_BTU(const double kJ) { return Btu_per_kJ * kJ; }
inline double BTU_TO_KJ(const double Btu) { return kJ_per_Btu * Btu; }

inline double KWH_TO_BTU(const double kWh) { return KJ_TO_BTU(KWH_TO_KJ(kWh)); }
inline double BTU_TO_KWH(const double Btu) { return KJ_TO_KWH(BTU_TO_KJ(Btu)); }

// power conversion
inline double KW_TO_BTUperH(const double kW) { return Btu_per_kJ * s_per_h * kW; }
inline double BTUperH_TO_KW(const double Btu_per_h) { return kJ_per_Btu * Btu_per_h / s_per_h; }

inline double KW_TO_W(const double kW) { return 1000. * kW; }
inline double W_TO_KW(const double W) { return W / 1000.; }

inline double KW_TO_KJperH(const double kW) { return kW * s_per_h; }
inline double KJperH_TO_KW(const double kJ_per_h) { return kJ_per_h / s_per_h; }

inline double BTUperH_TO_W(const double Btu_per_h) { return KW_TO_W(BTUperH_TO_KW(Btu_per_h)); }
inline double W_TO_BTUperH(const double W) { return KW_TO_BTUperH(W_TO_KW(W)); }

inline double BTUperH_TO_KJperH(const double Btu_per_h)
{
    return KW_TO_KJperH(BTUperH_TO_KW(Btu_per_h));
}
inline double KJperH_TO_BTUperH(const double kJ_per_h)
{
    return KW_TO_BTUperH(KJperH_TO_KW(kJ_per_h));
}

inline double W_TO_KJperH(const double W) { return KW_TO_KJperH(W_TO_KW(W)); }
inline double KJperH_TO_W(const double kJ_per_h) { return KW_TO_W(KJperH_TO_KW(kJ_per_h)); }

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
inline double GAL_TO_M3(const double gal) { return L_TO_M3(GAL_TO_L(gal)); }
inline double GAL_TO_FT3(const double gal) { return L_TO_FT3(GAL_TO_L(gal)); }

inline double FT3_TO_L(const double ft3) { return ft3 / ft3_per_L; }
inline double FT3_TO_M3(const double ft3) { return L_TO_M3(FT3_TO_L(ft3)); }
inline double FT3_TO_GAL(const double ft3) { return L_TO_GAL(FT3_TO_L(ft3)); }

inline double M3_TO_L(const double m3) { return L_per_m3 * m3; }
inline double M3_TO_GAL(const double m3) { return L_TO_GAL(M3_TO_L(m3)); }
inline double M3_TO_FT3(const double m3) { return L_TO_FT3(M3_TO_L(m3)); }

// flow-rate conversion
inline double GPM_TO_LPS(const double gpm) { return (gpm / gal_per_L / s_per_min); }
inline double LPS_TO_GPM(const double lps) { return (gal_per_L * lps * s_per_min); }

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

    using ConversionMap =
        std::unordered_map<std::pair<T, T>, std::function<double(const double)>, PairHash>;

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

    UnitsVect(const std::vector<double>& xV_from = {}, const T fromUnits = units)
    {
        fV.clear();
        for (auto x : xV_from)
            fV.push_back(Converter<T, mode>::convert(x, fromUnits, units));
    }

    template <T fromUnits, Mode fromMmode = Mode::Abs>
    UnitsVect(const std::vector<UnitsVal<T, fromUnits, fromMmode>>& xV_from)
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

    std::vector<double> to(const T toUnits) const
    {
        return Converter<T>::convert(fV, units, toUnits);
    }

    std::vector<double> to(const T toUnits, int power) const
    {
        return Converter<T, mode>::convert(fV, units, toUnits, power);
    }

    std::vector<double> operator()(const T toUnits) const { return to(toUnits); }

    template <T toUnits>
    bool operator==(const UnitsVect<T, toUnits> unitsVect) const
    {
        return (unitsVect(units) == fV);
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

template <>
inline Converter<Time>::ConversionMap Converter<Time>::conversionMap = {
    {{Time::h, Time::h}, &ident},
    {{Time::min, Time::min}, &ident},
    {{Time::s, Time::s}, &ident},
    {{Time::h, Time::min}, H_TO_MIN},
    {{Time::h, Time::s}, &H_TO_S},
    {{Time::min, Time::h}, MIN_TO_H},
    {{Time::min, Time::s}, &MIN_TO_S},
    {{Time::s, Time::h}, &S_TO_H},
    {{Time::s, Time::min}, &S_TO_MIN}};

template <>
inline Converter<Temp /*,Mode::Abs*/>::ConversionMap Converter<Temp, Mode::Abs>::conversionMap = {
    {{Temp::F, Temp::F}, &ident},
    {{Temp::C, Temp::C}, &ident},
    {{Temp::C, Temp::F}, &C_TO_F},
    {{Temp::F, Temp::C}, &F_TO_C}};

template <>
inline Converter<Temp, Mode::Diff>::ConversionMap Converter<Temp, Mode::Diff>::conversionMap = {
    {{Temp::F, Temp::F}, ident},
    {{Temp::C, Temp::C}, ident},
    {{Temp::C, Temp::F}, dC_TO_dF},
    {{Temp::F, Temp::C}, dF_TO_dC}};

template <>
inline Converter<Energy>::ConversionMap Converter<Energy>::conversionMap = {
    {{Energy::kJ, Energy::kJ}, ident},
    {{Energy::kWh, Energy::kWh}, ident},
    {{Energy::Btu, Energy::Btu}, ident},
    {{Energy::kJ, Energy::kWh}, KJ_TO_KWH},
    {{Energy::kJ, Energy::Btu}, KJ_TO_BTU},
    {{Energy::kWh, Energy::kJ}, KWH_TO_KJ},
    {{Energy::kWh, Energy::Btu}, KWH_TO_BTU},
    {{Energy::Btu, Energy::kJ}, BTU_TO_KJ},
    {{Energy::Btu, Energy::kWh}, BTU_TO_KWH}};

template <>
inline Converter<Power>::ConversionMap Converter<Power>::conversionMap = {
    {{Power::kW, Power::kW}, ident},
    {{Power::Btu_per_h, Power::Btu_per_h}, ident},
    {{Power::W, Power::W}, ident},
    {{Power::kJ_per_h, Power::kJ_per_h}, ident},
    {{Power::kW, Power::Btu_per_h}, KW_TO_BTUperH},
    {{Power::kW, Power::W}, KW_TO_W},
    {{Power::kW, Power::kJ_per_h}, KW_TO_KJperH},
    {{Power::Btu_per_h, Power::kW}, BTUperH_TO_KW},
    {{Power::Btu_per_h, Power::W}, BTUperH_TO_W},
    {{Power::Btu_per_h, Power::kJ_per_h}, BTUperH_TO_KJperH},
    {{Power::W, Power::kW}, W_TO_KW},
    {{Power::W, Power::Btu_per_h}, W_TO_BTUperH},
    {{Power::W, Power::kJ_per_h}, W_TO_KJperH},
    {{Power::kJ_per_h, Power::kW}, KJperH_TO_KW},
    {{Power::kJ_per_h, Power::Btu_per_h}, KJperH_TO_BTUperH},
    {{Power::kJ_per_h, Power::W}, KJperH_TO_W}};

template <>
inline Converter<Length>::ConversionMap Converter<Length>::conversionMap = {
    {{Length::m, Length::m}, &ident},
    {{Length::ft, Length::ft}, &ident},
    {{Length::m, Length::ft}, &M_TO_FT},
    {{Length::ft, Length::m}, &FT_TO_M}};

template <>
inline Converter<Area>::ConversionMap Converter<Area>::conversionMap = {
    {{Area::m2, Area::m2}, &ident},
    {{Area::ft2, Area::ft2}, &ident},
    {{Area::m2, Area::ft2}, &M2_TO_FT2},
    {{Area::ft2, Area::m2}, &FT2_TO_M2}};

template <>
inline Converter<Volume>::ConversionMap Converter<Volume>::conversionMap = {
    {{Volume::L, Volume::L}, ident},
    {{Volume::gal, Volume::gal}, ident},
    {{Volume::m3, Volume::m3}, ident},
    {{Volume::ft3, Volume::ft3}, ident},
    {{Volume::L, Volume::gal}, L_TO_GAL},
    {{Volume::L, Volume::m3}, L_TO_M3},
    {{Volume::L, Volume::ft3}, L_TO_FT3},
    {{Volume::gal, Volume::L}, GAL_TO_L},
    {{Volume::gal, Volume::m3}, GAL_TO_M3},
    {{Volume::gal, Volume::ft3}, GAL_TO_FT3},
    {{Volume::ft3, Volume::L}, FT3_TO_L},
    {{Volume::ft3, Volume::gal}, FT3_TO_GAL},
    {{Volume::ft3, Volume::m3}, FT3_TO_M3},
    {{Volume::m3, Volume::L}, M3_TO_L},
    {{Volume::m3, Volume::gal}, M3_TO_GAL},
    {{Volume::m3, Volume::ft3}, M3_TO_FT3}};

template <>
inline Converter<UA>::ConversionMap Converter<UA>::conversionMap = {
    {{UA::kJ_per_hC, UA::kJ_per_hC}, &ident},
    {{UA::Btu_per_hF, UA::Btu_per_hF}, &ident},
    {{UA::kJ_per_hC, UA::Btu_per_hF}, &KJperHC_TO_BTUperHF},
    {{UA::Btu_per_hF, UA::kJ_per_hC}, &BTUperHF_TO_KJperHC}};

template <>
inline Converter<FlowRate>::ConversionMap Converter<FlowRate>::conversionMap = {
    {{FlowRate::L_per_s, FlowRate::L_per_s}, &ident},
    {{FlowRate::gal_per_min, FlowRate::gal_per_min}, &ident},
    {{FlowRate::L_per_s, FlowRate::gal_per_min}, &LPS_TO_GPM},
    {{FlowRate::gal_per_min, FlowRate::L_per_s}, &GPM_TO_LPS}};

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
} // namespace Units

#endif
