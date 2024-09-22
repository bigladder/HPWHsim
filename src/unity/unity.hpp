#ifndef _UNITY_
#define _UNITY_

#include <iterator>
#include <unordered_map>
#include <map>
#include <utility>
#include <vector>
#include <cmath>
#include <functional>
#include <algorithm>

/// units data structures
namespace Unity
{

template <typename Data_t, typename Transform_t, typename Transform_inv_t = Transform_t>
struct Transform
{
    Data_t data;

    explicit Transform(Data_t data_in) : data(std::move(data_in)) {}

    Data_t operator()() const { return data; }
    Data_t& operator()() { return data; }

    static auto ident() { Transform_t::ident(); }

    [[nodiscard]] virtual Transform_inv_t inverse() const = 0;

    virtual double operator*(double x) const = 0;

    friend double operator/(const double x, const Transform& t) { return t.inverse() * x; }
};

struct Shift;
struct ScaleShift;

struct Scale : Transform<double, Scale>
{
    explicit Scale(const double scale) : Transform(scale) {}

    static Scale ident() { return Scale(1.); }

    [[nodiscard]] Scale inverse() const override { return Scale(1. / data); }

    double operator*(const double x) const override { return data * x; }

    Scale operator*(const Scale& s_) const { return Scale(data * s_()); }

    Scale operator/(const Scale& s_) const { return Scale(data / s_()); }

    [[nodiscard]] double scale() const { return data; }
};

struct Shift : Transform<double, Shift>
{
    using Transform<double, Shift>::data;

    explicit Shift(const double offset) : Transform(offset) {}

    static Shift ident() { return Shift(0.); }

    [[nodiscard]] Shift inverse() const override { return Shift(-data); }

    double operator*(const double x) const override { return x + data; }

    Shift operator*(const Shift& o) const { return Shift(data + o.data); }

    Shift operator/(const Shift& o) const { return Shift(data - o.data); }

    [[nodiscard]] double shift() const { return data; }
};

using ScaleShiftSeq = std::pair<Scale, Shift>;

struct ScaleShift : Transform<ScaleShiftSeq, ScaleShift>
{
    explicit ScaleShift(const ScaleShiftSeq& scaleShiftSeq) : Transform(scaleShiftSeq) {}

    ScaleShift(const Scale& scale, const Shift& shift) : Transform({scale, shift}) {}

    static ScaleShift ident() { return {Scale(1.), Shift(0.)}; }

    [[nodiscard]] ScaleShift inverse() const override
    {
        return {Scale(data.first.inverse()),
                Shift(data.first.inverse()() * data.second.inverse()())};
    }

    double operator*(const double x) const override { return data.second * (data.first * x); }

    ScaleShift operator*(const ScaleShift& a) const
    {
        return {data.first * a.data.first,
                Shift(data.first.scale() * a.data.second.shift() + data.second.shift())};
    }

    [[nodiscard]] Scale scale() const { return data.first; }

    [[nodiscard]] Shift shift() const { return data.second; }
};

/// Transformer classes
template <typename Units_t, typename Transform_t>
struct Transformer
{
    struct TransformMap : std::map<std::pair<Units_t, Units_t>, Transform_t>
    {
        using UPair_t = std::pair<Units_t, Units_t>;
        using std::map<UPair_t, Transform_t>::insert;
        using std::map<UPair_t, Transform_t>::at;

        TransformMap(const Units_t uRef,
                     const std::vector<std::pair<Units_t, Transform_t>>& transforms)
        {
            insert({{uRef, uRef}, Transform_t::ident()});
            for (auto transform0 = transforms.begin(); transform0 != transforms.end(); ++transform0)
            {
                auto t0 = transform0->first;  // e.g., Time::min
                auto f0 = transform0->second; // e.g., 1. / s_per_min
                // insert({{t0, t0}, T::ident()}); // e.g., 1.
                insert({{t0, uRef}, f0.inverse()});
                insert({{uRef, t0}, f0});

                for (auto transform1 = transform0 + 1; transform1 != transforms.end(); ++transform1)
                {
                    auto t1 = transform1->first;           // e.g., Time::h
                    auto f1 = transform1->second;          // t1 -> (standard)
                    insert({{t0, t1}, f1 * f0.inverse()}); // t0 -> (standard) -> t1
                    insert({{t1, t0}, f0 * f1.inverse()}); // t1 -> (standard) -> t0
                }
            }
        }

        inline Transform_t transform(const Units_t fromUnits, const Units_t toUnits)
        {
            return (fromUnits != toUnits) ? at({fromUnits, toUnits}) : Transform_t::ident();
        }
    };
};

template <typename Units_t>
struct Scaler : Transformer<Units_t, Scale>
{
    static struct ScaleMap : Transformer<Units_t, Scale>::TransformMap
    {
        ScaleMap(const Units_t tRef, const std::vector<std::pair<Units_t, Scale>>& transforms)
            : Transformer<Units_t, Scale>::TransformMap(tRef, transforms)
        {
        }

    } scaleMap;

    inline static Scale transform(const Units_t fromUnits, const Units_t toUnits)
    {
        return scaleMap.transform(fromUnits, toUnits);
    }
};

template <typename Units_t>
struct ScaleShifter : Transformer<Units_t, ScaleShift>
{
    static struct ScaleShiftMap : Transformer<Units_t, ScaleShift>::TransformMap
    {
        ScaleShiftMap(const Units_t tRef,
                      const std::vector<std::pair<Units_t, ScaleShift>>& transforms)
            : Transformer<Units_t, ScaleShift>::TransformMap(tRef, transforms)
        {
        }

    } scaleShiftMap;

    inline static ScaleShift transform(const Units_t fromUnits, const Units_t toUnits)
    {
        return scaleShiftMap.transform(fromUnits, toUnits);
    }
};

/// front-facing fncs
template <class Units_t>
Scale scale(const Units_t fromUnits, const Units_t toUnits)
{
    return Scaler<Units_t>::transform(fromUnits, toUnits);
}

template <class Units_t>
ScaleShift scaleShift(const Units_t fromUnits, const Units_t toUnits)
{
    return ScaleShifter<Units_t>::transform(fromUnits, toUnits);
}

template <class Units_t>
double scale(const Units_t fromUnits, const Units_t toUnits, const double x)
{
    return scale(fromUnits, toUnits) * x;
}

template <class Units_t>
double scaleShift(const Units_t fromUnits, const Units_t toUnits, const double x)
{
    return scaleShift(fromUnits, toUnits) * x;
}

/// transform values
template <class Units_t, Units_t units, typename Transform_t, template <class, Units_t> class Val_t>
struct TransformVal
{
  protected:
    double x;

  public:
    explicit TransformVal(const double x_in = 0.) : x(x_in) {}

    template <Units_t fromUnits>
    explicit TransformVal(const TransformVal<Units_t, fromUnits, Transform_t, Val_t>& transformVal)
        : TransformVal(Val_t<Units_t, fromUnits>(transformVal())(units))
    {
    }

    ~TransformVal() = default;

    explicit operator const double&() const { return x; }
    explicit operator double&() { return x; }

    double operator()() const { return x; }
    double& operator()() { return x; }

    bool operator==(const double y) const { return (x == y); }
    bool operator!=(const double y) const { return !(operator==(y)); }

    bool operator<(const double y) const { return x < y; }
    bool operator>(const double y) const { return x > y; }
    bool operator<=(const double y) const { return x <= y; }
    bool operator>=(const double y) const { return x >= y; }

    auto& operator=(const double y)
    {
        x = y;
        return *this;
    }

    auto operator+=(const double y)
    {
        x += y;
        return *this;
    }

    auto operator-=(const double y)
    {
        x -= y;
        return *this;
    }

    auto& operator+=(const Val_t<Units_t, units>& val)
    {
        x += val();
        return *this;
    }

    auto& operator-=(const Val_t<Units_t, units>& scaleVal)
    {
        x -= scaleVal();
        return *this;
    }

    template <Units_t toUnits>
    bool operator==(const Val_t<Units_t, toUnits> val) const
    {
        return (val(units) == x);
    }

    template <Units_t toUnits>
    bool operator!=(const Val_t<Units_t, toUnits>& val) const
    {
        return !(operator==(val));
    }

    template <Units_t toUnits>
    bool operator<(const Val_t<Units_t, toUnits> val) const
    {
        return (x < val(units));
    }

    template <Units_t toUnits>
    bool operator>(const Val_t<Units_t, toUnits> val) const
    {
        return (x > val(units));
    }

    template <Units_t toUnits>
    bool operator<=(const Val_t<Units_t, toUnits> val) const
    {
        return (x <= val(units));
    }

    template <Units_t toUnits>
    bool operator>=(const Val_t<Units_t, toUnits> val) const
    {
        return (x >= val(units));
    }
    Units_t in() { return units; }
};

template <class Units_t, Units_t units>
struct ScaleVal : TransformVal<Units_t, units, Scale, ScaleVal>
{
    using TVal = TransformVal<Units_t, units, Scale, ScaleVal>;
    using TVal::x;
    using TVal::operator<;
    using TVal::operator>;
    using TVal::operator<=;
    using TVal::operator>=;
    using TVal::operator==;
    using TVal::operator!=;
    using TVal::operator=;
    using TVal::operator+=;
    using TVal::operator-=;
    using TVal::operator();

    explicit ScaleVal(const double x_in = 0.) : TVal(x_in) {}

    template <Units_t fromUnits>
    explicit ScaleVal(const ScaleVal<Units_t, fromUnits>& scaleVal) : TVal(scaleVal(units))
    {
    }

    explicit ScaleVal(const TVal& transformVal) : TVal(transformVal) {}

    ScaleVal<Units_t, units>(const double x_in, const Units_t fromUnits)
        : TVal(scale(fromUnits, units) * x_in)
    {
    }

    double operator()(const Units_t toUnits) const { return scale(units, toUnits) * x; }

    template <Units_t fromUnits>
    ScaleVal& operator=(const ScaleVal<Units_t, fromUnits>& scaleVal)
    {
        x = scaleVal(units);
        return *this;
    }

    auto operator+(const double y) const { return ScaleVal(x + y); }
    auto operator-(const double y) const { return ScaleVal(x - y); }
    auto operator/(const double y) const { return ScaleVal(x / y); }

    friend auto operator*(const double y, const ScaleVal& scaleVal)
    {
        return ScaleVal(y * scaleVal());
    }

    double operator/(const ScaleVal& scaleVal) const { return x / scaleVal(); }

    ScaleVal operator+(const ScaleVal& scaleVal) const { return ScaleVal(x + scaleVal()); }
    ScaleVal operator-(const ScaleVal& scaleVal) const { return ScaleVal(x - scaleVal()); }

    ScaleVal& operator*=(const double y)
    {
        x = y * x;
        return *this;
    }
    ScaleVal& operator/=(const double y)
    {
        x = x / y;
        return *this;
    }

    template <Units_t fromUnits>
    ScaleVal operator+(const ScaleVal<Units_t, fromUnits>& scaleVal) const
    {
        return x + scaleVal(units);
    }

    template <Units_t fromUnits>
    ScaleVal operator-(const ScaleVal<Units_t, fromUnits>& scaleVal) const
    {
        return x - scaleVal(units);
    }
};

template <class Units_t, Units_t units>
struct ScaleShiftVal : TransformVal<Units_t, units, ScaleShift, ScaleShiftVal>
{
    using TVal = TransformVal<Units_t, units, ScaleShift, ScaleShiftVal>;
    using TVal::x;
    using TVal::operator<;
    using TVal::operator>;
    using TVal::operator<=;
    using TVal::operator>=;
    using TVal::operator==;
    using TVal::operator!=;
    using TVal::operator=;
    using TVal::operator+=;
    using TVal::operator-=;
    using TVal::operator();

    explicit ScaleShiftVal<Units_t, units>(const double x_in = 0.) : TVal(x_in) {}

    template <Units_t fromUnits>
    explicit ScaleShiftVal(const ScaleShiftVal<Units_t, fromUnits>& scaleShiftVal)
        : TVal(scaleShiftVal(units))
    {
    }

    ScaleShiftVal(const double x_in, const Units_t fromUnits)
        : ScaleShiftVal<Units_t, units>(scaleShift(fromUnits, units) * x_in)
    {
    }

    explicit ScaleShiftVal(const TVal& transformVal) : TVal(transformVal) {}

    template <Units_t fromUnits>
    ScaleShiftVal& operator=(const ScaleShiftVal<Units_t, fromUnits>& scaleShiftVal)
    {
        x = scaleShiftVal(units);
        return *this;
    }

    template <Units_t fromUnits>
    ScaleShiftVal& operator+(const ScaleVal<Units_t, fromUnits>& scaleVal) const
    {
        return x + scaleVal(units);
    }

    template <Units_t fromUnits>
    ScaleShiftVal& operator-(const ScaleVal<Units_t, fromUnits>& scaleVal) const
    {
        return x - scaleVal(units);
    }

    template <Units_t fromUnits>
    ScaleShiftVal& operator+=(const ScaleVal<Units_t, fromUnits>& scaleVal)
    {
        x += scaleVal();
        return *this;
    }

    template <Units_t fromUnits>
    ScaleShiftVal& operator-=(const ScaleVal<Units_t, fromUnits>& scaleVal)
    {
        x -= scaleVal();
        return *this;
    }

    double operator()(const Units_t toUnits) const { return scaleShift(units, toUnits) * x; }
};

/// transform vectors
template <class Units_t,
          Units_t units,
          typename Transform_t,
          template <class, Units_t>
          class Vect_t,
          template <class, Units_t>
          class Val_t>
struct TransformVect
{
  protected:
    std::vector<double> xV;

  public:
    explicit TransformVect(const std::vector<double>& xV_in = {}) : xV(xV_in) {}

    explicit TransformVect(const std::size_t n) : xV(n) {}

    explicit operator std::vector<double>&() { return xV; }
    explicit operator const std::vector<double>&() const { return xV; }

    std::vector<double> operator()() const { return xV; }
    std::vector<double>& operator()() { return xV; }

    inline auto size() const { return xV.size(); }
    inline auto resize(const std::size_t n) { return xV.resize(n); }
    inline auto clear() { return xV.clear(); }
    inline auto empty() const { return xV.empty(); }

    template <Units_t toUnits>
    bool operator==(const Vect_t<Units_t, toUnits> vect) const
    {
        return (vect(units) == fV(units));
    }

    template <Units_t toUnits>
    bool operator!=(const Vect_t<Units_t, toUnits> vect) const
    {
        return !(operator==(vect));
    }

    auto operator[](const std::size_t i) const { return Val_t<Units_t, units>(xV[i]); }

    auto& operator[](const std::size_t i)
    {
        return reinterpret_cast<Val_t<Units_t, units>&>(xV[i]);
    }

    auto begin()
    {
        return xV.empty() ? nullptr : reinterpret_cast<Val_t<Units_t, units>*>(&(*xV.begin()));
    }
    auto end() { return begin() + xV.size(); }

    auto begin() const
    {
        return xV.empty() ? nullptr
                          : reinterpret_cast<const Val_t<Units_t, units>*>(&(*xV.begin()));
    }
    auto end() const { return begin() + xV.size(); }

    auto rbegin()
    {
        return xV.empty() ? nullptr : reinterpret_cast<Val_t<Units_t, units>*>(&(*xV.rbegin()));
    }
    auto rend() { return rbegin() + xV.size(); }

    auto rbegin() const
    {
        return xV.empty() ? nullptr
                          : reinterpret_cast<const Val_t<Units_t, units>*>(&(*xV.rbegin()));
    }
    auto rend() const { return rbegin() + xV.size(); }

    auto& front() { return reinterpret_cast<Val_t<Units_t, units>&>(xV.front()); }
    auto& back() { return reinterpret_cast<Val_t<Units_t, units>&>(xV.back()); }

    auto front() const { return reinterpret_cast<const Val_t<Units_t, units>&>(xV.front()); }
    auto back() const { return reinterpret_cast<const Val_t<Units_t, units>&>(xV.back()); }

    auto push_back(const Val_t<Units_t, units>& val) { xV.push_back(val()); }

    Units_t in() { return units; }
};

template <class Units_t, Units_t units>
struct ScaleVect : public TransformVect<Units_t, units, Scale, ScaleVect, ScaleVal>
{
  public:
    using TVect = TransformVect<Units_t, units, Scale, ScaleVect, ScaleVal>;
    using TVect::clear;
    using TVect::resize;
    using TVect::size;
    using TVect::xV;
    using TVect::operator();

    explicit ScaleVect(const std::vector<double>& xV_in = {}) : TVect(xV_in) {}

    template <Units_t fromUnits>
    explicit ScaleVect(const ScaleVect<Units_t, fromUnits>& sV) : ScaleVect({})
    {
        xV.reserve(sV.size());
        auto t = scale(fromUnits, units);
        for (auto s_ : sV)
            xV.push_back(t * s_());
    }

    template <Units_t fromUnits>
    explicit ScaleVect(const std::vector<ScaleVal<Units_t, fromUnits>>& sV) : ScaleVect()
    {
        xV.reserve(sV.size());
        auto t = scale(fromUnits, units);
        for (auto s_ : sV)
            xV.push_back(t * s_());
    }

    explicit ScaleVect(const std::size_t n) : TVect(n) {}

    ScaleVect(const std::vector<double>& xV_in, const Units_t fromUnits)
        : ScaleVect<Units_t, units>()
    {
        auto t = scale(fromUnits, units);
        for (auto x : xV_in)
            xV.push_back(t * x);
    }

    std::vector<double> operator()(const Units_t toUnits) const
    {
        std::vector<double> xV_out = {};
        xV_out.reserve(xV.size());
        auto t = scale(units, toUnits);
        for (auto& x : xV)
            xV_out.push_back(t * x);
        return xV_out;
    }
};

template <class Units_t, Units_t units>
struct ScaleShiftVect : TransformVect<Units_t, units, ScaleShift, ScaleShiftVect, ScaleShiftVal>
{
  public:
    using TVect = TransformVect<Units_t, units, ScaleShift, ScaleShiftVect, ScaleShiftVal>;
    using TVect::clear;
    using TVect::resize;
    using TVect::size;
    using TVect::xV;
    using TVect::operator();

    explicit ScaleShiftVect(const std::vector<double>& xV_in = {}) : TVect(xV_in) {}

    explicit ScaleShiftVect(const std::vector<ScaleShiftVal<Units_t, units>>& sV) : ScaleShiftVect()
    {
        xV = sV();
    }

    template <Units_t fromUnits>
    explicit ScaleShiftVect(const ScaleVect<Units_t, fromUnits>& sV) : ScaleShiftVect({})
    {
        xV.reserve(sV.size());
        auto t = scaleShift(fromUnits, units);
        for (auto s_ : sV)
            xV.push_back(t * s_());
    }

    template <Units_t fromUnits>
    explicit ScaleShiftVect(const std::vector<ScaleVal<Units_t, fromUnits>>& sV) : ScaleShiftVect()
    {
        xV.reserve(sV.size());
        auto t = scaleShift(fromUnits, units);
        for (auto s_ : sV)
            xV.push_back(t * s_());
    }

    explicit ScaleShiftVect(const std::size_t n) : TVect(n) {}

    ScaleShiftVect(const std::vector<double>& xV_in, const Units_t fromUnits) : ScaleShiftVect()
    {
        xV.reserve(xV_in.size());
        const auto t = scaleShift(fromUnits, units);
        for (auto x : xV_in)
            xV.push_back(t * x);
    }

    std::vector<double> operator()(const Units_t toUnits) const
    {
        std::vector<double> xV_out = {};
        xV_out.reserve(xV.size());
        auto t = scaleShift(units, toUnits);
        for (auto& x : xV)
            xV_out.push_back(t * x);
        return xV;
    }
};

/// scale pairs
template <class Units_t, Units_t units0, Units_t units1>
struct ScalePair : std::pair<ScaleVal<Units_t, units0>, ScaleVal<Units_t, units1>>
{
    ScalePair(const double x0_in = 0., const double x1_in = 0.)
        : std::pair<ScaleVal<Units_t, units0>, ScaleVal<Units_t, units1>>(
              {ScaleVal<Units_t, units0>(x0_in), ScaleVal<Units_t, units1>(x1_in)})
    {
    }

    double operator()(const Units_t toUnits) const { return first(toUnits) + second(toUnits); }

    using std::pair<ScaleVal<Units_t, units0>, ScaleVal<Units_t, units1>>::first;
    using std::pair<ScaleVal<Units_t, units0>, ScaleVal<Units_t, units1>>::second;
};

} // namespace Unity

#endif
