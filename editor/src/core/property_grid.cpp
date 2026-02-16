#include "imgui_internal.h"
#include <editor/core/property_grid.hpp>

#include <editor/asset_database.hpp>
#include <imgui.h>
#include <zabato/base_object.hpp>
#include <zabato/imgui.hpp>
#include <zabato/math.hpp>
#include <zabato/object.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/string.hpp>
#include <zabato/value.hpp>
#include <zabato/vector.hpp>

#include <cstring>

namespace zabato::editor
{

class native_script_args : public script_args
{
public:
    vector<value> m_values;
    vector<value> m_returns;

    void push_arg(value v) { m_values.push_back(v); }

    virtual int count() const override { return (int)m_values.size(); }
    virtual value get_value(int index) override
    {
        if (index >= 0 && index < (int)m_values.size())
            return m_values[index];
        return value();
    }

    virtual void push_return(const value &v) override
    {
        m_returns.push_back(v);
    }

    virtual void error(const char *msg) override {}
    virtual void type_error(const char *msg) override {}
};

static void list_get(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;

    auto obj = args->get_value(0);
    if (!obj.is_list())
    {
        args->error("Object is not a list");
        return;
    }

    auto index = args->get_value(1).as_int();
    args->push_return(obj.get_at(index));
}

static void list_set(script_system *, script_instance *, script_args *args)
{
    if (args->count() < 2)
        return;

    auto obj = args->get_value(0);
    if (!obj.is_list())
    {
        args->error("Object is not a list");
        return;
    }
    auto index = args->get_value(1).as_int();
    auto val   = args->get_value(2);
    obj.set(index, val);
}

static void render_property(value obj,
                            const string &name,
                            property_def &prop,
                            asset_database *db,
                            delegate<void(const string &)> on_locate,
                            bool &changed,
                            value *arg = nullptr)
{
    if (prop.attributes.is_valid())
    {
        value visible_fn = prop.attributes.get_field("visible_if");
        if (!visible_fn.is_nil())
        {
            native_script_args args_check;
            args_check.push_arg(obj);
            visible_fn.call(nullptr, nullptr, &args_check);
            if (args_check.m_returns.size() > 0)
            {
                if (!args_check.m_returns[0].as_bool())
                    return;
            }
        }
    }

    value val; // nil
    value new_val;

    // Retrieve current value if getter exists
    if (prop.getter.is_valid())
    {
        native_script_args args_get;
        args_get.push_arg(obj);
        if (arg)
            args_get.push_arg(*arg);
        prop.getter.call(nullptr, nullptr, &args_get);
        if (args_get.m_returns.size() > 0)
            val = args_get.m_returns[0];
    }
    else
        return;

    ImGui::PushID(name.c_str());

    if (!prop.setter.is_valid())
        ImGui::PushItemFlag(ImGuiItemFlags_ReadOnly, true);

    // Attribute Checks
    bool handled = false;
    if (prop.attributes.is_valid())
    {
        // Check for Enum
        value enum_list = prop.attributes.get_field("enum");
        if (enum_list.is_list())
        {
            int current_idx = (int)val.as_int();
            vector<const char *> items;
            vector<string> hold_strings;
            int count = (int)enum_list.length();

            // Build combo items
            for (int i = 0; i < count; ++i)
            {
                value item = enum_list.get_at(i);
                if (item.is_string())
                {
                    hold_strings.push_back(string(item.as_string()));
                    items.push_back(hold_strings.back().c_str());
                }
                else
                {
                    items.push_back("Unknown");
                }
            }

            if (ImGui::Combo(name.c_str(),
                             &current_idx,
                             items.data(),
                             (int)items.size()))
            {
                new_val = value((double)current_idx);
                changed = true;
            }
            handled = true;
        }

        value asset_type_v = prop.attributes.get_field("asset_type");
        if (asset_type_v.is_string() && db && val.is_string())
        {
            string type_str     = string(asset_type_v.as_string());
            string current_path = string(val.as_string());
            asset_type type     = asset_type::unknown;

            if (type_str == "script")
                type = asset_type::script;
            else if (type_str == "mesh")
                type = asset_type::mesh;
            else if (type_str == "texture")
                type = asset_type::texture;
            else if (type_str == "audio")
                type = asset_type::audio;
            else if (type_str == "scene")
                type = asset_type::scene;
            else if (type_str == "shader")
                type = asset_type::shader;
            else if (type_str == "material")
                type = asset_type::material;

            if (type != asset_type::unknown)
            {

                if (draw_asset_selector(
                        name.c_str(), current_path, type, db, on_locate))
                {
                    new_val = value(current_path.c_str());
                    changed = true;
                }
                handled = true;
                ImGui::SameLine();
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::Text("%s", current_path.c_str());
            }
        }

        value types = prop.attributes.get_field("types");
        if (types.is_list() && val.is_object())
        {
            auto obj        = val.as_object();
            auto &type      = obj->type();
            int current_idx = 0;
            vector<const char *> items;
            vector<string> hold_strings;
            int count = (int)types.length();

            // Build combo items
            for (int i = 0; i < count; ++i)
            {
                value item = types.get_at(i);
                if (item.is_string())
                {
                    auto name = item.as_string();
                    hold_strings.push_back(name);
                    items.push_back(hold_strings.back().c_str());
                    if (type.name() == name)
                        current_idx = i;
                }
                else
                    items.push_back("Unknown");
            }

            if (ImGui::Combo(name.c_str(),
                             &current_idx,
                             items.data(),
                             (int)items.size()))
            {
                new_val = value(items[current_idx]);
                changed = true;
            }

            if (obj)
            {
                ImGui::PushID(name.c_str());
                if (ImGui::TreeNode(name.c_str()))
                {
                    auto obj         = val.as_object();
                    auto &type       = obj->type();
                    reflection &r    = type.ensure_reflection();
                    bool obj_changed = false;
                    for (const auto &name : r.property_order)
                    {
                        auto it           = r.properties.find(name);
                        bool prop_changed = false;
                        if (it != r.properties.end())
                            render_property(obj,
                                            name,
                                            it->value,
                                            db,
                                            on_locate,
                                            prop_changed);
                        obj_changed |= prop_changed;
                    }
                    if (obj_changed)
                    {
                        new_val = obj;
                        changed |= true;
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            handled = true;
        }
    }

    if (!handled)
    {
        // Type rendering
        if (val.is_bool())
        {
            bool b = val.as_bool();
            if (ImGui::Checkbox(name.c_str(), &b))
            {
                new_val = value(b);
                changed = true;
            }
        }
        else if (val.is_int())
        {
            int i = (int)val.as_int();
            if (ImGui::DragInt(name.c_str(), &i))
            {
                new_val = value((int64_t)i);
                changed = true;
            }
        }
        else if (val.is_number())
        {
            float f = (float)val.as_number();
            if (ImGui::DragFloat(name.c_str(), &f))
            {
                new_val = value((double)f);
                changed = true;
            }
        }
        else if (val.is_string())
        {
            string s(val.as_string());
            char buffer[256];
            size_t len = s.size();
            if (len > 255)
                len = 255;
            memcpy(buffer, s.data(), len);
            buffer[len] = 0;

            if (ImGui::InputText(name.c_str(), buffer, sizeof(buffer)))
            {
                new_val = value(buffer);
                changed = true;
            }
        }
        else if (val.is_vec2())
        {
            vec2<real> v = val.as_vec2();
            float vec[2] = {(float)v.x, (float)v.y};
            if (ImGui::DragFloat2(name.c_str(), vec, 0.1f))
            {
                new_val = value(vec2<real>(vec[0], vec[1]));
                changed = true;
            }
        }
        else if (val.is_vec3())
        {
            vec3<real> v = val.as_vec3();
            float vec[3] = {(float)v.x, (float)v.y, (float)v.z};
            if (ImGui::DragFloat3(name.c_str(), vec, 0.1f))
            {
                new_val = value(vec3<real>(vec[0], vec[1], vec[2]));
                changed = true;
            }
        }
        else if (val.is_vec4())
        {
            vec4<real> v = val.as_vec4();
            float vec[4] = {(float)v.x, (float)v.y, (float)v.z, (float)v.w};
            if (ImGui::DragFloat4(name.c_str(), vec, 0.1f))
            {
                new_val = value(vec4<real>(vec[0], vec[1], vec[2], vec[3]));
                changed = true;
            }
        }
        else if (val.is_color())
        {
            color c      = val.as_color();
            float vec[4] = {(float)c.r, (float)c.g, (float)c.b, (float)c.a};
            if (ImGui::ColorEdit4(name.c_str(), vec))
            {
                new_val = value(color(vec[0], vec[1], vec[2], vec[3]));
                changed = true;
            }
        }
        else if (val.is_quat())
        {
            quat<real> q = val.as_quat();
            float vec[4] = {(float)q.x, (float)q.y, (float)q.z, (float)q.w};
            if (ImGui::DragFloat4(name.c_str(), vec, 0.01f))
            {
                new_val = value(quat<real>(vec[3], vec[0], vec[1], vec[2]));
                changed = true;
            }
        }
        else if (val.is_object())
        {
            ImGui::PushID(name.c_str());
            if (ImGui::TreeNode(name.c_str()))
            {
                auto obj         = val.as_object();
                auto &type       = obj->type();
                reflection &r    = type.ensure_reflection();
                bool obj_changed = false;
                for (const auto &name : r.property_order)
                {
                    auto it           = r.properties.find(name);
                    bool prop_changed = false;
                    if (it != r.properties.end())
                        render_property(
                            obj, name, it->value, db, on_locate, prop_changed);
                    obj_changed |= prop_changed;
                }
                if (obj_changed)
                {
                    new_val = obj;
                    changed |= true;
                }
                ImGui::TreePop();
            }

            ImGui::PopID();
        }
        else if (val.is_list())
        {
            ImGui::PushID(name.c_str());

            char buf[64];
            snprintf(buf, sizeof(buf), "%s[%zu]", name.c_str(), val.length());
            if (ImGui::TreeNode(buf))
            {
                value item_attribute;
                if (prop.attributes.is_valid() && prop.attributes.is_list() &&
                    prop.attributes.get("item_attribute").is_valid())
                {
                    item_attribute = prop.attributes.get("item_attribute");
                }

                const size_t count = val.length();
                for (size_t i = 0; i < count; i++)
                {
                    bool prop_changed = false;
                    snprintf(buf, sizeof(buf), "%s[%zu]", name.c_str(), i);

                    property_def prop;
                    prop.getter     = list_get;
                    prop.setter     = list_set;
                    prop.attributes = item_attribute;

                    value index = value((int64_t)i);
                    render_property(
                        val, buf, prop, db, on_locate, prop_changed, &index);

                    if (prop.attributes.is_valid() &&
                        prop.attributes.is_map() &&
                        prop.attributes.get("remove").is_valid() &&
                        prop.attributes.get("remove").as_bool())
                    {
                        if (ImGui::Button("Remove"))
                        {
                            val.remove_at(i);
                            prop_changed = true;
                        }
                    }

                    if (prop_changed)
                        changed = true;
                }

                if (prop.attributes.is_valid() && prop.attributes.is_map() &&
                    prop.attributes.get("new_item").is_valid())
                {
                    auto new_item = prop.attributes.get("new_item");
                    if (ImGui::Button("Add"))
                    {
                        native_script_args args;
                        new_item.call(nullptr, nullptr, &args);
                        if (args.m_returns.size() > 0)
                            val.push(args.m_returns[0]);
                        changed = true;
                    }
                }
                ImGui::TreePop();
            }

            ImGui::PopID();
        }
        else
        {
            ImGui::Text(
                "%s: %s", name.c_str(), val.is_nil() ? "nil" : "Unknown");
        }
    }

    if (!prop.setter.is_valid())
        ImGui::PopItemFlag();
    ImGui::PopID();

    if (changed && prop.setter.is_valid())
    {
        native_script_args args_set;
        args_set.push_arg(value(obj));
        args_set.push_arg(new_val);
        prop.setter.call(nullptr, nullptr, &args_set);
    }
}

void property_grid::render_object(base_object *obj,
                                  const rtti &type,
                                  asset_database *db,
                                  delegate<void(const string &)> on_locate)
{
    if (!obj)
        return;

    ImGui::PushID(obj);

    reflection &r = type.ensure_reflection();

    for (const auto &name : r.property_order)
    {
        auto it = r.properties.find(name);
        if (it != r.properties.end())
        {
            bool changed = false;
            render_property(obj, name, it->value, db, on_locate, changed);
        }
    }

    ImGui::PopID();
}

} // namespace zabato::editor
