#include <editor/core/editor_registry.hpp>
#include <editor/editor.hpp>
#include <editor/object_resource.hpp>
#include <imgui.h>
#include <zabato/base64.hpp>
#include <zabato/gpu.hpp>
#include <zabato/resource.hpp>
#include <zabato/stb/image_utils.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato::editor
{
class object_inspector
{
public:
    void init(editor_app &app) { m_app = &app; }

    void draw(resource_ref obj_ref);

private:
    shared_ptr<texture> load_preview(const string &path);

    editor_app *m_app = nullptr;

    // Preview state
    shared_ptr<texture> m_preview_tex;
    string m_current_path;
};

shared_ptr<texture> object_inspector::load_preview(const string &path)
{
    vector<uint8_t> jpg_data;
    int w, h;
    stb::comp comp;
    vector<uint8_t> pixels;
    tinyxml2::XMLElement *root, *meta, *thumb;
    const char *thumb_base64;
    shared_ptr<texture> tex = {};
    xml_serializer s;
    vector<uint8_t> b;

    auto rm = m_app->get_resource_manager();
    if (!rm)
        return {};
    auto fs = rm->get_file_system();
    if (!fs)
        return {};

    b = fs->read_all_bytes(path);
    if (b.empty())
        return {};

    s.set_manager(rm);
    if (s.doc().Parse((const char *)b.data()) != tinyxml2::XML_SUCCESS)
        return {};

    root = s.doc().RootElement();
    if (!root)
        return {};

    meta = root->FirstChildElement("metadata");
    if (!meta)
        return {};

    thumb = meta->FirstChildElement("thumbnail");
    if (!thumb)
        return {};

    thumb_base64 = thumb->GetText();
    if (!thumb_base64)
        return {};

    jpg_data = base64::decode(thumb_base64);
    pixels   = stb::load_image_from_memory(jpg_data, w, h, comp, 4);

    if (!pixels.empty())
    {
        tex = shared_ptr<texture>(
            m_app->get_gpu()->create_texture(w, h, color_format::rgba8888));
        tex->load(w, h, color_format::rgba8888, pixels.size(), pixels.data());
    }

    return tex;
}

void object_inspector::draw(resource_ref obj_ref)
{
    if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Check if loading a new object
        if (m_current_path != obj_ref.c_path())
        {
            m_current_path = obj_ref.c_path();
            m_preview_tex  = load_preview(m_current_path);
        }

        ImVec2 region = ImGui::GetContentRegionAvail();
        float size    = region.x;

        if (ImGui::GetScrollMaxY() == 0.0f)
            size -= ImGui::GetStyle().ScrollbarSize;
        if (size < 128)
            size = 128;

        if (m_preview_tex)
        {
            ImGui::Image((void *)m_preview_tex.get(),
                         ImVec2(size, size),
                         ImVec2(0, 1),
                         ImVec2(1, 0));

            float avail = ImGui::GetContentRegionAvail().x;
            float btn_w = 160;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                 (avail - btn_w) * 0.5f);

            if (ImGui::Button("Regenerate Thumbnail", ImVec2(btn_w, 30)))
            {
                if (m_app->generate_and_save_thumbnail(m_current_path))
                {
                    m_preview_tex = load_preview(m_current_path);
                }
            }
        }
        else
        {
            ImGui::Dummy(ImVec2(0, (size - 30) * 0.5f));
            float avail = ImGui::GetContentRegionAvail().x;
            float btn_w = 160;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                 (avail - btn_w) * 0.5f);

            if (ImGui::Button("Generate Thumbnail", ImVec2(btn_w, 30)))
            {
                if (m_app->generate_and_save_thumbnail(m_current_path))
                {
                    m_preview_tex = load_preview(m_current_path);
                }
            }
        }
    }
}

void register_object_inspector()
{
    object_inspector *inspector = new object_inspector();
    editor_registry::register_preview(
        object_resource::TYPE,
        [inspector](void *obj, editor_app &app, real dtime)
        {
            resource_ref *ref = (resource_ref *)obj;
            inspector->init(app);
            inspector->draw(*ref);
        });
}

} // namespace zabato::editor
