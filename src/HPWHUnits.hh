#ifndef HPWH_UNITS_
#define HPWH_UNITS_

#include <iterator>
#include <unordered_map>
#include <vector>

/// units specifications
namespace Units {

template <typename D> struct Transform {
    D data;
    Transform(const D &data_in) : data(data_in) {}
    operator D() const { return data; }
    virtual double operator*(const double x) const = 0;
};

struct Offset;
struct ScaleOffset;

struct Scale : Transform<double> {
    Scale(const double scale) : Transform(scale) {}
    static Scale ident() { return 1.; }
    Scale inverse() const { return 1. / data; }
    double operator*(const double x) const override { return data * x; }
    Scale operator*(const Scale &s) const { return data * s; }
    Scale operator/(const Scale &s) const { return inverse() * s; }
    double scale() const { return data; }
    friend double operator/(const double x, const Scale &scale) {
        return scale.inverse() * x;
    }
};

struct Offset : Transform<double> {
    using Transform<double>::data;
    Offset(const double offset) : Transform(offset) {}
    operator double() { return data; }
    static Offset ident() { return 0.; }
    Offset inverse() const { return -data; }
    double operator*(const double x) const override { return x + data; }
    Offset operator*(const Offset &o) const { return data + o.data; }
    Offset operator/(const Offset &o) const { return data - o.data; }
    double offset() const { return data; }
    friend double operator/(const double x, const Offset &offset) {
        return offset.inverse() * x;
    }
};

using ScaleOffsetSeq = std::pair<Scale, Offset>;
struct ScaleOffset : Transform<ScaleOffsetSeq> {
    ScaleOffset(const ScaleOffsetSeq &scaleOffsetSeq)
        : Transform(scaleOffsetSeq) {}
    ScaleOffset(const Scale &scale, const Offset &offset)
        : Transform({scale, offset}) {}

    static ScaleOffset ident() { return {1., 0.}; }
    ScaleOffset inverse() const {
        return {data.first.inverse(), data.second.inverse()};
    }
    double operator*(const double x) const {
        return data.second * data.first * x;
    }
    ScaleOffset operator*(const ScaleOffset &a) const {
        return {data.first * a.data.first,
                Offset(data.first.scale() * a.data.second.offset() +
                       data.second.offset())};
    }
    Scale scale() const { return data.first; }
    Offset offset() const { return data.second; }

    friend double operator/(const double x, const ScaleOffset &scaleOffset) {
        return scaleOffset.inverse() * x;
    }
};

/// wrapper for Transforms
template <class T> struct Conversion : public T {
    Conversion(const T &t_in) : T(t_in) {}

    template <typename D> Conversion(const D &d_in) : T(d_in) {}

    static T ident() { return T::ident(); }
};

template <typename U> struct PairHash {
    std::size_t operator()(const std::pair<U, U> &p) const {
        auto h1 = static_cast<std::size_t>(p.first);
        auto h2 = static_cast<std::size_t>(p.second);
        return h1 ^ h2;
    }
};

/// Converter classes
template <typename U, typename T> struct Converter {

    struct ConversionMap : std::unordered_map<std::pair<U, U>, T, PairHash<U>> {

        using std::unordered_map<std::pair<U, U>, T, PairHash<U>>::insert;
        using std::unordered_map<std::pair<U, U>, T, PairHash<U>>::at;
        ConversionMap(const U uRef,
                      const std::vector<std::pair<U, T>> &conversions) {
            insert({{uRef, uRef}, T::ident()});
            for (auto conversion0 = conversions.begin();
                 conversion0 != conversions.end(); ++conversion0) {
                auto t0 = conversion0->first;   // e.g., Time::min
                auto f0 = conversion0->second;  // e.g., 1. / s_per_min
                insert({{t0, t0}, T::ident()}); // e.g., 1.
                insert({{t0, uRef}, f0.inverse()});
                insert({{uRef, t0}, f0});

                for (auto conversion1 = conversion0 + 1;
                     conversion1 != conversions.end(); ++conversion1) {
                    auto t1 = conversion1->first;          // e.g., Time::h
                    auto f1 = conversion1->second;         // t1 -> (standard)
                    insert({{t0, t1}, f1 * f0.inverse()}); // t0 -> (standard) -> t1
                    insert({{t1, t0}, f0 * f1.inverse()}); // t1 -> (standard) -> t0
                }
            }
        }
        T conversion(const U fromUnits, const U toUnits) {
            return at({fromUnits, toUnits});
        }
    };
};

template <typename U> struct Scaler : Converter<U, Scale> {

    static struct ScaleMap : Converter<U, Conversion<Scale>>::ConversionMap {

        ScaleMap(const U tRef,
                 const std::vector<std::pair<U, Conversion<Scale>>> &conversions)
            : Converter<U, Conversion<Scale>>::ConversionMap(tRef, conversions) {}

    } scaleMap;

    static Conversion<Scale> conversion(const U fromUnits, const U toUnits) {
        return scaleMap.conversion(fromUnits, toUnits);
    }
};

template <typename U> struct ScaleOffseter : Converter<U, ScaleOffset> {

    static struct ScaleOffsetMap : Converter<U, ScaleOffset>::ConversionMap {

        ScaleOffsetMap(const U tRef,
                       const std::vector<std::pair<U, ScaleOffset>> &conversions)
            : Converter<U, ScaleOffset>::ConversionMap(tRef, conversions) {}

    } scaleOffsetMap;

    inline static ScaleOffset conversion(const U fromUnits, const U toUnits) {
        return scaleOffsetMap.conversion(fromUnits, toUnits);
    }
};

/// non-member conversion fncs
template <class U> Scale scale(const U fromUnits, const U toUnits) {
    return Scaler<U>::conversion(fromUnits, toUnits);
}

template <class U> ScaleOffset scaleOffset(const U fromUnits, const U toUnits) {
    return ScaleOffseter<U>::conversion(fromUnits, toUnits);
}

/// conversion fncs
template <class U>
double scale(const U fromUnits, const U toUnits, const double x) {
    return scale(fromUnits, toUnits) * x;
}

template <class U>
double scaleOffset(const U fromUnits, const U toUnits, const double x) {
    return scaleOffset(fromUnits, toUnits) * x;
}

/// convert values
template <class U, typename datatype, U units> struct ConvertVal {
  protected:
    double x;

  public:
    ConvertVal(const double x_in = 0.) : x(x_in) {}

    template <U fromUnits>
    ConvertVal(const ConvertVal<U, datatype, fromUnits> convertVal)
        : x(convertVal.to(units)) {}

    virtual double operator()(const U toUnits) const = 0;

    operator double() const { return x; }

    double operator=(double x_in) { return x = x_in; }

    template <U toUnits>
    bool operator==(const ConvertVal<U, datatype, toUnits> convertVal) const {
        return (convertVal.to(units) == x);
    }

    template <U toUnits>
    bool operator!=(const ConvertVal<U, datatype, toUnits> convertVal) const {
        return !(operator==(convertVal));
    }
};

/// scale values
template <class U, U units> struct ScaleVal : ConvertVal<U, Scale, units> {
    ScaleVal(const double x_in = 0.) : ConvertVal<U, Scale, units>(x_in) {}

    ScaleVal(const double x_in, const U fromUnits)
        : ConvertVal<U, Scale, units>(scale(fromUnits, units) * x_in) {}

    template <U fromUnits>
    ScaleVal(const ScaleVal<U, fromUnits> &scaleVal)
        : ConvertVal<U, Scale, units>(scale(fromUnits, units) * scaleVal) {}

    using ConvertVal<U, Scale, units>::x;

    double operator()(const U toUnits) const override {
        return scale(units, toUnits) * x;
    }
};

/// scale-offset values
template <class U, U units>
struct ScaleOffsetVal : ConvertVal<U, ScaleOffset, units> {
    ScaleOffsetVal(const double x_in = 0.)
        : ConvertVal<U, ScaleOffset, units>(x_in) {}

    ScaleOffsetVal(const double x_in, const U fromUnits)
        : ConvertVal<U, Scale, units>(scaleOffset(fromUnits, units, x_in)) {}

    template <U fromUnits>
    ScaleOffsetVal(const ScaleVal<U, fromUnits> &scaleOffsetVal)
        : ConvertVal<U, Scale, units>(scaleOffset(fromUnits, units) *
                                      scaleOffsetVal) {}

    using ConvertVal<U, ScaleOffset, units>::x;
    double operator()(const U toUnits) const override {
        return scaleOffset(units, toUnits) * x;
    }
};

/// scale pairs
template <class U, U units0, U units1>
struct ScalePair : std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>> {
    ScalePair(const double x0_in = 0., const double x1_in = 0.)
        : std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>({x0_in, x1_in}) {}

    double operator()(const U toUnits) const {
        return first(toUnits) + second(toUnits);
    }

    using std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>::first;
    using std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>::second;
};

/// conversion vectors
template <class U, U units> struct ScaleVect : std::vector<ScaleVal<U, units>> {

    using std::vector<ScaleVal<U, units>>::clear;
    using std::vector<ScaleVal<U, units>>::push_back;
    ScaleVect(const std::vector<double> &xV, const U fromUnits = units) {
        clear();
        for (auto x : xV)
            push_back(scale(fromUnits, units) * x);
    }

    template <U fromUnits>
    ScaleVect(const std::vector<ScaleVal<U, fromUnits>> &sV) {
        clear();
        for (auto s : sV)
            push_back(scale(fromUnits, units) * s);
    }

    operator std::vector<double>() const {
        std::vector<double> xV;
        for (auto &s : *this)
            xV.push_back(s);
        return xV;
    }

    std::vector<double> operator()(const U toUnits) const {
        std::vector<double> xV;
        for (auto &x : *this)
            xV.push_back(scale(units, toUnits) * x);
        return xV;
    }

    template <U toUnits>
    bool operator==(const ScaleVect<U, toUnits> scaleVect) const {
        return (scaleVect(units) == fV(units));
    }

    template <U toUnits>
    bool operator!=(const ScaleVect<U, toUnits> scaleVect) const {
        return !(operator==(scaleVect));
    }
};

/* time units */
enum class Time {
    h,   // hours
    min, // minutes
    s    // seconds
};

/* temperature units */
enum class Temp {
    C, // celsius
    F  // fahrenheit
};

/* energy units */
enum class Energy {
    kJ,  // kilojoules
    kWh, // kilowatt hours
    Btu, // british thermal units
    J    // joules
};

/* power units */
enum class Power {
    kW,        // kilowatts
    Btu_per_h, // BTU per hour
    W,         // watts
    kJ_per_h,  // kilojoules per hour
};

/* length units */
enum class Length {
    m, // meters
    ft // feet
};

/* area units */
enum class Area {
    m2, // square meters
    ft2 // square feet
};

/* volume units */
enum class Volume {
    L,   // liters
    gal, // gallons
    m3,  // cubic meters
    ft3  // cubic feet
};

/* flow-rate units */
enum class FlowRate {
    L_per_s,    // liters per second
    gal_per_min // gallons per minute
};

/* UA units */
enum class UA {
    kJ_per_hC, // kilojoules per hour degree celsius
    Btu_per_hF // british thermal units per hour degree Fahrenheit
};

/// reference conversion factors
constexpr double s_per_min = 60.;    // s / min
constexpr double min_per_h = 60.;    // min / h
constexpr double in3_per_gal = 231.; // in^3 / gal (U.S., exact)
constexpr double m_per_in = 0.0254;  // m / in (exact)
constexpr double F_per_C = 1.8;      // degF / degC
constexpr double offsetF = 32.;      // degF offset
constexpr double kJ_per_Btu =
    1.05505585262; // kJ / Btu (IT), https://www.unitconverters.net/
constexpr double L_per_m3 = 1000.; // L / m^3

/// derived conversion factors
constexpr double s_per_h = s_per_min * min_per_h; // s / h
constexpr double ft_per_m = 1. / m_per_in / 12.;  // ft / m
constexpr double gal_per_L =
    1.e-3 / in3_per_gal / m_per_in / m_per_in / m_per_in;            // gal / L
constexpr double ft3_per_L = ft_per_m * ft_per_m * ft_per_m / 1000.; // ft^3 / L

/// conversion maps
template <>
inline Scaler<Length>::ScaleMap
    Scaler<Length>::scaleMap(Length::m, {{Length::ft, ft_per_m}});

template <>
inline Scaler<Time>::ScaleMap Scaler<Time>::scaleMap(
    Time::s, {{Time::min, 1. / s_per_min}, {Time::h, 1. / s_per_h}});

template <>
inline Scaler<Temp>::ScaleMap Scaler<Temp>::scaleMap(Temp::C,
                                                     {{Temp::F, F_per_C}});

template <>
inline ScaleOffseter<Temp>::ScaleOffsetMap
    ScaleOffseter<Temp>::scaleOffsetMap(Temp::C,
                                        {{Temp::F, {F_per_C, offsetF}}});

template <>
inline Scaler<Energy>::ScaleMap Scaler<Energy>::scaleMap(
    Energy::kJ,
    {{Energy::kWh, s_per_h}, {Energy::Btu, kJ_per_Btu}, {Energy::J, 1000.}});

template <>
inline Scaler<Power>::ScaleMap
    Scaler<Power>::scaleMap(Power::kW,
                            {{Power::Btu_per_h, scale(Energy::kJ, Energy::Btu) /
                                                    scale(Time::s, Time::h)},
                             {Power::W, 1000.},
                             {Power::kJ_per_h, 1.}});

template <>
inline Scaler<Area>::ScaleMap Scaler<Area>::scaleMap(
    Area::m2, {{Area::ft2, Scale(std::pow(scale(Length::m, Length::ft), 2.))}});

template <>
inline Scaler<Volume>::ScaleMap
    Scaler<Volume>::scaleMap(Volume::L, {{Volume::gal, gal_per_L},
                                         {Volume::m3, 1. / L_per_m3},
                                         {Volume::ft3, ft3_per_L}});

template <>
inline Scaler<UA>::ScaleMap
    Scaler<UA>::scaleMap(UA::kJ_per_hC,
                         {{UA::Btu_per_hF, scale(Energy::kJ, Energy::Btu) *
                                               scale(Temp::C, Temp::F)}});

template <>
inline Scaler<FlowRate>::ScaleMap Scaler<FlowRate>::scaleMap(
    FlowRate::L_per_s,
    {{FlowRate::gal_per_min,
      scale(Volume::L, Volume::gal) / scale(Time::s, Time::min)}});

/// units-values partial specializations
template <Time units> using TimeVal = ScaleVal<Time, units>;
template <Temp units> using TempVal = ScaleVal<Temp, units>;
template <Temp units> using TempDiffVal = ScaleOffsetVal<Temp, units>;
template <Energy units> using EnergyVal = ScaleVal<Energy, units>;
template <Power units> using PowerVal = ScaleVal<Power, units>;
template <Length units> using LengthVal = ScaleVal<Length, units>;
template <Area units> using AreaVal = ScaleVal<Area, units>;
template <Volume units> using VolumeVal = ScaleVal<Volume, units>;
template <FlowRate units> using FlowRateVal = ScaleVal<FlowRate, units>;
template <UA units> using UAVal = ScaleVal<UA, units>;

/// units-pair partial specialization
template <Time units0, Time units1>
using TimePair = ScalePair<Time, units0, units1>;

/// units-vectors partial specialization
template <Power units> using PowerVect = ScaleVect<Power, units>;

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

/// units-pair full specialization
typedef TimePair<Time::h, Time::min> Time_h_min;

/// units-vectors full specialization
typedef PowerVect<Power::kW> PowerVect_kW;

/// convenience funcs
auto inline F_to_C() { return scaleOffset(Temp::F, Temp::C); }
auto inline C_to_F() { return scaleOffset(Temp::C, Temp::F); }

auto inline dF_to_dC() { return scale(Temp::F, Temp::C); }
auto inline dC_to_dF() { return scale(Temp::C, Temp::F); }

auto inline KJ_to_KWH() { return scale(Energy::kJ, Energy::kWh); }
auto inline KWH_to_KWH() { return scale(Energy::kWh, Energy::kJ); }

auto inline MIN_to_S() { return scale(Time::min, Time::s); }
auto inline MIN_to_H() { return scale(Time::min, Time::h); }

auto inline GAL_to_L() { return scale(Volume::gal, Volume::L); }
auto inline L_to_GAL() { return scale(Volume::L, Volume::gal); }

auto inline GPM_to_LPS() { return scale(FlowRate::gal_per_min, FlowRate::L_per_s); }
} // namespace Units

#endif
