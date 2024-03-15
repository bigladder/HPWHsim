#ifndef HPWH_UNITS_
#define HPWH_UNITS_

#include <unordered_map>
#include <vector>

// reference conversion factors
constexpr double s_per_min = 60.;            // s / min
constexpr double min_per_h = 60.;            // min / h
constexpr double in3_per_gal = 231.;         // in^3 / gal (U.S., exact)
constexpr double m_per_in = 0.0254;          // m / in (exact)
constexpr double F_per_C = 1.8;              // degF / degC
constexpr double offsetF = 32.;              // degF offset
constexpr double kJ_per_Btu = 1.05505585262; // kJ / Btu (IT), https://www.unitconverters.net/

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
inline double GAL_TO_L(const double gal) { return gal / gal_per_L; }

inline double L_TO_FT3(const double L) { return ft3_per_L * L; }
inline double FT3_TO_L(const double ft3) { return ft3 / ft3_per_L; }

inline double GAL_TO_FT3(const double gal) { return L_TO_FT3(GAL_TO_L(gal)); }
inline double FT3_TO_GAL(const double ft3) { return L_TO_GAL(FT3_TO_L(ft3)); }

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

/* temperature-difference units */
enum class TempDiff
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

/// unit-conversion utilities
template <typename T>
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
        // .find(std::make_pair(fromUnits, toUnits))(x);
        //[std::make_pair(fromUnits, toUnits)](x); //
    }

    static double revert(const double x, const T fromUnits, const T toUnits)
    {
        return convert(x, toUnits, fromUnits);
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
    revert(const std::vector<double>& xV, const T fromUnits, const T toUnits)
    {
        return convert(xV, toUnits, fromUnits);
    }
};

template <class T>
static double convert(const double x, const T fromUnits, const T toUnits)
{
    return Converter<T>::convert(x, fromUnits, toUnits);
}

template <class T>
static double revert(const double x, const T fromUnits, const T toUnits)
{
    return Converter<T>::revert(x, fromUnits, toUnits);
}

template <class T>
static std::vector<double>
convert(const std::vector<double>& xV, const T fromUnits, const T toUnits)
{
    return Converter<T>::convert(xV, fromUnits, toUnits);
}

template <class T>
static std::vector<double> revert(const std::vector<double>& xV, const T fromUnits, const T toUnits)
{
    return Converter<T>::revert(xV, fromUnits, toUnits);
}

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
inline Converter<Temp>::ConversionMap Converter<Temp>::conversionMap = {
    {{Temp::F, Temp::F}, &ident},
    {{Temp::C, Temp::C}, &ident},
    {{Temp::C, Temp::F}, &C_TO_F},
    {{Temp::F, Temp::C}, &F_TO_C}};

template <>
inline Converter<TempDiff>::ConversionMap Converter<TempDiff>::conversionMap = {
    {{TempDiff::F, TempDiff::F}, ident},
    {{TempDiff::C, TempDiff::C}, ident},
    {{TempDiff::C, TempDiff::F}, dC_TO_dF},
    {{TempDiff::F, TempDiff::C}, dF_TO_dC}};

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
    {{Volume::ft3, Volume::ft3}, ident},
    {{Volume::L, Volume::gal}, L_TO_GAL},
    {{Volume::L, Volume::ft3}, L_TO_FT3},
    {{Volume::gal, Volume::L}, GAL_TO_L},
    {{Volume::gal, Volume::ft3}, GAL_TO_FT3},
    {{Volume::ft3, Volume::L}, FT3_TO_L},
    {{Volume::ft3, Volume::gal}, FT3_TO_GAL}};

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

/// fixed-unit quantities
template <class T>
struct UnitsVal
{

  protected:
    double x;

  public:
    UnitsVal(const double x_in) : x(x_in) {}

    UnitsVal(const double x_in, const T fromUnits, const T toUnits)
        : x(Converter<T>::convert(x_in, fromUnits, toUnits))
    {
    }

    UnitsVal operator=(const UnitsVal unitVal_in)
    {
        x = unitVal_in.x;
        return *this;
    }

    UnitsVal operator=(const double x_in)
    {
        x = x_in;
        return *this;
    }

    UnitsVal operator()(const T fromUnits, const T toUnits) const { return to(fromUnits, toUnits); }

    double to(const T fromUnits, const T toUnits) const
    {
        return Converter<T>::convert(x, fromUnits, toUnits);
    }

    operator double() const { return x; }
};

template <class T, T refUnits>
struct FixedUnitsVal : public UnitsVal<T>
{

    FixedUnitsVal(const double x_in = 0.) : UnitsVal<T>(x_in) {}

    FixedUnitsVal(const double x_in, const T fromUnits)
        : UnitsVal<T>(Converter<T>::convert(x_in, fromUnits, refUnits))
    {
    }

    template <T fromUnits>
    FixedUnitsVal(const FixedUnitsVal<T, fromUnits> fixedUnitsVal)
        : UnitsVal<T>(fixedUnitsVal, fromUnits, refUnits)
    {
    }

    UnitsVal<T> operator()(const T toUnits) const
    {
        return UnitsVal<T>(this->x, refUnits, toUnits);
    }
};
typedef FixedUnitsVal<Time, Time::s> Time_s;
typedef FixedUnitsVal<Time, Time::min> Time_min;
typedef FixedUnitsVal<Temp, Temp::C> Temp_C;
typedef FixedUnitsVal<TempDiff, TempDiff::C> TempDiff_C;
typedef FixedUnitsVal<Energy, Energy::kJ> Energy_kJ;
typedef FixedUnitsVal<Power, Power::kW> Power_kW;
typedef FixedUnitsVal<Length, Length::m> Length_m;
typedef FixedUnitsVal<Area, Area::m2> Area_m2;
typedef FixedUnitsVal<Volume, Volume::L> Volume_L;
typedef FixedUnitsVal<FlowRate, FlowRate::L_per_s> FlowRate_L_per_s;
typedef FixedUnitsVal<UA, UA::kJ_per_hC> UA_kJ_per_hC;

} // namespace Units

#endif
