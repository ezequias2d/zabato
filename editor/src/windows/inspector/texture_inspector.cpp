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

class texture_inspector
{
public:
    void init(editor_app &app) { m_app = &app; }
    void draw(resource_ref texture);

private:
    editor_app *m_app = nullptr;
    shared_ptr<texture> m_current_texture;
};

void texture_inspector::draw(resource_ref texture_ref)
{
    if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
    {
        gpu *g = m_app->get_gpu();
        if (!g)
            return;

        shared_ptr<texture> t = texture_ref.get<texture>();

        if (m_current_texture != t)
            m_current_texture = t;

        ImVec2 region = ImGui::GetContentRegionAvail();
        float size    = region.x;

        // Workaround for flickering scrollbar
        if (ImGui::GetScrollMaxY() == 0.0f)
            size -= ImGui::GetStyle().ScrollbarSize;

        if (size < 128)
            size = 128;

        if (t)
        {
            float aspect = (float)t->get_size().x / (float)t->get_size().y;
            ImGui::Image((void *)t.get(), ImVec2(size, size / aspect));
        }
    }
}

void register_texture_inspector()
{
    texture_inspector *inspector = new texture_inspector();
    editor_registry::register_preview(
        texture::TYPE,
        [inspector](void *obj, editor_app &app, real dtime)
        {
            resource_ref *texture_ref = (resource_ref *)obj;
            inspector->init(app);
            inspector->draw(*texture_ref);
        });
}
} // namespace zabato::editor