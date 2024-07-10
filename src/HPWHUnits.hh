#ifndef HPWH_UNITS_
#define HPWH_UNITS_

#include <iterator>
#include <unordered_map>
#include <vector>

//////////////////////
/// units specifications
namespace Units {

template <typename D,typename T,typename Ti = T> struct Transform {
    D data;

    Transform(const D &data_in) : data(data_in) {}

    operator D() const { return data; }

    static auto ident() {T::ident();}

    virtual Ti inverse() const = 0;

    virtual double operator*(const double x) const = 0;

    friend double operator/(const double x, const Transform& t ) { return t.inverse() * x;}

};

struct Offset;
struct ScaleOffset;

struct Scale : Transform<double, Scale> {
    Scale(const double scale) : Transform(scale) {}

    static Scale ident() { return 1.; }

    Scale inverse() const override{ return 1. / data; }

    double operator*(const double x) const override { return data * x; }

    Scale operator*(const Scale &s) const { return data * s; }

    double scale() const { return data; }
};

struct Offset : Transform<double, Offset> {
    using Transform<double, Offset>::data;

    Offset(const double offset) : Transform(offset) {}

    operator double() { return data; }

    static Offset ident() { return 0.; }

    Offset inverse() const override { return -data; }

    double operator*(const double x) const override { return x + data; }

    Offset operator*(const Offset &o) const { return data + o.data; }

    Offset operator/(const Offset &o) const { return data - o.data; }

    double offset() const { return data; }
};

using ScaleOffsetSeq = std::pair<Scale, Offset>;

struct ScaleOffset : Transform<ScaleOffsetSeq, ScaleOffset> {
    ScaleOffset(const ScaleOffsetSeq &scaleOffsetSeq)
        : Transform(scaleOffsetSeq) {}

    ScaleOffset(const Scale &scale, const Offset &offset)
        : Transform({scale, offset}) {}

    static ScaleOffset ident() { return {1., 0.}; }

    ScaleOffset inverse() const {
        return {data.first.inverse(), data.first.inverse() * data.second.inverse()};
    }

    double operator*(const double x) const {
        return data.second * (data.first * x);
    }

    ScaleOffset operator*(const ScaleOffset &a) const {
        return {data.first * a.data.first,
                Offset(data.first.scale() * a.data.second.offset() +
                       data.second.offset())};
    }

    Scale scale() const { return data.first; }

    Offset offset() const { return data.second; }
};

template <typename U> struct PairHash {
    std::size_t operator()(const std::pair<U, U> &p) const {
        auto h1 = static_cast<std::size_t>(p.first);
        auto h2 = static_cast<std::size_t>(p.second);
        return h1 ^ h2;
    }
};

/// Transformer classes
template <typename U, typename T> struct Transformer {

    struct TransformMap
        : std::unordered_map<std::pair<U, U>, T, PairHash<U>> {

        using std::unordered_map<std::pair<U, U>, T, PairHash<U>>::insert;
        using std::unordered_map<std::pair<U, U>, T, PairHash<U>>::at;

        TransformMap(const U uRef,
                     const std::vector<std::pair<U, T>> &transforms) {
            insert({{uRef, uRef}, T::ident()});
            for (auto transform0 = transforms.begin();
                 transform0 != transforms.end(); ++transform0) {
                auto t0 = transform0->first;  // e.g., Time::min
                auto f0 = transform0->second; // e.g., 1. / s_per_min
                insert({{t0, t0}, T::ident()});    // e.g., 1.
                insert({{t0, uRef}, f0.inverse()});
                insert({{uRef, t0}, f0});

                for (auto transform1 = transform0 + 1;
                     transform1 != transforms.end(); ++transform1) {
                    auto t1 = transform1->first;      // e.g., Time::h
                    auto f1 = transform1->second;     // t1 -> (standard)
                    insert({{t0, t1}, f1 * f0.inverse()}); // t0 -> (standard) -> t1
                    insert({{t1, t0}, f0 * f1.inverse()}); // t1 -> (standard) -> t0
                }
            }
        }

        T transform(const U fromUnits, const U toUnits) {
            return at({fromUnits, toUnits});
        }
    };
};

template <typename U> struct Scaler : Transformer<U, Scale> {

    static struct ScaleMap
        : Transformer<U, Scale>::TransformMap {

        ScaleMap(
            const U tRef,
            const std::vector<std::pair<U, Scale>> &transforms)
            : Transformer<U, Scale>::TransformMap(
                  tRef, transforms) {}

    } scaleMap;

    static Scale transform(const U fromUnits,
                           const U toUnits) {
        return scaleMap.transform(fromUnits, toUnits);
    }
};

template <typename U> struct ScaleOffseter : Transformer<U, ScaleOffset> {

    static struct ScaleOffsetMap
        : Transformer<U, ScaleOffset>::TransformMap {

        ScaleOffsetMap(
            const U tRef,
            const std::vector<std::pair<U, ScaleOffset>> &transforms)
            : Transformer<U, ScaleOffset>::TransformMap(tRef,
                                                        transforms) {}

    } scaleOffsetMap;

    inline static ScaleOffset transform(const U fromUnits, const U toUnits) {
        return scaleOffsetMap.transform(fromUnits, toUnits);
    }
};

/// front-facing fncs
template <class U> Scale scale(const U fromUnits, const U toUnits) {
    return Scaler<U>::transform(fromUnits, toUnits);
}

template <class U> ScaleOffset scaleOffset(const U fromUnits, const U toUnits) {
    return ScaleOffseter<U>::transform(fromUnits, toUnits);
}

template <class U>
double scale(const U fromUnits, const U toUnits, const double x) {
    return scale(fromUnits, toUnits) * x;
}

template <class U>
double scaleOffset(const U fromUnits, const U toUnits, const double x) {
    return scaleOffset(fromUnits, toUnits) * x;
}

/// transform values
template <class U, typename T, U units> struct TransformVal {
  protected:
    double x;

  public:
    TransformVal(const double x_in = 0.) : x(x_in) {}

    template <U fromUnits>
    TransformVal(const TransformVal<U, T, fromUnits> transformVal)
        : x(transformVal.to(units)) {}

    virtual ~TransformVal() {}

    virtual double operator()(const U toUnits) const = 0;

    operator double() const { return x; }

    double operator=(double x_in) { return x = x_in; }


    template <U toUnits>
    bool operator==(const TransformVal<U, T, toUnits> transformVal) const {
        return (transformVal.to(units) == x);
    }

    template <U toUnits>
    bool operator!=(const TransformVal<U, T, toUnits> transformVal) const {
        return !(operator==(transformVal));
    }
};

template <class U, U units> struct ScaleVal : TransformVal<U, Scale, units> {
    ScaleVal(const double x_in = 0.) : TransformVal<U, Scale, units>(x_in) {}

    ScaleVal(const double x_in, const U fromUnits)
        : TransformVal<U, Scale, units>(scale(fromUnits, units) * x_in) {}

    template <U fromUnits>
    ScaleVal(const ScaleVal<U, fromUnits> &scaleVal)
        : TransformVal<U, Scale, units>(scale(fromUnits, units) * scaleVal) {}

    template <U toUnits>
    auto operator()() {return ScaleVal<U, toUnits>(x, units);}

    using TransformVal<U, Scale, units>::x;

    double operator()(const U toUnits) const override {
        return scale(units, toUnits) * x;
    }
};

template <class U, U units>
struct ScaleOffsetVal : TransformVal<U, ScaleOffset, units> {
    ScaleOffsetVal(const double x_in = 0.)
        : TransformVal<U, ScaleOffset, units>(x_in) {}

    ScaleOffsetVal(const double x_in, const U fromUnits)
        : TransformVal<U, ScaleOffset, units>(
              scaleOffset(fromUnits, units, x_in)) {}

    template <U fromUnits>
    ScaleOffsetVal(const ScaleOffsetVal<U, fromUnits> &scaleOffsetVal)
        : TransformVal<U, ScaleOffset, units>(scaleOffset(fromUnits, units) *
                                              scaleOffsetVal) {}

    using TransformVal<U, ScaleOffset, units>::x;

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

/// transform vectors
template <class U, U units> struct ScaleVect : std::vector<ScaleVal<U, units>> {

    using std::vector<ScaleVal<U, units>>::clear;
    using std::vector<ScaleVal<U, units>>::push_back;

    ScaleVect(const std::vector<double> &xV = {}, const U fromUnits = units) :
        std::vector<ScaleVal<U, units>>({}){
        for (auto x : xV)
            push_back(scale(fromUnits, units) * x);
    }

    template <U fromUnits>
    ScaleVect(const std::vector<ScaleVal<U, fromUnits>> &sV):ScaleVect({}, fromUnits) {
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

template <class U, U units>
struct ScaleOffsetVect : std::vector<ScaleOffsetVal<U, units>> {

    using std::vector<ScaleOffsetVal<U, units>>::clear;
    using std::vector<ScaleOffsetVal<U, units>>::push_back;

    ScaleOffsetVect(const std::vector<double> &xV = {}, const U fromUnits = units):std::vector<ScaleOffsetVal<U, units>>({}) {
        for (auto x : xV)
            push_back(scaleOffset(fromUnits, units) * x);
    }

    template <U fromUnits>
    ScaleOffsetVect(const std::vector<ScaleOffsetVal<U, fromUnits>> &sV):ScaleOffsetVect(sV, fromUnits) {
        for (auto s : sV)
            push_back(scaleOffset(fromUnits, units) * s);
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
            xV.push_back(scaleOffset(units, toUnits) * x);
        return xV;
    }

    template <U toUnits>
    bool operator==(const ScaleOffsetVect<U, toUnits> scaleOffsetVect) const {
        return (scaleOffsetVect(units) == fV(units));
    }

    template <U toUnits>
    bool operator!=(const ScaleOffsetVect<U, toUnits> scaleOffsetVect) const {
        return !(operator==(scaleOffsetVect));
    }
};
//////////////////////

/* time units */
enum class Time {
    h,   // hours
    min, // minutes
    s    // seconds
} inline h = Time::h,
         min = Time::min, s = Time::s;

/* temperature units */
enum class Temp {
    C, // celsius
    F  // fahrenheit
} inline C = Temp::C,
         F = Temp::F;

/* length units */
enum class Length {
    m, // meters
    ft // feet
} inline m = Length::m,
         ft = Length::ft;

/* area units */
enum class Area {
    m2, // square meters
    ft2 // square feet
} inline m2 = Area::m2,
         ft2 = Area::ft2;

/* volume units */
enum class Volume {
    L,   // liters
    gal, // gallons
    m3,  // cubic meters
    ft3  // cubic feet
} inline L = Volume::L,
         gal = Volume::gal, m3 = Volume::m3, ft3 = Volume::ft3;

/* energy units */
enum class Energy {
    kJ,  // kilojoules
    kWh, // kilowatt hours
    Btu, // british thermal units
    J    // joules
} inline kJ = Energy::kJ,
         kWh = Energy::kWh, Btu = Energy::Btu, J = Energy::J;

/* power units */
enum class Power {
    kW,        // kilowatts
    Btu_per_h, // BTU per hour
    W,         // watts
    kJ_per_h,  // kilojoules per hour
} inline kW = Power::kW,
         Btu_per_h = Power::Btu_per_h, W = Power::W, kJ_per_h = Power::kJ_per_h;

/* flow-rate units */
enum class FlowRate {
    L_per_s,    // liters per second
    gal_per_min // gallons per minute
} inline L_per_s = FlowRate::L_per_s,
         gal_per_min = FlowRate::gal_per_min;

/* UA units */
enum class UA {
    kJ_per_hC, // kilojoules per hour degree celsius
    Btu_per_hF // british thermal units per hour degree Fahrenheit
} inline kJ_per_hC = UA::kJ_per_hC,
         Btu_per_hF = UA::Btu_per_hF;

/// reference transform factors
constexpr double s_per_min = 60.;    // s / min
constexpr double min_per_h = 60.;    // min / h
constexpr double in3_per_gal = 231.; // in^3 / gal (U.S., exact)
constexpr double m_per_in = 0.0254;  // m / in (exact)
constexpr double F_per_C = 1.8;      // degF / degC
constexpr double offsetF = 32.;      // degF offset
constexpr double kJ_per_Btu =
    1.05505585262; // kJ / Btu (IT), https://www.unitconverters.net/
constexpr double L_per_m3 = 1000.; // L / m^3

/// derived transform factors
constexpr double s_per_h = s_per_min * min_per_h; // s / h
constexpr double ft_per_m = 1. / m_per_in / 12.;  // ft / m
constexpr double gal_per_L =
    1.e-3 / in3_per_gal / m_per_in / m_per_in / m_per_in;            // gal / L
constexpr double ft3_per_L = ft_per_m * ft_per_m * ft_per_m / 1000.; // ft^3 / L

/// transform maps
template <>
inline Scaler<Time>::ScaleMap Scaler<Time>::scaleMap(
    s, {{min, 1. / s_per_min}, {h, 1. / s_per_h}});

template <>
inline Scaler<Length>::ScaleMap
    Scaler<Length>::scaleMap(m, {{ft, ft_per_m}});

template <>
inline Scaler<Temp>::ScaleMap Scaler<Temp>::scaleMap(C,
                                                     {{F, F_per_C}});

template <>
inline ScaleOffseter<Temp>::ScaleOffsetMap
    ScaleOffseter<Temp>::scaleOffsetMap(C,
                                        {{F, {F_per_C, offsetF}}});

template <>
inline Scaler<Energy>::ScaleMap Scaler<Energy>::scaleMap(
    kJ,
    {{kWh, s_per_h}, {Btu, 1. / kJ_per_Btu}, {J, 1000.}});

template <>
inline Scaler<Power>::ScaleMap
    Scaler<Power>::scaleMap(kW,
                            {{Btu_per_h, scale(kJ, Btu) /
                                             scale(s, h)},
                             {W, 1000.},
                             {kJ_per_h, 1.}});

template <>
inline Scaler<Area>::ScaleMap Scaler<Area>::scaleMap(
    m2, {{ft2, Scale(std::pow(scale(m, ft), 2.))}});

template <>
inline Scaler<Volume>::ScaleMap
    Scaler<Volume>::scaleMap(L, {{gal, gal_per_L},
                                 {m3, 1. / L_per_m3},
                                 {ft3, ft3_per_L}});

template <>
inline Scaler<UA>::ScaleMap
    Scaler<UA>::scaleMap(kJ_per_hC,
                         {{Btu_per_hF, scale(kJ, Btu) *
                                           scale(C, F)}});

template <>
inline Scaler<FlowRate>::ScaleMap Scaler<FlowRate>::scaleMap(
    L_per_s,
    {{gal_per_min,
      scale(L, gal) / scale(s,min)}});

/// units-values partial specializations
template <Time units> using TimeVal = ScaleVal<Time, units>;
template <Temp units> using TempVal = ScaleOffsetVal<Temp, units>;
template <Temp units> using dTempVal = ScaleVal<Temp, units>;
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
template <Time units> using TimeVect = ScaleVect<Time, units>;
template <Temp units> using TempVect = ScaleOffsetVect<Temp, units>;
template <Energy units> using EnergyVect = ScaleVect<Energy, units>;
template <Power units> using PowerVect = ScaleVect<Power, units>;

/// units-values full specializations
typedef TimeVal<Time::s> Time_s;
typedef TimeVal<Time::min> Time_min;
typedef TempVal<Temp::C> Temp_C, T_C;
typedef TempVal<Temp::F> Temp_F, T_F;
typedef dTempVal<Temp::C> dTemp_C, dT_C;
typedef EnergyVal<Energy::kJ> Energy_kJ, E_kJ;
typedef PowerVal<Power::kW> Power_kW, P_kW;
typedef LengthVal<Length::m> Length_m, L_m;
typedef AreaVal<Area::m2> Area_m2, A_m2;
typedef VolumeVal<Volume::L> Volume_L, V_L;
typedef FlowRateVal<FlowRate::L_per_s> FlowRate_L_per_s, FR_L_per_s;
typedef UAVal<UA::kJ_per_hC> UA_kJ_per_hC;

/// units-pair full specialization
typedef TimePair<Time::h, Time::min> Time_h_min;

/// units-vectors full specialization
typedef TimeVect<Time::s> TimeVect_s, TimeV_s;
typedef TimeVect<Time::min> TimeVect_min, TimeV_min;
typedef TempVect<Temp::C> TempVect_C, TempV_C;
typedef EnergyVect<Energy::kJ> EnergyVect_kJ, EnergyV_kJ;
typedef PowerVect<Power::kW> PowerVect_kW, PowerV_kW;

} // namespace Units

#endif
