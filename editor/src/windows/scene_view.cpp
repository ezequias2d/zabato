#include <editor/editor.hpp>
#include <editor/gizmo_registry.hpp>
#include <editor/windows/scene_view.hpp>

#include <zabato/bounding_volume.hpp>
#include <zabato/error.hpp>
#include <zabato/gizmos.hpp>
#include <zabato/imgui.hpp>
#include <zabato/material.hpp>
#include <zabato/math.hpp>
#include <zabato/model.hpp>
#include <zabato/node.hpp>
#include <zabato/object.hpp>
#include <zabato/object_resource.hpp>
#include <zabato/resource.hpp>
#include <zabato/spatial.hpp>
#include <zabato/symbol.hpp>
#include <zabato/uuid.hpp>

namespace zabato::editor
{

scene_view_window::scene_view_window() : m_viewport("Scene View") {}

void scene_view_window::init(window *win)
{
    m_camera = new camera();
    m_viewport.init();

    m_window = win;
    m_window->add_cursor_move_callback(
        window::cursor_move_callback::
            from_method<scene_view_window, &scene_view_window::on_cursor_move>(
                this));
}

void scene_view_window::on_cursor_move(window *w,
                                       real x,
                                       real y,
                                       real dx,
                                       real dy)
{
    if (m_is_dragging)
    {
        m_accumulated_delta += vec2<real>(dx, dy);
    }
}

void scene_view_window::shutdown()
{
    if (m_window)
    {
        m_window->remove_cursor_move_callback(
            window::cursor_move_callback::from_method<
                scene_view_window,
                &scene_view_window::on_cursor_move>(this));
    }
}

void scene_view_window::focus(const vec3<real> &center, real radius)
{
    real fov  = m_camera->get_fov();
    real dist = radius / sin(fov * 0.5);
    dist *= 1.2f;

    if (dist < 0.5f)
        dist = 0.5f;

    vec3<real> forward =
        (m_camera->get_local().rotate() * vec3<real>(0, 0, -1));
    m_camera_position = center - forward * dist;

    m_camera->look_at(
        m_camera_position, m_camera_position + forward, {0, 1, 0});
}

void scene_view_window::look_along(const vec3<real> &dir)
{
    if (length_sq(dir) < 0.0001f)
        return;

    vec3<real> d = normalize(dir);

    real pitch_rad      = asin(d.y);
    m_camera_rotation.x = to_deg(pitch_rad);

    real yaw_rad        = atan2(d.x, -d.z);
    m_camera_rotation.y = to_deg(yaw_rad);
}

void scene_view_window::look_at(const vec3<real> &target)
{
    m_camera->look_at(m_camera_position, target, {0, 1, 0});
    vec3<real> dir = target - m_camera_position;
    look_along(dir);
}

void scene_view_window::update(real dt)
{
    if (m_viewport.is_focused() || m_viewport.is_hovered() || m_is_dragging)
    {
        if (!ImGui::GetCurrentContext())
            return;

        ImGuiIO &io = ImGui::GetIO();

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            if (!m_is_dragging)
            {
                m_is_dragging = true;
                m_window->hold_cursor();
                m_accumulated_delta = {0, 0};
            }

            if (m_accumulated_delta.x != 0 || m_accumulated_delta.y != 0)
            {
                m_camera_rotation.y -=
                    m_accumulated_delta.x * real(m_sensitivity) * dt * real(45);
                m_camera_rotation.x -=
                    m_accumulated_delta.y * real(m_sensitivity) * dt * real(45);

                m_accumulated_delta = {0, 0};

                // Clamp pitch
                if (m_camera_rotation.x > 89)
                    m_camera_rotation.x = 89;
                if (m_camera_rotation.x < -89)
                    m_camera_rotation.x = -89;
            }
        }
        else
        {
            if (m_is_dragging)
            {
                m_is_dragging = false;
                m_window->release_cursor();
            }
        }

        // Update Camera Rotation
        real yaw_rad   = to_rad(m_camera_rotation.y);
        real pitch_rad = to_rad(m_camera_rotation.x);

        mat4<real> rot = mat4<real>::identity();
        rot            = rot * mat4_rotation_y(yaw_rad);
        rot            = rot * mat4_rotation_x(pitch_rad);

        vec3<real> forward = (rot * vec4<real>(0, 0, -1, 0)).xyz();
        vec3<real> right   = (rot * vec4<real>(1, 0, 0, 0)).xyz();
        vec3<real> up      = (rot * vec4<real>(0, 1, 0, 0)).xyz();

        if (m_is_dragging)
        {
            vec3<real> move_dir = {0, 0, 0};

            if (ImGui::IsKeyDown(ImGuiKey_W))
                move_dir += forward;
            if (ImGui::IsKeyDown(ImGuiKey_S))
                move_dir -= forward;
            if (ImGui::IsKeyDown(ImGuiKey_A))
                move_dir -= right;
            if (ImGui::IsKeyDown(ImGuiKey_D))
                move_dir += right;
            if (ImGui::IsKeyDown(ImGuiKey_E))
                move_dir += vec3<real>(0, 1, 0); // Up
            if (ImGui::IsKeyDown(ImGuiKey_Q))
                move_dir -= vec3<real>(0, 1, 0); // Down

            if (length_sq(move_dir) > 0)
            {
                move_dir = normalize(move_dir);
                float speed_mult =
                    ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 3.0f : 1.0f;
                m_camera_position += move_dir * m_speed * speed_mult * dt;
            }
        }

        // Apply transform
        auto t = m_camera->get_local();
        t.set_translate(m_camera_position);
        m_camera->set_local(t);

        m_camera->set_perspective(to_rad(real(45)),
                                  m_viewport.get_size().x /
                                      m_viewport.get_size().y,
                                  real(0.1),
                                  real(100.0));
        m_camera->look_at(
            m_camera_position, m_camera_position + forward, {0, 1, 0});
    }
}

void scene_view_window::render(world &w, renderer &r, gpu &g, editor_app &app)
{
    m_viewport.set_on_scene_render(
        [&](world &w_arg, camera &c_arg, gpu &g_arg)
        { this->on_scene_render(w_arg, c_arg, g_arg, app); });

    m_viewport.set_on_overlay_render(
        [&](world &w_arg, camera &c_arg)
        { this->on_overlay_render(w_arg, c_arg, app); });

    m_viewport.set_on_drop(
        [&](const char *path)
        {
            const string asset_path = path;
            const string ext        = asset_path.substr(asset_path.rfind('.'));
            resource_manager *rm    = app.get_resource_manager();

            if (ext == ".zfile")
            {
                game_message msg;
                msg.msg_id    = cmd_instantiate_prefab;
                msg.sender_id = uuid();
                msg.data      = value(asset_path.c_str());
                app.send_message(msg);
                return;
            }

            if (rm->is_resource_type<object_resource>(asset_path))
            {
                resource_ref res;
                res.set_path(asset_path);
                res.set_manager(rm);
                auto result = res.get<object_resource>();
                if (result)
                {
                    auto obj =
                        c_dynamic_cast<spatial>(result->get_object().get());
                    if (!obj)
                    {
                        report(
                            report_type::error,
                            "Failed to instantiate a non-spatial object (%s).",
                            obj->type().name());
                        return;
                    }

                    auto clone = obj->clone(*rm);
                    if (!clone)
                    {
                        report(report_type::error,
                               "Failed to clone object (%s).",
                               obj->type().name());
                        return;
                    }

                    auto clone_cast = c_dynamic_cast<spatial>(clone);
                    if (!clone_cast)
                    {
                        report(report_type::error,
                               "Failed to cast clone to spatial (%s).",
                               clone->type().name());
                        return;
                    }

                    if (auto root = w.get_scene_root())
                    {
                        if (auto n = c_dynamic_cast<node>(root.get()))
                        {
                            n->attach_child(clone_cast);
                        }
                        else
                        {
                            report(report_type::error,
                                   "Failed to attach node to root: %s",
                                   root->name());
                        }
                    }

                    return;
                }
            }

            if (rm->is_resource_type<mesh>(asset_path))
            {
                // Create Model
                pointer<model> mdl = new model();
                mdl->set_resource_manager(rm);
                mdl->set_mesh(asset_path.c_str());

                // Set name from filename
                size_t last_slash = asset_path.rfind('/');
                string filename   = (last_slash == string::npos)
                                        ? asset_path
                                        : asset_path.substr(last_slash + 1);
                mdl->set_name(filename.c_str());

                // Attach to World Root
                if (auto root = w.get_scene_root())
                {
                    if (auto n = c_dynamic_cast<node>(root.get()))
                    {
                        n->attach_child(mdl);
                    }
                    else
                    {
                        report(report_type::error,
                               "Failed to attach model to root: %s",
                               root->name());
                    }
                }
            }
        });

    m_viewport.render(w, r, m_camera, g);
}

void scene_view_window::on_message(const game_message &msg)
{
    if (msg.msg_id == cmd_select)
    {
        object *obj = nullptr;
        if (object::s_in_use.try_get_value(msg.receiver_id, obj))
        {
            // Check if already selected
            bool found = false;
            for (auto *s : m_selection)
                if (s == obj)
                    found = true;

            if (!found)
                m_selection.push_back(obj);
        }
    }
    else if (msg.msg_id == cmd_focus_selection)
    {
        if (m_selection.empty())
            return;

        vec3<real> min_pt = {1e9, 1e9, 1e9};
        vec3<real> max_pt = {-1e9, -1e9, -1e9};

        for (auto *obj : m_selection)
            if (auto *spat = c_dynamic_cast<spatial>(obj))
                spatial::get_global_bounds(spat, min_pt, max_pt, true);

        bool any_valid = min_pt != vec3<real>(1e9, 1e9, 1e9) ||
                         max_pt != vec3<real>(-1e9, -1e9, -1e9);

        if (any_valid)
        {
            vec3<real> center = (min_pt + max_pt) * 0.5f;
            real radius       = length(max_pt - min_pt) * 0.5f;
            focus(center, radius);
        }
    }
    else if (msg.msg_id == cmd_deselect)
    {
        if (msg.receiver_id == uuid::null())
        {
            m_selection.clear();
        }
        else
        {
            for (auto it = m_selection.begin(); it != m_selection.end();)
            {
                if ((*it)->id() == msg.receiver_id)
                    it = m_selection.erase(it);
                else
                    ++it;
            }
        }
    }
    else if (msg.msg_id == evt_scene_change)
    {
        m_selection.clear();
    }
    else if (msg.msg_id == cmd_change_tool)
    {
        if (msg.data.is_int())
            m_active_tool = (tool_mode)msg.data.as_int();
    }
}

void scene_view_window::on_scene_render(world &w,
                                        camera &cam,
                                        gpu &gpu,
                                        editor_app &app)
{

    draw_grid(gpu, {.cam = cam, .size = 128, .steps = 128});
    m_is_gizmo_hovered = false;

    m_hovered_icon      = nullptr;
    m_hovered_icon_dist = real::max_val();

    auto root = w.get_scene_root();

    // Draw Gizmos
    if (root)
    {
        // Occluded
        gpu.set_depth_func(depth_func::greater);
        gpu.set_depth_write(false);

        delegate<void(spatial *)> draw_pass_occluded = [&](spatial *s)
        {
            if (!s)
                return;
            color tint = {real(0.4), real(0.4), real(0.4), real(0.3)};
            gizmo_context ctx{
                .gpu       = gpu,
                .cam       = cam,
                .mouse_ray = &m_latest_ray,
                .app       = app,
                .color     = tint,
                .occluded  = true,

                // output
                .hovered  = m_hovered_icon,
                .hit_dist = m_hovered_icon_dist,
                .selected = false,
            };

            for (auto *sel : m_selection)
                if (sel == s)
                    ctx.selected = true;

            gizmo_registry::draw(s, ctx);

            auto *n = c_dynamic_cast<node>(s);
            if (n)
            {
                int q = n->quantity();
                for (int i = 0; i < q; ++i)
                    draw_pass_occluded(n->child_at(i));
            }
        };
        draw_pass_occluded(root);

        // Visible
        gpu.set_depth_func(depth_func::less_equal);
        gpu.set_depth_write(true);

        delegate<void(spatial *)> draw_pass_visible = [&](spatial *s)
        {
            if (!s)
                return;

            bool is_selected_node = false;
            for (auto *sel : m_selection)
                if (sel == s)
                {
                    is_selected_node = true;
                    break;
                }

            gizmo_context ctx{
                .gpu       = gpu,
                .cam       = cam,
                .mouse_ray = &m_latest_ray,
                .app       = app,
                .color     = color::white(),
                .occluded  = false,

                // output
                .hovered  = m_hovered_icon,
                .hit_dist = m_hovered_icon_dist,
                .selected = is_selected_node,
            };

            editor::gizmo_registry::draw(s, ctx);

            if (ctx.hovered != m_hovered_icon)
            {
                m_hovered_icon      = ctx.hovered;
                m_hovered_icon_dist = ctx.hit_dist;
            }

            // Draw Selection Highlight
            if (is_selected_node)
            {
                auto *mod = c_dynamic_cast<model>(s);
                if (mod)
                {
                    shared_ptr<mesh> m = mod->get_mesh();
                    if (m)
                    {
                        gpu.push_matrix();

                        transformation t     = s->get_world_transform();
                        mat4<real> model_mat = mat4_translation(t.translate()) *
                                               mat4_from_quat(t.rotate()) *
                                               mat4_scaling(t.scale());

                        gpu.mult_matrix(model_mat);
                        gpu.scale(1.002, 1.002, 1.002);

                        wire_mesh_options opts = {
                            .m     = *m,
                            .color = color::rosa_felps(),
                        };
                        draw_wire_mesh(gpu, opts);
                        gpu.pop_matrix();
                    }
                }
            }

            auto *n = c_dynamic_cast<node>(s);
            if (n)
            {
                int q = n->quantity();
                for (int i = 0; i < q; ++i)
                    draw_pass_visible(n->child_at(i));
            }
        };
        draw_pass_visible(root);

        // Restore defaults
        gpu.set_depth_func(depth_func::less);
    }

    if (!m_selection.empty())
    {
        vec3<real> centroid(0, 0, 0);
        int valid_count = 0;

        for (auto *obj : m_selection)
        {
            auto *spat = c_dynamic_cast<spatial>(obj);
            if (spat)
            {
                centroid += spat->get_world_transform().translate();
                valid_count++;
            }
        }

        if (valid_count > 0)
        {
            centroid /= (real)valid_count;

            transformation t;

            t.make_identity();
            t.set_translate(centroid);

            if (m_selection.size() == 1)
            {
                if (auto *spat = c_dynamic_cast<spatial>(m_selection.back()))
                    t.set_rotate(spat->get_world_transform().rotate());
            }

            if (m_active_tool == tool_mode::move)
            {
                move_gizmo_options opts = {

                    .cam           = cam,
                    .position      = t.translate(),
                    .is_mouse_down = m_is_mouse_down,
                    .mouse_ray     = m_latest_ray,
                    .out_hovered   = &m_is_gizmo_hovered,
                };

                vec3<real> start_pos = opts.position;

                if (draw_move_gizmo(gpu, opts))
                {
                    vec3<real> delta = opts.position - start_pos;

                    // Apply delta to all
                    for (auto *obj : m_selection)
                    {
                        if (auto *spat = c_dynamic_cast<spatial>(obj))
                        {
                            transformation lt = spat->get_local();

                            auto parent            = spat->parent();
                            vec3<real> local_delta = delta;
                            if (parent)
                            {
                                // Rotate delta by inverse parent rotation
                                local_delta =
                                    inverse(parent->get_world_transform()
                                                .rotate()) *
                                    delta;
                            }

                            lt.set_translate(lt.translate() + local_delta);
                            spat->set_local(lt);
                        }
                    }
                }
            }
            else if (m_active_tool == tool_mode::rotate)
            {
                vec3<real> pos = t.translate();

                // Reset proxy if not dragging
                if (!m_is_mouse_down)
                    m_gizmo_rotation = t.rotate();

                quat<real> prev_rot = m_gizmo_rotation;

                rotate_gizmo_options opts = {
                    .cam           = cam,
                    .position      = t.translate(),
                    .rotation      = m_gizmo_rotation,
                    .is_mouse_down = m_is_mouse_down,
                    .mouse_ray     = m_latest_ray,
                    .out_hovered   = &m_is_gizmo_hovered,
                };

                if (draw_rotate_gizmo(gpu, opts))
                {
                    m_gizmo_rotation = opts.rotation;
                    // Incremental Delta: New * Inverse(Old)
                    quat<real> delta_rot = m_gizmo_rotation * inverse(prev_rot);

                    for (auto *obj : m_selection)
                    {
                        if (auto *spat = c_dynamic_cast<spatial>(obj))
                        {
                            // Local Transform
                            transformation lt = spat->get_local();

                            quat<real> new_local_rot;

                            auto parent = spat->parent();
                            if (parent)
                            {
                                quat<real> p_rot =
                                    parent->get_world_transform().rotate();
                                quat<real> inv_p = inverse(p_rot);

                                // R_l' = (inv_P * D * P) * R_l
                                quat<real> relative_delta =
                                    inv_p * delta_rot * p_rot;
                                new_local_rot = relative_delta * lt.rotate();
                            }
                            else
                            {
                                // Parent Identity
                                // R_l' = D * R_l
                                new_local_rot = delta_rot * lt.rotate();
                            }

                            lt.set_rotate(new_local_rot);
                            spat->set_local(lt);
                        }
                    }
                }
            }
            else if (m_active_tool == tool_mode::scale)
            {
                vec3<real> pos = t.translate();
                quat<real> rot = t.rotate();

                // Reset proxy if not dragging
                if (!m_is_mouse_down)
                    m_gizmo_scale = {1, 1, 1};

                vec3<real> prev_scale = m_gizmo_scale;

                scale_gizmo_options opts = {
                    .cam           = cam,
                    .position      = t.translate(),
                    .scale         = m_gizmo_scale,
                    .rotation      = t.rotate(),
                    .is_mouse_down = m_is_mouse_down,
                    .mouse_ray     = m_latest_ray,
                    .out_hovered   = &m_is_gizmo_hovered,
                };
                if (draw_scale_gizmo(gpu, opts))
                {
                    m_gizmo_scale    = opts.scale;
                    vec3<real> delta = {1, 1, 1};

                    if (abs(prev_scale.x) > 1e-6)
                        delta.x = m_gizmo_scale.x / prev_scale.x;
                    if (abs(prev_scale.y) > 1e-6)
                        delta.y = m_gizmo_scale.y / prev_scale.y;
                    if (abs(prev_scale.z) > 1e-6)
                        delta.z = m_gizmo_scale.z / prev_scale.z;

                    for (auto *obj : m_selection)
                    {
                        if (auto *spat = c_dynamic_cast<spatial>(obj))
                        {
                            transformation lt = spat->get_local();
                            lt.set_scale(lt.scale() * delta);
                            spat->set_local(lt);
                        }
                    }
                }
            }
        }
    }
}

bool draw_orientation_gizmo(const orientation_gizmo_options &options)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 p              = ImGui::GetCursorScreenPos();
    ImVec2 center         = ImVec2((float)((real)p.x + options.position.x),
                           (float)((real)p.y + options.position.y));

    vec3<real> axes[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    ImU32 colors[]    = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000};
    char labels[]     = {'X', 'Y', 'Z'};
    auto view         = options.cam.get_view();

    struct AxisDepth
    {
        int index;
        vec3<real> proj;
        real z;
    };
    AxisDepth projected[3];

    for (int i = 0; i < 3; ++i)
    {
        vec3<real> &a = axes[i];
        real x        = view[0][0] * a.x + view[1][0] * a.y + view[2][0] * a.z;
        real y        = view[0][1] * a.x + view[1][1] * a.y + view[2][1] * a.z;
        real z        = view[0][2] * a.x + view[1][2] * a.y + view[2][2] * a.z;
        projected[i]  = {i, {x, y, z}, z};
    }

    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < 2 - i; ++j)
            if (projected[j].z > projected[j + 1].z)
            {
                auto temp        = projected[j];
                projected[j]     = projected[j + 1];
                projected[j + 1] = temp;
            }

    for (int i = 0; i < 3; ++i)
    {
        int idx      = projected[i].index;
        auto v       = projected[i].proj;
        ImVec2 p_end = ImVec2((float)((real)center.x + v.x * options.size),
                              (float)((real)center.y - v.y * options.size));

        draw_list->AddLine(center, p_end, colors[idx], 3);
        draw_list->AddText(
            p_end, colors[idx], (char *)&labels[idx], (char *)&labels[idx] + 1);

        ImGui::SetCursorScreenPos(ImVec2(p_end.x - 7, p_end.y - 7));
        char btn_id[4] = "##?";
        btn_id[2]      = labels[idx];

        ImGui::InvisibleButton(btn_id, ImVec2(14, 14));

        if (ImGui::IsItemHovered())
            draw_list->AddCircleFilled(p_end, 8, colors[idx] & 0x88FFFFFF);

        if (ImGui::IsItemClicked())
        {
            options.out_dir = axes[idx] * -1;
            return true;
        }
    }
    return false;
}

void scene_view_window::on_overlay_render(world &w,
                                          camera &cam,
                                          editor_app &app)
{
    using namespace zabato;

    // Toolbar UI
    ImVec2 start_pos = ImGui::GetCursorStartPos();
    ImGui::SetCursorPos(ImVec2(start_pos.x + 10, start_pos.y + 10)); // Top-left
    ImGui::BeginGroup();

    auto tool_button =
        [&](const char *label, tool_mode mode, editor_icon icon_id)
    {
        bool is_active = (m_active_tool == mode);
        if (is_active)
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.2f, 0.6f, 1.0f, 1.0f));

        const char *icon_str = app.get_resources()->get_icon_str(icon_id);
        string btn_label     = string(icon_str) + label;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        bool clicked = ImGui::Button(btn_label.c_str());
        ImGui::PopStyleColor();

        if (clicked)
        {
            // Send Tool Changed Message
            game_message msg;
            msg.msg_id    = cmd_change_tool;
            msg.sender_id = uuid();
            msg.data      = value((int64_t)mode);
            app.send_message(msg);
        }

        if (is_active)
            ImGui::PopStyleColor();

        // Tooltip
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", label);
        }

        ImGui::SameLine();
    };

    tool_button("Select", tool_mode::hand, editor_icon::hand);
    tool_button("Move", tool_mode::move, editor_icon::move);
    tool_button("Rotate", tool_mode::rotate, editor_icon::rotate);
    tool_button("Scale", tool_mode::scale, editor_icon::scale);
    ImGui::EndGroup();
    bool toolbar_hovered = ImGui::IsItemHovered();

    // Interaction Logic

    // Per-frame ray update for dragging etc.
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 win_pos   = ImGui::GetWindowPos();
    ImVec2 win_size  = ImGui::GetWindowSize();
    vec2<real> screen_pos(mouse_pos.x - win_pos.x, mouse_pos.y - win_pos.y);
    vec2<real> screen_size(win_size.x, win_size.y);

    m_latest_ray    = get_screen_ray(cam, screen_pos, screen_size);
    m_is_mouse_down = ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(1);

    /*
        Allow picking if:
    - Mouse Clicked
    - Scene View Window is hovered
    - AND (No item is hovered OR The item hovered IS the scene image)
      This prevents picking when clicking toolbar buttons, but allows it on the
    scene.
    */
    bool image_hovered = m_viewport.is_image_hovered();
    bool item_hovered  = ImGui::IsAnyItemHovered();
    bool ui_blocking   = item_hovered && !image_hovered;

    /* Explicitly block picking if hovering toolbar */
    if (toolbar_hovered)
        ui_blocking = true;

    if (ImGui::IsMouseClicked(0) && image_hovered && !ui_blocking &&
        !m_is_gizmo_hovered)
    {
        auto [picked_model, picked_dist] = pick_object(w, m_latest_ray);

        pointer<object> picked_obj = nullptr;

        // Compare with Icon Hit
        if (m_hovered_icon && m_hovered_icon_dist < picked_dist)
        {
            picked_obj = c_dynamic_cast<object>(m_hovered_icon.get());
        }
        else if (picked_model)
        {
            picked_obj = picked_model;
        }

        bool ctrl  = ImGui::GetIO().KeyCtrl;
        bool shift = ImGui::GetIO().KeyShift;

        /* Logic:
            - None: Clear, Select
            - Ctrl/Shift: Toggle/Add
        */

        if (!ctrl && !shift)
        {
            /*
             - If clicked nothing, clear.
             - If clicked something not selected, clear then select.
             - If clicked something selected, keep selection (unless we want to
             support single select interaction? Usually click selects only that
             one if modifiers absent)
            */
            bool already_selected = false;
            for (auto *s : m_selection)
                if (s == picked_obj)
                    already_selected = true;

            if (picked_obj)
            {
                // Replace selection
                game_message clear_msg;
                clear_msg.msg_id      = cmd_deselect;
                clear_msg.sender_id   = uuid();
                clear_msg.receiver_id = uuid::null();
                app.send_message(clear_msg);

                game_message sel_msg;
                sel_msg.msg_id      = cmd_select;
                sel_msg.sender_id   = uuid();
                sel_msg.receiver_id = picked_obj->id();
                app.send_message(sel_msg);
            }
            else
            {
                // Deselect all
                game_message clear_msg;
                clear_msg.msg_id      = cmd_deselect;
                clear_msg.sender_id   = uuid();
                clear_msg.receiver_id = uuid::null();
                app.send_message(clear_msg);
            }
        }
        else if (picked_obj)
        {
            bool already_selected = false;
            for (auto *s : m_selection)
                if (s == picked_obj)
                    already_selected = true;

            if (already_selected)
            {
                game_message msg;
                msg.msg_id      = cmd_deselect;
                msg.sender_id   = uuid();
                msg.receiver_id = picked_obj->id();
                app.send_message(msg);
            }
            else
            {
                game_message msg;
                msg.msg_id      = cmd_select;
                msg.sender_id   = uuid();
                msg.receiver_id = picked_obj->id();
                app.send_message(msg);
            }
        }
    }

    // Orientation Gizmo
    ImGui::SetCursorPos(ImVec2(win_size.x - 60.0f, start_pos.y + 60.0f));
    vec3<real> target_dir;

    orientation_gizmo_options opts = {
        .cam      = cam,
        .position = {0.0f, 0.0f},
        .size     = 40.0f,
        .out_dir  = target_dir,
    };
    if (draw_orientation_gizmo(opts))
    {
        look_along(target_dir);
    }
}

} // namespace zabato::editor
