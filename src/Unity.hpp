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

template <typename D, typename T, typename Ti = T>
struct Transform
{
    D data;

    explicit Transform(D data_in) : data(std::move(data_in)) {}

    D operator()() const { return data; }
    D& operator()() { return data; }

    static auto ident() { T::ident(); }

    [[nodiscard]] virtual Ti inverse() const = 0;

    virtual double operator*(double x) const = 0;

    friend double operator/(const double x, const Transform& t) { return t.inverse() * x; }
};

struct Offset;
struct ScaleOffset;

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

struct Offset : Transform<double, Offset>
{
    using Transform<double, Offset>::data;

    explicit Offset(const double offset) : Transform(offset) {}

    static Offset ident() { return Offset(0.); }

    [[nodiscard]] Offset inverse() const override { return Offset(-data); }

    double operator*(const double x) const override { return x + data; }

    Offset operator*(const Offset& o) const { return Offset(data + o.data); }

    Offset operator/(const Offset& o) const { return Offset(data - o.data); }

    [[nodiscard]] double offset() const { return data; }
};

using ScaleOffsetSeq = std::pair<Scale, Offset>;

struct ScaleOffset : Transform<ScaleOffsetSeq, ScaleOffset>
{
    explicit ScaleOffset(const ScaleOffsetSeq& scaleOffsetSeq) : Transform(scaleOffsetSeq) {}

    ScaleOffset(const Scale& scale, const Offset& offset) : Transform({scale, offset}) {}

    static ScaleOffset ident() { return {Scale(1.), Offset(0.)}; }

    [[nodiscard]] ScaleOffset inverse() const override
    {
        return {Scale(data.first.inverse()),
                Offset(data.first.inverse()() * data.second.inverse()())};
    }

    double operator*(const double x) const override { return data.second * (data.first * x); }

    ScaleOffset operator*(const ScaleOffset& a) const
    {
        return {data.first * a.data.first,
                Offset(data.first.scale() * a.data.second.offset() + data.second.offset())};
    }

    Scale scale() const { return data.first; }

    Offset offset() const { return data.second; }
};

/// Transformer classes
template <typename U, typename T>
struct Transformer
{
    struct TransformMap : std::map<std::pair<U, U>, T>
    {
        using std::map<std::pair<U, U>, T>::insert;
        using std::map<std::pair<U, U>, T>::at;

        TransformMap(const U uRef, const std::vector<std::pair<U, T>>& transforms)
        {
            insert({{uRef, uRef}, T::ident()});
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

        inline T transform(const U fromUnits, const U toUnits)
        {
            return (fromUnits != toUnits) ? at({fromUnits, toUnits}) : T::ident();
        }
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

    inline static Scale transform(const U fromUnits, const U toUnits)
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
    explicit TransformVal(const double x_in = 0.) : x(x_in) {}

    template <U fromUnits>
    explicit TransformVal(const TransformVal<U, T, fromUnits> transformVal) : x(transformVal(units))
    {
    }

    ~TransformVal() = default;

    explicit operator const double&() const { return x; }
    explicit operator double&() { return x; }

    //double operator+(const double y) const { return x + y; }
    //double operator-(const double y) const { return x - y; }
    //double operator/(const double y) const { return x / y; }

    bool operator==(const double y) const { return (x == y); }
    bool operator!=(const double y) const { return !(operator==(y)); }

    bool operator<(const double y) const { return x < y; }
    bool operator>(const double y) const { return x > y; }
    bool operator<=(const double y) const { return x <= y; }
    bool operator>=(const double y) const { return x >= y; }

    TransformVal& operator=(double x_in)
    {
        x = x_in;
        return *this;
    }

    U in() { return units; }
};

template <class U, U units>
struct ScaleVal : TransformVal<U, Scale, units>
{
    using TransformVal<U, Scale, units>::x;
    using TransformVal<U, Scale, units>::operator<;
    using TransformVal<U, Scale, units>::operator>;
    using TransformVal<U, Scale, units>::operator<=;
    using TransformVal<U, Scale, units>::operator>=;
    using TransformVal<U, Scale, units>::operator==;
    using TransformVal<U, Scale, units>::operator!=;
    //using TransformVal<U, Scale, units>::operator+;
    //using TransformVal<U, Scale, units>::operator-;
    //using TransformVal<U, Scale, units>::operator/;

    explicit ScaleVal(const double x_in = 0.) : TransformVal<U, Scale, units>(x_in) {}

    ScaleVal(const double x_in, const U fromUnits)
        : TransformVal<U, Scale, units>(scale(fromUnits, units) * x_in)
    {
    }

    template <U fromUnits>
    explicit ScaleVal(const ScaleVal<U, fromUnits>& scaleVal)
        : TransformVal<U, Scale, units>(scale(fromUnits, units) * scaleVal())
    {
    }

    template <U toUnits>
    auto operator()()
    {
        return ScaleVal<U, toUnits>(x, units);
    }

    double operator()() const { return x; }
    double& operator()() { return x; }

    double operator()(const U toUnits) const { return scale(units, toUnits) * x; }

    ScaleVal& operator+=(const double y)
    {
        x += y;
        return *this;
    }
    ScaleVal& operator-=(const double y)
    {
        x -= y;
        return *this;
    }

    template <U fromUnits>
    ScaleVal& operator=(const ScaleVal<U, fromUnits>& scaleVal)
    {
        x = scaleVal(units);
        return *this;
    }

    auto operator+(const double y) const { return ScaleVal(x + y); }
    auto operator-(const double y) const { return ScaleVal(x - y); }
    auto operator/(const double y) const { return ScaleVal(x / y); }
    friend auto operator*(const double y, const ScaleVal& scaleVal) { return ScaleVal(y * scaleVal()); }

    double operator/(const ScaleVal& scaleVal) const { return x / scaleVal(); }

    ScaleVal operator+(const ScaleVal& scaleVal) const { return ScaleVal(x + scaleVal()); }
    ScaleVal operator-(const ScaleVal& scaleVal) const { return ScaleVal(x - scaleVal()); }

    ScaleVal operator+=(const ScaleVal& scaleVal)
    {
        x += scaleVal();
        return *this;
    }
    ScaleVal operator-=(const ScaleVal& scaleVal)
    {
        x -= scaleVal();
        return *this;
    }

    template <U toUnits>
    bool operator==(const ScaleVal<U, toUnits> scaleVal) const
    {
        return (scaleVal(units) == x);
    }

    template <U toUnits>
    bool operator!=(const ScaleVal<U, toUnits>& scaleVal) const
    {
        return !(operator==(scaleVal));
    }

    template <U toUnits>
    bool operator<(const ScaleVal<U, toUnits> scaleVal) const
    {
        return (x < scaleVal(units));
    }

    template <U toUnits>
    bool operator>(const ScaleVal<U, toUnits> scaleVal) const
    {
        return (x > scaleVal(units));
    }

    template <U toUnits>
    bool operator<=(const ScaleVal<U, toUnits> scaleVal) const
    {
        return (x <= scaleVal(units));
    }

    template <U toUnits>
    bool operator>=(const ScaleVal<U, toUnits> scaleVal) const
    {
        return (x >= scaleVal(units));
    }

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
    ScaleVal& operator+=(const ScaleVal<U, fromUnits>& scaleVal)
    {
        return *this = *this + scaleVal;
    }

    template <U fromUnits>
    ScaleVal& operator-=(const ScaleVal<U, fromUnits>& scaleVal)
    {
        return *this = *this - scaleVal;
    }
};

template <class U, U units>
struct ScaleOffsetVal : TransformVal<U, ScaleOffset, units>
{
    using TransformVal<U, ScaleOffset, units>::x;
    //using TransformVal<U, ScaleOffset, units>::operator+;
    //using TransformVal<U, ScaleOffset, units>::operator-;
    using TransformVal<U, ScaleOffset, units>::operator<;
    using TransformVal<U, ScaleOffset, units>::operator>;
    using TransformVal<U, ScaleOffset, units>::operator<=;
    using TransformVal<U, ScaleOffset, units>::operator>=;
    using TransformVal<U, ScaleOffset, units>::operator==;
    using TransformVal<U, ScaleOffset, units>::operator!=;

    explicit ScaleOffsetVal(const double x_in = 0.) : TransformVal<U, ScaleOffset, units>(x_in) {}

    ScaleOffsetVal(const double x_in, const U fromUnits)
        : TransformVal<U, ScaleOffset, units>(scaleOffset(fromUnits, units, x_in))
    {
    }

    template <U fromUnits>
    explicit ScaleOffsetVal(const ScaleOffsetVal<U, fromUnits>& scaleOffsetVal)
        : TransformVal<U, ScaleOffset, units>(scaleOffset(fromUnits, units) * scaleOffsetVal())
    {
    }

    ScaleOffsetVal& operator+=(const double y)
    {
        x += y;
        return *this;
    }
    ScaleOffsetVal& operator-=(const double y)
    {
        x -= y;
        return *this;
    }

    template <U fromUnits>
    ScaleOffsetVal& operator=(const ScaleOffsetVal<U, fromUnits>& scaleOffsetVal)
    {
        x = scaleOffsetVal(units);
        return *this;
    }

    template <U fromUnits>
    ScaleOffsetVal& operator+(const ScaleVal<U, fromUnits>& scaleVal) const
    {
        return x + scaleVal(units);
    }

    template <U fromUnits>
    ScaleOffsetVal& operator-(const ScaleVal<U, fromUnits>& scaleVal) const
    {
        return x - scaleVal(units);
    }


    template <U fromUnits>
    ScaleOffsetVal& operator+=(const ScaleVal<U, fromUnits>& scaleVal)
    {
        x += scaleVal();
        return *this;
    }

    template <U fromUnits>
    ScaleOffsetVal& operator-=(const ScaleVal<U, fromUnits>& scaleVal)
    {
        x -= scaleVal();
        return *this;
    }

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

    template <U toUnits>
    bool operator<(const ScaleOffsetVal<U, toUnits> scaleOffsetVal) const
    {
        return (x < scaleOffsetVal(units));
    }

    template <U toUnits>
    bool operator>(const ScaleOffsetVal<U, toUnits> scaleOffsetVal) const
    {
        return (x > scaleOffsetVal(units));
    }

    template <U toUnits>
    bool operator<=(const ScaleOffsetVal<U, toUnits> scaleOffsetVal) const
    {
        return (x <= scaleOffsetVal(units));
    }

    template <U toUnits>
    bool operator>=(const ScaleOffsetVal<U, toUnits> scaleOffsetVal) const
    {
        return (x >= scaleOffsetVal(units));
    }

    double operator()() const { return x; }
    double& operator()() { return x; }

    double operator()(const U toUnits) const { return scaleOffset(units, toUnits) * x; }
};

/// transform vectors
template <class U, typename T, U units>
struct TransformVect
{
  protected:
    std::vector<double> xV;

  public:
    TransformVect(const std::vector<double>& xV_in = {}) : xV(xV_in) {}

    explicit TransformVect(const std::size_t n) : xV(n) {}

    explicit operator std::vector<double>&() { return xV; }
    explicit operator const std::vector<double>&() const { return xV; }

    std::vector<double> operator()() const { return xV; }
    std::vector<double>& operator()() { return xV; }

    inline auto size() const { return xV.size(); }
    inline auto resize(const std::size_t n) { return xV.resize(n); }
    inline auto clear() { return xV.clear(); }
    inline auto empty() const { return xV.empty(); }

    U in() { return units; }
};

template <class U, U units>
struct ScaleVect : public TransformVect<U, Scale, units>
{
  public:
    using TransformVect<U, Scale, units>::xV;
    using TransformVect<U, Scale, units>::size;
    using TransformVect<U, Scale, units>::resize;
    using TransformVect<U, Scale, units>::clear;
    using TransformVect<U, Scale, units>::operator();

    ScaleVect(const std::vector<double>& xV_in = {}) : TransformVect<U, Scale, units>(xV_in) {}

    ScaleVect(const std::size_t n) : TransformVect<U, ScaleOffset, units>(n) {}

    ScaleVect(const std::vector<double>& xV_in, const U fromUnits) : ScaleVect<U, units>()
    {
        auto t = scale(fromUnits, units);
        for (auto x : xV_in)
            xV.push_back(t * x);
    }

    template <U fromUnits>
    ScaleVect(const ScaleVect<U, fromUnits>& sV) : ScaleVect({}, fromUnits)
    {
        xV.reserve(sV.size());
        auto t = scale(fromUnits, units);
        for (auto s_ : sV)
            xV.push_back(t * s_);
    }

    template <U fromUnits>
    ScaleVect(const std::vector<ScaleVal<U, fromUnits>>& sV) : ScaleVect({}, fromUnits)
    {
        xV.reserve(sV.size());
        auto t = scale(fromUnits, units);
        for (auto s_ : sV)
            xV.push_back(t * s_);
    }

    template <typename... val>
    ScaleVect(const std::tuple<val...>& sV) : ScaleVect()
    {
        xV.reserve(sV.size());
        for (auto s_ : sV)
            xV.push_back(s_);
    }

    std::vector<double> operator()(const U toUnits) const
    {
        std::vector<double> xV_out = {};
        xV_out.reserve(xV.size());
        auto t = scale(units, toUnits);
        for (auto& x : xV)
            xV_out.push_back(t * x);
        return xV_out;
    }

    ScaleVal<U, units> const& operator[](const std::size_t i) const { return xV[i]; }

    ScaleVal<U, units>& operator[](const std::size_t i)
    {
        return reinterpret_cast<ScaleVal<U, units>&>(xV[i]);
    }

    operator const std::vector<ScaleVal<U, units>>&() const
    {
        return static_cast<std::vector<ScaleVal<U, units>>>(xV);
    }

    operator std::vector<ScaleVal<U, units>>&()
    {
        return reinterpret_cast<std::vector<ScaleVal<U, units>>&>(xV);
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

    void rescale(const double factor)
    {
        if (factor != 1.)
        {
            std::transform(xV.begin(),
                           xV.end(),
                           xV.begin(),
                           [factor](auto&& PH1) {
                               return std::multiplies<>()(std::forward<decltype(PH1)>(PH1), factor);
                           });
        }
    }

    auto begin()
    {
        return xV.empty() ? nullptr : reinterpret_cast<ScaleVal<U, units>*>(&(*xV.begin()));
    }
    auto end() { return begin() + xV.size(); }

    auto begin() const
    {
        return xV.empty() ? nullptr : reinterpret_cast<ScaleVal<U, units>*>(&(*xV.begin()));
    }
    auto end() const { return begin() + xV.size(); }

    auto rbegin()
    {
        return xV.empty() ? nullptr : reinterpret_cast<ScaleVal<U, units>*>(&(*xV.rbegin()));
    }
    auto rend() { return rbegin() + xV.size(); }

    auto rbegin() const
    {
        return xV.empty() ? nullptr : reinterpret_cast<const ScaleVal<U, units>*>(&(*xV.rbegin()));
    }
    auto rend() const { return rbegin() + xV.size(); }

    auto& front() { return reinterpret_cast<ScaleVal<U, units>&>(xV.front()); }
    auto& back() { return reinterpret_cast<ScaleVal<U, units>&>(xV.back()); }

    auto front() const { return reinterpret_cast<const ScaleVal<U, units>&>(xV.front()); }
    auto back() const { return reinterpret_cast<const ScaleVal<U, units>&>(xV.back()); }

    auto push_back(const ScaleVal<U, units>& scaleVal) { xV.push_back(scaleVal()); }
};

template <class U, U units>
struct ScaleOffsetVect : TransformVect<U, ScaleOffset, units>
{
  public:
    using TransformVect<U, ScaleOffset, units>::xV;
    using TransformVect<U, ScaleOffset, units>::size;
    using TransformVect<U, ScaleOffset, units>::resize;
    using TransformVect<U, ScaleOffset, units>::clear;
    using TransformVect<U, ScaleOffset, units>::operator();

    ScaleOffsetVect(const std::vector<double>& xV_in = {})
        : TransformVect<U, ScaleOffset, units>(xV_in)
    {
    }

    ScaleOffsetVect(const std::vector<double>& xV_in, const U fromUnits) : ScaleOffsetVect()
    {
        xV.reserve(xV_in.size());
        const auto t = scaleOffset(fromUnits, units);
        for (auto x : xV_in)
            xV.push_back(t * x);
    }

    ScaleOffsetVect(const std::vector<ScaleOffsetVal<U, units>>& sV) : ScaleOffsetVect()
    {
        xV = sV();
    }

    ScaleOffsetVect(const std::size_t n) : TransformVect<U, ScaleOffset, units>(n) {}

    ScaleOffsetVal<U, units> operator[](const std::size_t i) const { return ScaleOffsetVal<U, units>(xV[i]); }

    ScaleOffsetVal<U, units>& operator[](const std::size_t i)
    {
        return reinterpret_cast<ScaleOffsetVal<U, units>&>(xV[i]);
    }

    operator const std::vector<ScaleOffsetVal<U, units>>&() const
    {
        return static_cast<std::vector<ScaleOffsetVal<U, units>>>(xV);
    }

    operator std::vector<ScaleOffsetVal<U, units>>&()
    {
        return reinterpret_cast<std::vector<ScaleOffsetVal<U, units>>&>(xV);
    }

    std::vector<double> operator()(const U toUnits) const
    {
        std::vector<double> xV_out = {};
        xV_out.reserve(xV.size());
        auto t = scaleOffset(units, toUnits);
        for (auto& x : xV)
            xV_out.push_back(t * x);
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

    auto begin()
    {
        return xV.empty() ? nullptr : reinterpret_cast<ScaleOffsetVal<U, units>*>(&(*xV.begin()));
    }
    auto end() { return begin() + xV.size(); }

    auto begin() const
    {
        return xV.empty() ? nullptr
                          : reinterpret_cast<const ScaleOffsetVal<U, units>*>(&(*xV.begin()));
    }
    auto end() const { return begin() + xV.size(); }

    auto rbegin()
    {
        return xV.empty() ? nullptr : reinterpret_cast<ScaleOffsetVal<U, units>*>(&(*xV.rbegin()));
    }
    auto rend() { return rbegin() + xV.size(); }

    auto rbegin() const
    {
        return xV.empty() ? nullptr
                          : reinterpret_cast<const ScaleOffsetVal<U, units>*>(&(*xV.rbegin()));
    }
    auto rend() const { return rbegin() + xV.size(); }

    auto& front() { return reinterpret_cast<ScaleOffsetVal<U, units>&>(xV.front()); }
    auto& back() { return reinterpret_cast<ScaleOffsetVal<U, units>&>(xV.back()); }

    auto front() const { return reinterpret_cast<const ScaleOffsetVal<U, units>&>(xV.front()); }
    auto back() const { return reinterpret_cast<const ScaleOffsetVal<U, units>&>(xV.back()); }

    auto push_back(const ScaleOffsetVal<U, units>& scaleOffsetVal)
    {
        xV.push_back(scaleOffsetVal());
    }
};

/// scale pairs
template <class U, U units0, U units1>
struct ScalePair : std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>
{
    ScalePair(const double x0_in = 0., const double x1_in = 0.)
        : std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>({ScaleVal<U, units0>(x0_in), ScaleVal<U, units1>(x1_in)})
    {
    }

    double operator()(const U toUnits) const { return first(toUnits) + second(toUnits); }

    using std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>::first;
    using std::pair<ScaleVal<U, units0>, ScaleVal<U, units1>>::second;
};

} // namespace Unity

#endif
