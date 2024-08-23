#ifndef _UNITY_
#define _UNITY_

#include <iterator>
#include <unordered_map>
#include <vector>

/// units data structures
namespace Unity
{

template <typename D, typename T, typename Ti = T>
struct Transform
{
    D data;

    Transform(const D& data_in) : data(data_in) {}

    operator D() const { return data; }

    static auto ident() { T::ident(); }

    virtual Ti inverse() const = 0;

    virtual double operator*(const double x) const = 0;

    friend double operator/(const double x, const Transform& t) { return t.inverse() * x; }
};

struct Offset;
struct ScaleOffset;

struct Scale : Transform<double, Scale>
{
    Scale(const double scale) : Transform(scale) {}

    static Scale ident() { return 1.; }

    Scale inverse() const override { return 1. / data; }

    double operator*(const double x) const override { return data * x; }

    Scale operator*(const Scale& s_) const { return data * s_; }

    double scale() const { return data; }
};

struct Offset : Transform<double, Offset>
{
    using Transform<double, Offset>::data;

    Offset(const double offset) : Transform(offset) {}

    operator double() { return data; }

    static Offset ident() { return 0.; }

    Offset inverse() const override { return -data; }

    double operator*(const double x) const override { return x + data; }

    Offset operator*(const Offset& o) const { return data + o.data; }

    Offset operator/(const Offset& o) const { return data - o.data; }

    double offset() const { return data; }
};

using ScaleOffsetSeq = std::pair<Scale, Offset>;

struct ScaleOffset : Transform<ScaleOffsetSeq, ScaleOffset>
{
    ScaleOffset(const ScaleOffsetSeq& scaleOffsetSeq) : Transform(scaleOffsetSeq) {}

    ScaleOffset(const Scale& scale, const Offset& offset) : Transform({scale, offset}) {}

    static ScaleOffset ident() { return {1., 0.}; }

    ScaleOffset inverse() const
    {
        return {data.first.inverse(), data.first.inverse() * data.second.inverse()};
    }

    double operator*(const double x) const { return data.second * (data.first * x); }

    ScaleOffset operator*(const ScaleOffset& a) const
    {
        return {data.first * a.data.first,
                Offset(data.first.scale() * a.data.second.offset() + data.second.offset())};
    }

    Scale scale() const { return data.first; }

    Offset offset() const { return data.second; }
};

template <typename U>
struct PairHash
{
    std::size_t operator()(const std::pair<U, U>& p) const
    {
        auto h1 = static_cast<std::size_t>(p.first);
        auto h2 = static_cast<std::size_t>(p.second);
        return h1 ^ h2;
    }
};

/// Transformer classes
template <typename U, typename T>
struct Transformer
{

    struct TransformMap : std::unordered_map<std::pair<U, U>, T, PairHash<U>>
    {

        using std::unordered_map<std::pair<U, U>, T, PairHash<U>>::insert;
        using std::unordered_map<std::pair<U, U>, T, PairHash<U>>::at;

        TransformMap(const U uRef, const std::vector<std::pair<U, T>>& transforms)
        {
            insert({{uRef, uRef}, T::ident()});
            for (auto transform0 = transforms.begin(); transform0 != transforms.end(); ++transform0)
            {
                auto t0 = transform0->first;    // e.g., Time::min
                auto f0 = transform0->second;   // e.g., 1. / s_per_min
                insert({{t0, t0}, T::ident()}); // e.g., 1.
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

        T transform(const U fromUnits, const U toUnits) { return at({fromUnits, toUnits}); }
    };
};

template <typename U>
struct Scaler : Transformer<U, Scale>
{

    static struct ScaleMap : Transformer<U, Scale>::TransformMap
    {

        ScaleMap(const U tRef, const std::vector<std::pair<U, Scale>>& transforms)
            : Transformer<U, Scale>::TransformMap(tRef, transforms)
        {
        }

    } scaleMap;

    static Scale transform(const U fromUnits, const U toUnits)
    {
        return scaleMap.transform(fromUnits, toUnits);
    }
};

template <typename U>
struct ScaleOffseter : Transformer<U, ScaleOffset>
{

    static struct ScaleOffsetMap : Transformer<U, ScaleOffset>::TransformMap
    {

        ScaleOffsetMap(const U tRef, const std::vector<std::pair<U, ScaleOffset>>& transforms)
            : Transformer<U, ScaleOffset>::TransformMap(tRef, transforms)
        {
        }

    } scaleOffsetMap;

    inline static ScaleOffset transform(const U fromUnits, const U toUnits)
    {
        return scaleOffsetMap.transform(fromUnits, toUnits);
    }
};

/// front-facing fncs
template <class U>
Scale scale(const U fromUnits, const U toUnits)
{
    return Scaler<U>::transform(fromUnits, toUnits);
}

template <class U>
ScaleOffset scaleOffset(const U fromUnits, const U toUnits)
{
    return ScaleOffseter<U>::transform(fromUnits, toUnits);
}

template <class U>
double scale(const U fromUnits, const U toUnits, const double x)
{
    return scale(fromUnits, toUnits) * x;
}

template <class U>
double scaleOffset(const U fromUnits, const U toUnits, const double x)
{
    return scaleOffset(fromUnits, toUnits) * x;
}

/// transform values
template <class U, typename T, U units>
struct TransformVal
{
  protected:
    double x;

  public:
    TransformVal(const double x_in = 0.) : x(x_in) {}

    template <U fromUnits>
    TransformVal(const TransformVal<U, T, fromUnits> transformVal) : x(transformVal(units))
    {
    }

    virtual ~TransformVal() {}

    virtual double operator()(const U toUnits) const = 0;

    operator double() const { return x; }

    double operator=(double x_in) { return x = x_in; }

    U in() { return units; }

    /*
    template <U fromUnits>
    bool operator>(const TransformVal<U, T, fromUnits> transformVal) const { return x >
    transformVal(units); }

    template <U fromUnits>
    bool operator<(const TransformVal<U, T, fromUnits> transformVal) const { return x <
    transformVal(units); }


    template <U fromUnits>
    bool operator>=(const TransformVal<U, T, fromUnits> transformVal) const { return x >=
    transformVal(units); }

    template <U fromUnits>
    bool operator<=(const TransformVal<U, T, fromUnits> transformVal) const { return x <=
    transformVal(units); }
*/
};

template <class U, U units>
struct ScaleVal : TransformVal<U, Scale, units>
{
    using TransformVal<U, Scale, units>::x;

    ScaleVal(const double x_in = 0.) : TransformVal<U, Scale, units>(x_in) {}

    ScaleVal(const double x_in, const U fromUnits)
        : TransformVal<U, Scale, units>(scale(fromUnits, units) * x_in)
    {
    }

    template <U fromUnits>
    ScaleVal(const ScaleVal<U, fromUnits>& scaleVal)
        : TransformVal<U, Scale, units>(scale(fromUnits, units) * scaleVal)
    {
    }

    template <U toUnits>
    auto operator()()
    {
        return ScaleVal<U, toUnits>(x, units);
    }

    double operator()(const U toUnits) const override { return scale(units, toUnits) * x; }

    ScaleVal operator+(const double y) const { return x + y; }
    ScaleVal operator-(const double y) const { return x - y; }

    ScaleVal operator+=(const double y) { return *this = *this + y; }
    ScaleVal operator-=(const double y) { return *this = *this - y; }

    ScaleVal operator*(const double y) const { return y * x; }

    template <U toUnits>
    bool operator==(const ScaleVal<U, toUnits> scaleVal) const
    {
        return (scaleVal(units) == x);
    }

    template <U toUnits>
    bool operator!=(const ScaleVal<U, toUnits> scaleVal) const
    {
        return !(operator==(scaleVal));
    }

    ScaleVal operator*=(const double y)
    {
        x = y * x;
        return *this;
    }
    ScaleVal operator/=(const double y)
    {
        x = x / y;
        return *this;
    }

    template <U fromUnits>
    ScaleVal operator+(const ScaleVal<U, fromUnits>& scaleVal) const
    {
        return x + scaleVal(units);
    }

    template <U fromUnits>
    ScaleVal operator-(const ScaleVal<U, fromUnits>& scaleVal) const
    {
        return x - scaleVal(units);
    }

    template <U fromUnits>
    ScaleVal operator+=(const ScaleVal<U, fromUnits>& scaleVal)
    {
        return *this = *this + scaleVal;
    }

    template <U fromUnits>
    ScaleVal operator-=(const ScaleVal<U, fromUnits>& scaleVal)
    {
        return *this = *this - scaleVal;
    }
};

template <class U, U units>
struct ScaleOffsetVal : TransformVal<U, ScaleOffset, units>
{
    using TransformVal<U, ScaleOffset, units>::x;

    ScaleOffsetVal(const double x_in = 0.) : TransformVal<U, ScaleOffset, units>(x_in) {}

    ScaleOffsetVal(const double x_in, const U fromUnits)
        : TransformVal<U, ScaleOffset, units>(scaleOffset(fromUnits, units, x_in))
    {
    }

    template <U fromUnits>
    ScaleOffsetVal(const ScaleOffsetVal<U, fromUnits>& scaleOffsetVal)
        : TransformVal<U, ScaleOffset, units>(scaleOffset(fromUnits, units) * scaleOffsetVal)
    {
    }

    ScaleOffsetVal operator+(const double y) const { return x + y; }
    ScaleOffsetVal operator-(const double y) const { return x - y; }

    template <U toUnits>
    bool operator==(const ScaleOffsetVal<U, toUnits> scaleOffsetVal) const
    {
        return (scaleOffsetVal(units) == x);
    }

    template <U toUnits>
    bool operator!=(const ScaleOffsetVal<U, toUnits> scaleOffsetVal) const
    {
        return !(operator==(scaleOffsetVal));
    }

    ScaleOffsetVal operator+=(const double y) { return *this = *this + y; }
    ScaleOffsetVal operator-=(const double y) { return *this = *this - y; }

    template <U fromUnits>
    ScaleOffsetVal operator+(const ScaleVal<U, fromUnits>& scaleVal) const
    {
        return x + scaleVal(units);
    }

    template <U fromUnits>
    ScaleOffsetVal operator-(const ScaleVal<U, fromUnits>& scaleVal) const
    {
        return x - scaleVal(units);
    }

    template <U fromUnits>
    ScaleOffsetVal operator+=(const ScaleVal<U, fromUnits>& scaleVal)
    {
        return *this = *this + scaleVal;
    }

    template <U fromUnits>
    ScaleOffsetVal operator-=(const ScaleVal<U, fromUnits>& scaleVal)
    {
        return *this = *this - scaleVal;
    }

    double operator()(const U toUnits) const override { return scaleOffset(units, toUnits) * x; }
};

/// scale pairs
template <class U, U units0, U units1>
struct ScalePair : std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>
{
    ScalePair(const double x0_in = 0., const double x1_in = 0.)
        : std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>({x0_in, x1_in})
    {
    }

    double operator()(const U toUnits) const { return first(toUnits) + second(toUnits); }

    using std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>::first;
    using std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>::second;
};

/// transform vectors
template <class U, U units>
struct ScaleVect : std::vector<ScaleVal<U, units>>
{
    using std::vector<ScaleVal<U, units>>::size;
    using std::vector<ScaleVal<U, units>>::clear;
    using std::vector<ScaleVal<U, units>>::push_back;
    using std::vector<ScaleVal<U, units>>::resize;

    ScaleVect(const std::vector<double>& xV = {}, const U fromUnits = units)
        : std::vector<ScaleVal<U, units>>({})
    {
        for (auto x : xV)
            push_back(scale(fromUnits, units) * x);
    }

    template <U fromUnits>
    ScaleVect(const std::vector<ScaleVal<U, fromUnits>>& sV) : ScaleVect({}, fromUnits)
    {
        for (auto s_ : sV)
            push_back(scale(fromUnits, units) * s_);
    }

    ScaleVect(const std::size_t n) : ScaleVect() { resize(n); }

    template <typename... val>
    ScaleVect(const std::tuple<val...>& sV) : ScaleVect()
    {
        for (auto s_ : sV)
            push_back(s_);
    }

    operator std::vector<double>() const
    {
        std::vector<double> xV = {};
        xV.reserve(size());
        for (auto& s_ : *this)
            xV.push_back(s_);
        return xV;
    }

    std::vector<double> operator()(const U toUnits) const
    {
        std::vector<double> xV = {};
        xV.reserve(size());
        for (auto& x : *this)
            xV.push_back(scale(units, toUnits) * x);
        return xV;
    }

    template <U toUnits>
    bool operator==(const ScaleVect<U, toUnits> scaleVect) const
    {
        return (scaleVect(units) == fV(units));
    }

    template <U toUnits>
    bool operator!=(const ScaleVect<U, toUnits> scaleVect) const
    {
        return !(operator==(scaleVect));
    }
};

template <class U, U units>
struct ScaleOffsetVect : std::vector<ScaleOffsetVal<U, units>>
{
    using std::vector<ScaleOffsetVal<U, units>>::size;
    using std::vector<ScaleOffsetVal<U, units>>::clear;
    using std::vector<ScaleOffsetVal<U, units>>::push_back;
    using std::vector<ScaleOffsetVal<U, units>>::resize;

    ScaleOffsetVect(const std::vector<double>& xV = {}, const U fromUnits = units)
        : std::vector<ScaleOffsetVal<U, units>>({})
    {
        for (auto x : xV)
            push_back(scaleOffset(fromUnits, units) * x);
    }

    template <U fromUnits>
    ScaleOffsetVect(const std::vector<ScaleOffsetVal<U, fromUnits>>& sV)
        : ScaleOffsetVect(sV, fromUnits)
    {
        for (auto s : sV)
            push_back(scaleOffset(fromUnits, units) * s);
    }

    ScaleOffsetVect(const std::size_t n) : ScaleOffsetVect() { resize(n); }

    operator std::vector<double>() const
    {
        std::vector<double> xV = {};
        xV.reserve(size());
        for (auto& s : *this)
            xV.push_back(s);
        return xV;
    }

    template <typename... val>
    ScaleOffsetVect(const std::tuple<val...>& sV) : ScaleOffsetVect()
    {
        for (auto s : sV)
            push_back(s);
    }

    std::vector<double> operator()(const U toUnits) const
    {
        std::vector<double> xV = {};
        xV.reserve(size());
        for (auto& x : *this)
            xV.push_back(scaleOffset(units, toUnits) * x);
        return xV;
    }

    template <U toUnits>
    bool operator==(const ScaleOffsetVect<U, toUnits> scaleOffsetVect) const
    {
        return (scaleOffsetVect(units) == fV(units));
    }

    template <U toUnits>
    bool operator!=(const ScaleOffsetVect<U, toUnits> scaleOffsetVect) const
    {
        return !(operator==(scaleOffsetVect));
    }
};

} // namespace Unity

#endif
