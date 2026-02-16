#pragma once

#include <zabato/base_object.hpp>
#include <zabato/math.hpp>
#include <zabato/real.hpp>
#include <zabato/rtti.hpp>
#include <zabato/serializer.hpp>
#include <zabato/utils.hpp>
#include <zabato/vector.hpp>

namespace zabato
{
struct linear_curve : public base_object
{
public:
    static const rtti TYPE;
    const rtti &type() const override final { return TYPE; }
    static void reflect(reflection &r);

    vector<vec2<real>> points;

    void clear() { points.clear(); }

    void reserve(size_t capacity) { points.reserve(capacity); }

    void add_point(real x, real y) { points.push_back(vec2<real>(x, y)); }

    void add_point(const vec2<real> &p) { points.push_back(p); }

    void sort()
    {
        zabato::sort(points.begin(),
                     points.end(),
                     [](const vec2<real> &inLHS, const vec2<real> &inRHS)
                     { return inLHS.x < inRHS.x; });
    }

    real get_min_x() const { return points.front().x; }

    real get_max_x() const { return points.back().x; }

    real get_value(real x) const
    {
        if (points.empty())
            return 0.0;

        if (x <= points.front().x)
            return points.front().y;

        if (x >= points.back().x)
            return points.back().y;

        auto it = lower_bound(points.begin(),
                              points.end(),
                              x,
                              [](const vec2<real> &val, real p)
                              { return val.x < p; });

        if (it == points.begin())
            return points.front().y;

        if (it == points.end())
            return points.back().y;

        auto it_before = it - 1;
        real x1        = it_before->x;
        real y1        = it_before->y;
        real x2        = it->x;
        real y2        = it->y;
        return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
    }

    void save(serializer &stream) const;
    void load(serializer &stream, serializer_link *link);
    void save_xml(xml_serializer &stream, tinyxml2::XMLElement &element) const;
    void load_xml(xml_serializer &stream, tinyxml2::XMLElement &element);
};
} // namespace zabato