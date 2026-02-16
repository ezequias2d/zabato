#include "preview_scene.hpp"
#include <editor/core/editor_registry.hpp>
#include <editor/editor.hpp>
#include <imgui.h>
#include <zabato/gpu.hpp>
#include <zabato/math.hpp>
#include <zabato/resource.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/transformation.hpp>

namespace zabato::editor
{

class mesh_inspector
{
public:
    void init(editor_app &app)
    {
        m_app = &app;
        m_preview.init(app);
    }
    void draw(resource_ref mesh, real dtime);

private:
    editor_app *m_app = nullptr;
    preview_scene m_preview;
    shared_ptr<mesh> m_current_mesh = nullptr;
};

void mesh_inspector::draw(resource_ref mesh_ref, real dtime)
{
    if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
    {
        gpu *g = m_app->get_gpu();
        if (!g)
            return;

        auto m = mesh_ref.get<mesh>();

        if (m_current_mesh != m)
        {
            vec3<real> min, max;
            m->get_bounds(min, max);

            vec3<real> center = (min + max) * 0.5f;
            real dist         = length(max - min) * 1.2f;
            if (dist < 0.1f)
                dist = 0.1f;

            m_preview.set_focus(center, dist);
            m_current_mesh = m;
        }

        ImVec2 region = ImGui::GetContentRegionAvail();
        float size    = region.x;

        // Workaround for flickering scrollbar
        if (ImGui::GetScrollMaxY() == 0.0f)
            size -= ImGui::GetStyle().ScrollbarSize;

        if (size < 128)
            size = 128;

        m_preview.set_mesh(mesh_ref.c_path(), mesh_ref.manager());

        m_preview.draw_controls();
        m_preview.render_default(size, size, dtime);
    }
}

void register_mesh_inspector()
{
    mesh_inspector *inspector = new mesh_inspector();
    editor_registry::register_preview(
        mesh::TYPE,
        [inspector](void *obj, editor_app &app, real dtime)
        {
            resource_ref *mesh_ref = (resource_ref *)obj;
            inspector->init(app);
            inspector->draw(*mesh_ref, dtime);
        });
}
} // namespace zabato::editor