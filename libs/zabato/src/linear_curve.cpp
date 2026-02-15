#include "zabato/base_object.hpp"
#include "zabato/ice.hpp"
#include "zabato/object.hpp"
#include "zabato/reflection.hpp"
#include "zabato/script.hpp"
#include "zabato/xml_serializer.hpp"
#include <zabato/linear_curve.hpp>

namespace zabato
{

const rtti linear_curve::TYPE =
    rtti("zabato.linear_curve", &base_object::TYPE, linear_curve::reflect);

static void w_get_points(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 1)
        return;
    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;

    auto points = value::make_list();
    for (auto point : curve->points)
        points.push(point);
    args->push_return(points);
}

static void w_set_points(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 2)
        return;

    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;

    auto points = args->get_value(1);
    curve->clear();
    if (points.is_list())
    {
        size_t count = points.length();
        curve->points.reserve(count);
        for (size_t i = 0; i < count; i++)
        {
            auto point = points.get_at(i);
            curve->points.push_back(point.as_vec2());
        }
    }
}

static void w_clear(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 1)
        return;

    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;
    curve->clear();
}

static void w_reserve(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 2)
        return;

    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;
    curve->reserve(args->get_value(1).as_int());
}

static void w_sort(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 1)
        return;

    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;
    curve->sort();
}

static void w_get_point(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 2)
        return;

    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;
    args->push_return(curve->points[args->get_value(1).as_int()]);
}

static void w_set_point(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 3)
        return;

    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;
    curve->points[args->get_value(1).as_int()] = args->get_value(2).as_vec2();
}

static void
w_get_point_count(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 1)
        return;

    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;
    args->push_return((int64_t)curve->points.size());
}

static void w_get_min_x(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 1)
        return;

    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;
    args->push_return(curve->get_min_x());
}

static void w_get_max_x(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 1)
        return;

    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;
    args->push_return(curve->get_max_x());
}

static void w_get_value(script_system *, script_instance *, script_args *args)
{
    if (args->count() != 2)
        return;

    auto curve =
        c_dynamic_cast<linear_curve>(args->get_value(0).as_object().get());
    if (!curve)
        return;

    args->push_return(curve->get_value(args->get_value(1).as_number()));
}

void linear_curve::reflect(reflection &r)
{
    base_object::reflect(r);

    r.add_property("points", w_get_points, w_set_points);

    r.add_method("clear", w_clear);
    r.add_method("reserve", w_reserve);
    r.add_method("sort", w_sort);
    r.add_method("get_point", w_get_point);
    r.add_method("set_point", w_set_point);
    r.add_method("get_point_count", w_get_point_count);
    r.add_method("get_min_x", w_get_min_x);
    r.add_method("get_max_x", w_get_max_x);
    r.add_method("get_value", w_get_value);
}

void linear_curve::save(serializer &stream) const
{
    ice_uint16_t count = (uint16_t)points.size();
    stream.write(count);
    for (auto &point : points)
    {
        ICE_VEC2<ice_real, ice_real> p = point;
        stream.write(p);
    }
}

void linear_curve::load(serializer &stream, serializer_link *link)
{
    ice_uint16_t count = 0;
    stream.read(count);
    points.resize(count);
    for (size_t i = 0; i < count; i++)
    {
        ICE_VEC2<ice_real, ice_real> p;
        stream.read(p);
        points[i] = p;
    }
}

void linear_curve::save_xml(xml_serializer &stream,
                            tinyxml2::XMLElement &element) const
{
    for (size_t i = 0; points.size(); i++)
    {
        auto p = element.InsertNewChildElement("point");
        xml_serializer::write_vec2(*p, points[i]);
    }
}

void linear_curve::load_xml(xml_serializer &stream,
                            tinyxml2::XMLElement &element)
{
    points.clear();
    for (auto p = element.FirstChildElement("point"); p;
         p      = p->NextSiblingElement("point"))
    {
        points.push_back(xml_serializer::read_vec2(*p));
    }
}

} // namespace zabato