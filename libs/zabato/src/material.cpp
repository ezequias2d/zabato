#include "tinyxml2.h"
#include "zabato/gpu.hpp"
#include "zabato/resource.hpp"
#include "zabato/xml_serializer.hpp"
#include <zabato/error.hpp>
#include <zabato/material.hpp>
#include <zabato/reflection.hpp>

namespace zabato
{

const rtti material::TYPE("zabato.material", &resource::TYPE, nullptr);

material::material() = default;
material::~material() { release_dependencies(); }

material_parameter &material::ensure_param(const string_view &name,
                                           param_type type)
{
    for (auto &p : m_parameters)
    {
        if (p.name == name)
        {
            if (p.type != type)
                p.type = type;
            return p;
        }
    }
    material_parameter p;
    p.name = name;
    p.type = type;
    m_parameters.push_back(p);
    return m_parameters.back();
}

void material::set_float(const string_view &name, real val)
{
    ensure_param(name, param_type::float_val).r = val;
}

void material::set_int(const string_view &name, int val)
{
    ensure_param(name, param_type::int_val).i = val;
}

void material::set_bool(const string_view &name, bool val)
{
    ensure_param(name, param_type::bool_val).i = val ? 1 : 0;
}

void material::set_vec2(const string_view &name, const vec2<real> &val)
{
    ensure_param(name, param_type::vec2_val).v2 = val;
}

void material::set_vec3(const string_view &name, const vec3<real> &val)
{
    ensure_param(name, param_type::vec3_val).v3 = val;
}

void material::set_vec4(const string_view &name, const vec4<real> &val)
{
    ensure_param(name, param_type::vec4_val).v4 = val;
}

void material::set_color(const string_view &name, const color &val)
{
    auto &p = ensure_param(name, param_type::color_val);
    p.v4    = {(real)val.r, (real)val.g, (real)val.b, (real)val.a};
}

void material::set_texture(const string_view &name, const resource_ref &tex)
{
    bool found = false;
    for (auto &t : m_textures)
    {
        if (t.uniform_name == name)
        {
            t.uniform_name = name;
            t.tex          = tex;
            found          = true;
            break;
        }
    }
    if (!found)
    {
        texture_slot t;
        t.uniform_name = name;
        t.tex          = tex;
        m_textures.push_back(t);
    }
}

void material::apply(gpu &g)
{
    auto shader = get_shader();
    if (!shader)
        return;

    // Check if shader is compiled?
    if (!shader->is_compiled())
    {
        auto script_system = get_script_system();
        if (!script_system)
        {
            report_error(error_code::no_script_system);
            return;
        }
        shader->compile(*script_system, &g);
    }

    program *prog = shader->get_program();
    if (!prog)
        return;

    g.use_program(prog);
    g.set_render_state(m_state);

    // Apply Properties
    for (const auto &p : m_parameters)
    {
        switch (p.type)
        {
        case param_type::float_val:
            g.set_uniform(prog, p.name, p.r);
            break;
        case param_type::bool_val:
            g.set_uniform(prog, p.name, p.i);
            break;
        case param_type::int_val:
            g.set_uniform(prog, p.name, p.i);
            break;
        case param_type::vec2_val:
            g.set_uniform(prog, p.name, p.v2);
            break;
        case param_type::vec3_val:
            g.set_uniform(prog, p.name, p.v3);
            break;
        case param_type::vec4_val:
        case param_type::color_val:
            g.set_uniform(prog, p.name, p.v4);
            break;
        }
    }

    // Bind Textures
    int slot = 0;
    for (const auto &t : m_textures)
    {
        g.set_active_texture(slot);

        shared_ptr<texture> tex = t.tex.get<texture>();
        m_dependencies.push_back(tex);

        g.bind_texture(tex.get());
        g.set_uniform(prog, t.uniform_name, slot);
        slot++;
    }
    g.set_active_texture(0); // Reset to default
}

static const char *to_string(depth_func func)
{
    switch (func)
    {
    case depth_func::never:
        return "never";
    case depth_func::less:
        return "less";
    case depth_func::equal:
        return "equal";
    case depth_func::less_equal:
        return "less_equal";
    case depth_func::greater:
        return "greater";
    case depth_func::not_equal:
        return "not_equal";
    case depth_func::greater_equal:
        return "greater_equal";
    case depth_func::always:
        return "always";
    default:
        return "unknown";
    }
}

static const char *to_string(blend_factor factor)
{
    switch (factor)
    {
    case blend_factor::zero:
        return "zero";
    case blend_factor::one:
        return "one";
    case blend_factor::src_color:
        return "src_color";
    case blend_factor::one_minus_src_color:
        return "one_minus_src_color";
    case blend_factor::src_alpha:
        return "src_alpha";
    case blend_factor::one_minus_src_alpha:
        return "one_minus_src_alpha";
    case blend_factor::dst_alpha:
        return "dst_alpha";
    case blend_factor::one_minus_dst_alpha:
        return "one_minus_dst_alpha";
    case blend_factor::dst_color:
        return "dst_color";
    case blend_factor::one_minus_dst_color:
        return "one_minus_dst_color";
    case blend_factor::src_alpha_saturate:
        return "src_alpha_saturate";
    default:
        return "unknown";
    }
}

static const char *to_string(alpha_func func)
{
    switch (func)
    {
    case alpha_func::never:
        return "never";
    case alpha_func::less:
        return "less";
    case alpha_func::equal:
        return "equal";
    case alpha_func::less_equal:
        return "less_equal";
    case alpha_func::greater:
        return "greater";
    case alpha_func::not_equal:
        return "not_equal";
    case alpha_func::greater_equal:
        return "greater_equal";
    case alpha_func::always:
        return "always";
    default:
        return "unknown";
    }
}

static const char *to_string(cull_face_mode mode)
{
    switch (mode)
    {
    case cull_face_mode::front:
        return "front";
    case cull_face_mode::back:
        return "back";
    case cull_face_mode::front_and_back:
        return "front_and_back";
    default:
        return "unknown";
    }
}

static const char *to_string(polygon_mode mode)
{
    switch (mode)
    {
    case polygon_mode::point:
        return "point";
    case polygon_mode::line:
        return "line";
    case polygon_mode::fill:
        return "fill";
    default:
        return "unknown";
    }
}

static void save(xml_serializer &serializer,
                 tinyxml2::XMLElement &element,
                 const render_state &state)
{
    auto depth = element.InsertNewChildElement("depth");
    depth->SetAttribute("test", state.depth_test);
    depth->SetAttribute("write", state.depth_write);
    depth->SetAttribute("compare", to_string(state.depth_compare));

    auto blend = element.InsertNewChildElement("blend");
    blend->SetAttribute("enable", state.blend);
    blend->SetAttribute("src", to_string(state.blend_src));
    blend->SetAttribute("dst", to_string(state.blend_dst));

    auto alpha = element.InsertNewChildElement("alpha");
    alpha->SetAttribute("test", state.alpha_test);
    alpha->SetAttribute("compare", to_string(state.alpha_compare));
    alpha->SetAttribute("ref", (double)state.alpha_ref);

    auto cull = element.InsertNewChildElement("cull");
    cull->SetAttribute("enable", state.cull_face);
    cull->SetAttribute("mode", to_string(state.cull_mode));

    auto poly = element.InsertNewChildElement("polygon");
    poly->SetAttribute("mode", to_string(state.poly_mode));
    poly->SetAttribute("offset", state.poly_offset);

    auto offset = poly->InsertNewChildElement("offset");
    offset->SetAttribute("factor", (double)state.poly_offset_factor);
    offset->SetAttribute("units", (double)state.poly_offset_units);
}

static const char *to_string(param_type type)
{
    switch (type)
    {
    case param_type::bool_val:
        return "bool";
    case param_type::int_val:
        return "int";
    case param_type::vec2_val:
        return "vec2";
    case param_type::vec3_val:
        return "vec3";
    case param_type::vec4_val:
        return "vec4";
    case param_type::color_val:
        return "color";
    default:
        return "unknown";
    }
}

void material::save_xml(xml_serializer &serializer,
                        tinyxml2::XMLElement &element)
{
    element.SetAttribute("shader", m_shader.c_path());

    auto state = element.InsertNewChildElement("state");
    save(serializer, *state, m_state);

    auto params = element.InsertNewChildElement("parameters");
    for (const auto &p : m_parameters)
    {
        auto param = params->InsertNewChildElement("param");
        param->SetAttribute("name", p.name.c_str());
        param->SetAttribute("type", to_string(p.type));

        switch (p.type)
        {
        case param_type::bool_val:
            param->SetAttribute("value", p.i ? true : false);
            break;
        case param_type::int_val:
            param->SetAttribute("value", p.i);
            break;
        case param_type::vec2_val:
            xml_serializer::write_vec2(*param, p.v2);
            break;
        case param_type::vec3_val:
            xml_serializer::write_vec3(*param, p.v3);
            break;
        case param_type::vec4_val:
            xml_serializer::write_vec4(*param, p.v4);
            break;
        case param_type::color_val:
            xml_serializer::write_vec4(*param, p.v4);
            break;
        default:
            break;
        }
    }

    auto textures = element.InsertNewChildElement("textures");
    for (const auto &t : m_textures)
    {
        auto texture = textures->InsertNewChildElement("texture");
        texture->SetAttribute("name", t.uniform_name.c_str());
        texture->SetAttribute("path", t.tex.c_path());
    }
}

static depth_func to_depth_func(const char *str)
{
    if (strcmp(str, "never") == 0)
        return depth_func::never;
    if (strcmp(str, "less") == 0)
        return depth_func::less;
    if (strcmp(str, "equal") == 0)
        return depth_func::equal;
    if (strcmp(str, "less_equal") == 0)
        return depth_func::less_equal;
    if (strcmp(str, "greater") == 0)
        return depth_func::greater;
    if (strcmp(str, "not_equal") == 0)
        return depth_func::not_equal;
    if (strcmp(str, "greater_equal") == 0)
        return depth_func::greater_equal;
    if (strcmp(str, "always") == 0)
        return depth_func::always;
    return depth_func::never;
}

static alpha_func to_alpha_func(const char *str)
{
    if (strcmp(str, "never") == 0)
        return alpha_func::never;
    if (strcmp(str, "less") == 0)
        return alpha_func::less;
    if (strcmp(str, "equal") == 0)
        return alpha_func::equal;
    if (strcmp(str, "less_equal") == 0)
        return alpha_func::less_equal;
    if (strcmp(str, "greater") == 0)
        return alpha_func::greater;
    if (strcmp(str, "not_equal") == 0)
        return alpha_func::not_equal;
    if (strcmp(str, "greater_equal") == 0)
        return alpha_func::greater_equal;
    if (strcmp(str, "always") == 0)
        return alpha_func::always;
    return alpha_func::never;
}

static blend_factor to_blend_factor(const char *str)
{
    if (strcmp(str, "zero") == 0)
        return blend_factor::zero;
    if (strcmp(str, "one") == 0)
        return blend_factor::one;
    if (strcmp(str, "src_color") == 0)
        return blend_factor::src_color;
    if (strcmp(str, "one_minus_src_color") == 0)
        return blend_factor::one_minus_src_color;
    if (strcmp(str, "dst_color") == 0)
        return blend_factor::dst_color;
    if (strcmp(str, "one_minus_dst_color") == 0)
        return blend_factor::one_minus_dst_color;
    if (strcmp(str, "src_alpha") == 0)
        return blend_factor::src_alpha;
    if (strcmp(str, "one_minus_src_alpha") == 0)
        return blend_factor::one_minus_src_alpha;
    if (strcmp(str, "dst_alpha") == 0)
        return blend_factor::dst_alpha;
    if (strcmp(str, "one_minus_dst_alpha") == 0)
        return blend_factor::one_minus_dst_alpha;
    return blend_factor::zero;
}

static cull_face_mode to_cull_face_mode(const char *str)
{
    if (strcmp(str, "front") == 0)
        return cull_face_mode::front;
    if (strcmp(str, "back") == 0)
        return cull_face_mode::back;
    if (strcmp(str, "front_and_back") == 0)
        return cull_face_mode::front_and_back;
    return cull_face_mode::back;
}

static polygon_mode to_polygon_mode(const char *str)
{
    if (strcmp(str, "point") == 0)
        return polygon_mode::point;
    if (strcmp(str, "line") == 0)
        return polygon_mode::line;
    if (strcmp(str, "fill") == 0)
        return polygon_mode::fill;
    return polygon_mode::fill;
}

static void load(xml_serializer &serializer,
                 tinyxml2::XMLElement &element,
                 render_state &state)
{
    auto depth = element.FirstChildElement("depth");
    if (depth)
    {
        depth->QueryBoolAttribute("test", &state.depth_test);
        depth->QueryBoolAttribute("write", &state.depth_write);
        state.depth_compare = to_depth_func(depth->Attribute("compare"));
    }

    auto blend = element.FirstChildElement("blend");
    if (blend)
    {
        blend->QueryBoolAttribute("enable", &state.blend);
        state.blend_src = to_blend_factor(blend->Attribute("src"));
        state.blend_dst = to_blend_factor(blend->Attribute("dst"));
    }

    auto alpha = element.FirstChildElement("alpha");
    if (alpha)
    {
        alpha->QueryBoolAttribute("test", &state.alpha_test);
        state.alpha_compare = to_alpha_func(alpha->Attribute("compare"));

        double ref = 0;
        alpha->QueryDoubleAttribute("ref", &ref);
        state.alpha_ref = (real)ref;
    }

    auto cull = element.FirstChildElement("cull");
    if (cull)
    {
        cull->QueryBoolAttribute("enable", &state.cull_face);
        state.cull_mode = to_cull_face_mode(cull->Attribute("mode"));
    }

    auto poly = element.FirstChildElement("polygon");
    if (poly)
    {
        state.poly_mode = to_polygon_mode(poly->Attribute("mode"));
        poly->QueryBoolAttribute("offset", &state.poly_offset);

        auto offset = poly->FirstChildElement("offset");
        if (offset)
        {
            double offset_factor = 0;
            double offset_units  = 0;

            offset->QueryDoubleAttribute("factor", &offset_factor);
            offset->QueryDoubleAttribute("units", &offset_units);

            state.poly_offset_factor = (real)offset_factor;
            state.poly_offset_units  = (real)offset_units;
        }
    }
}

static param_type to_param_type(const char *t)
{
    if (strcmp(t, "float") == 0)
        return param_type::float_val;
    if (strcmp(t, "int") == 0)
        return param_type::int_val;
    if (strcmp(t, "bool") == 0)
        return param_type::bool_val;
    if (strcmp(t, "vec2") == 0)
        return param_type::vec2_val;
    if (strcmp(t, "vec3") == 0)
        return param_type::vec3_val;
    if (strcmp(t, "vec4") == 0)
        return param_type::vec4_val;
    if (strcmp(t, "color") == 0)
        return param_type::color_val;
    return param_type::float_val;
}

void material::load_xml(xml_serializer &serializer,
                        tinyxml2::XMLElement &element)
{
    auto shader = element.Attribute("shader");
    if (shader)
        m_shader.set_path(shader);

    load(serializer, element, m_state);

    auto params = element.FirstChildElement("parameters");
    if (params)
    {
        for (auto param = params->FirstChildElement("param"); param;
             param      = param->NextSiblingElement("param"))
        {
            auto name = param->Attribute("name");
            auto type = param->Attribute("type");
            if (name && type)
            {
                material_parameter p;
                p.name = name;
                p.type = to_param_type(type);
                switch (p.type)
                {
                case param_type::float_val:
                {
                    double v = 0;
                    param->QueryDoubleAttribute("value", &v);
                    p.r = v;
                    break;
                }
                case param_type::int_val:
                    param->QueryIntAttribute("value", &p.i);
                    break;
                case param_type::bool_val:
                {
                    bool v = false;
                    param->QueryBoolAttribute("value", &v);
                    p.i = v ? 1 : 0;
                    break;
                }
                case param_type::vec2_val:
                    p.v2 = xml_serializer::read_vec2(*param);
                    break;
                case param_type::vec3_val:
                    p.v3 = xml_serializer::read_vec3(*param);
                    break;
                case param_type::vec4_val:
                    p.v4 = xml_serializer::read_vec4(*param);
                    break;
                case param_type::color_val:
                    p.v4 = xml_serializer::read_vec4(*param);
                    break;
                default:
                    break;
                }
                m_parameters.push_back(p);
            }
        }
    }

    auto textures = element.FirstChildElement("textures");
    if (textures)
    {
        for (auto texture = textures->FirstChildElement("texture"); texture;
             texture      = texture->NextSiblingElement("texture"))
        {
            auto name = texture->Attribute("name");
            auto path = texture->Attribute("path");
            if (name && path)
            {
                texture_slot t;
                t.uniform_name = name;
                t.tex.set_path(path);
                m_textures.push_back(t);
            }
        }
    }
}

} // namespace zabato
