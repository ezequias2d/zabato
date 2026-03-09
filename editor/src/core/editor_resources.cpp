#include <editor/editor_resources.hpp>
#include <stdio.h>

#include <zabato/error.hpp>
#include <zabato/gpu.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/imgui.hpp>
#include <zabato/importer.hpp>
#include <zabato/mesh.hpp>
#include <zabato/shape.hpp>
#include <zabato/stb/image_utils.hpp>
#include <zabato/vector.hpp>

namespace zabato::editor
{

struct pending_icon_data
{
    int id;
    int width;
    int height;
    vector<uint8_t> pixels;
};

static hash_map<editor_icon, uint32_t> g_icon_map;
static vector<pending_icon_data> g_pending_icons;

static void fill_atlas(unsigned char *pixels, int w, int h, void *user_data)
{
    ImFontAtlas *atlas = ImGui::GetIO().Fonts;

    for (const auto &icon : g_pending_icons)
    {
        ImFontAtlasRect rect;
        if (atlas->GetCustomRect(icon.id, &rect))
        {
            // Blit
            for (int y = 0; y < icon.height; y++)
            {
                for (int x = 0; x < icon.width; x++)
                {
                    int atlas_x = rect.x + x;
                    int atlas_y = rect.y + y;

                    if (atlas_x < w && atlas_y < h)
                    {
                        const uint8_t *src =
                            icon.pixels.data() + (y * icon.width + x) * 4;
                        uint8_t *dst = pixels + (atlas_y * w + atlas_x) * 4;

                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        dst[3] = src[3];
                    }
                }
            }
        }
    }
    g_pending_icons.clear();
}

void editor_resources::install_custom_icons()
{
    ImGuiIO &io        = ImGui::GetIO();
    ImFontAtlas *atlas = io.Fonts;

    struct IconDef
    {
        editor_icon id;
        const char *file;
    };

    IconDef icons[] = {
        {editor_icon::file, "icons/file.png"},
        {editor_icon::folder, "icons/folder.png"},
        {editor_icon::mesh, "icons/mesh.png"},
        {editor_icon::font, "icons/font.png"},
        {editor_icon::lua_script, "icons/lua_script.png"},
        {editor_icon::camera, "icons/camera.png"},
        {editor_icon::light, "icons/light.png"},
        {editor_icon::hand, "icons/hand.png"},
        {editor_icon::move, "icons/move.png"},
        {editor_icon::rotate, "icons/rotate.png"},
        {editor_icon::scale, "icons/scale.png"},
        {editor_icon::directonal_light, "icons/directional_light.png"},
        {editor_icon::spot_light, "icons/spot_light.png"},
        {editor_icon::point_light, "icons/point_light.png"},
        {editor_icon::dot0, "icons/dot0.png"},
        {editor_icon::dot1, "icons/dot1.png"},
        {editor_icon::dot2, "icons/dot2.png"},
        {editor_icon::dot3, "icons/dot3.png"},
        {editor_icon::dot4, "icons/dot4.png"},
        {editor_icon::dot5, "icons/dot5.png"},
        {editor_icon::dot6, "icons/dot6.png"},
        {editor_icon::dot7, "icons/dot7.png"},
        {editor_icon::dot10, "icons/dot10.png"},
        {editor_icon::dot11, "icons/dot11.png"},
        {editor_icon::dot12, "icons/dot12.png"},
        {editor_icon::dot13, "icons/dot13.png"},
        {editor_icon::dot14, "icons/dot14.png"},
        {editor_icon::dot15, "icons/dot15.png"},
        {editor_icon::dot16, "icons/dot16.png"},
        {editor_icon::dot17, "icons/dot17.png"},
        {editor_icon::texture, "icons/texture.png"},
        {editor_icon::material, "icons/material.png"},
        {editor_icon::zfile, "icons/zfile.png"},
        {editor_icon::shader, "icons/shader.png"},
        {}};

    uint32_t current_codepoint = 0xf000;

    for (const auto &def : icons)
    {
        if (!def.file)
            continue;

        FILE *file = fopen(def.file, "rb");
        if (!file)
        {
            report(report_type::error,
                   "Failed to load icon for atlas (fopen): %s",
                   def.file);
            continue;
        }
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);

        vector<uint8_t> data;
        data.resize(size);
        size_t readed = fread(data.data(), 1, data.size(), file);
        assert(readed == size);
        fclose(file);

        int w, h;
        stb::comp c;
        vector<uint8_t> px = stb::load_image_from_memory(data, w, h, c, 4);
        if (!px.empty())
        {
            float font_size = io.Fonts->Fonts[0]->LegacySize;
            if (font_size <= 0.0f)
                font_size = 13.0f; // Fallback
            float offset_y = (font_size - h) * 0.5f;

            int id = atlas->AddCustomRectFontGlyph(io.Fonts->Fonts[0],
                                                   (ImWchar)current_codepoint,
                                                   w,
                                                   h,
                                                   w + 4.0f,
                                                   ImVec2(0, offset_y));

            g_pending_icons.push_back({id, w, h, move(px)});
            g_icon_map.add(def.id, current_codepoint);
            current_codepoint++;
        }
        else
        {
            report(report_type::error,
                   "Failed to load icon for atlas (stbi): %s",
                   def.file);
        }
    }

    imgui::set_atlas_pack_callback(fill_atlas, nullptr);
}

const char *editor_resources::get_icon_str(editor_icon icon)
{
    uint32_t c;
    if (m_icon_codepoints.try_get_value(icon, c))
    {
        // Encode utf8
        if (c <= 0x7F)
        {
            m_tmp_utf8[0] = (char)c;
            m_tmp_utf8[1] = 0;
        }
        else if (c <= 0x7FF)
        {
            m_tmp_utf8[0] = (char)(0xC0 | (c >> 6));
            m_tmp_utf8[1] = (char)(0x80 | (c & 0x3F));
            m_tmp_utf8[2] = 0;
        }
        else if (c <= 0xFFFF)
        {
            m_tmp_utf8[0] = (char)(0xE0 | (c >> 12));
            m_tmp_utf8[1] = (char)(0x80 | ((c >> 6) & 0x3F));
            m_tmp_utf8[2] = (char)(0x80 | (c & 0x3F));
            m_tmp_utf8[3] = 0;
        }
        else
        {
            m_tmp_utf8[0] = (char)(0xF0 | (c >> 18));
            m_tmp_utf8[1] = (char)(0x80 | ((c >> 12) & 0x3F));
            m_tmp_utf8[2] = (char)(0x80 | ((c >> 6) & 0x3F));
            m_tmp_utf8[3] = (char)(0x80 | (c & 0x3F));
            m_tmp_utf8[4] = 0;
        }
        return m_tmp_utf8;
    }
    return "";
}

uint32_t editor_resources::get_icon_codepoint(editor_icon icon)
{
    uint32_t c;
    if (m_icon_codepoints.try_get_value(icon, c))
    {
        return c;
    }
    return 0;
}

tuple<texture *, box2<real>> editor_resources::get_icon(editor_icon icon)
{
    ImGuiIO &io              = ImGui::GetIO();
    ImFont *font             = io.Fonts->Fonts[0];
    ImFontBaked *baked       = font->GetFontBaked(font->LegacySize);
    uint32_t codepoint       = get_icon_codepoint(icon);
    const ImFontGlyph *glyph = baked->FindGlyph((ImWchar)codepoint);

    box2<real> uv = {{0, 0}, {0, 0}};
    if (glyph)
    {
        uv = {{glyph->U0, glyph->V0}, {glyph->U1, glyph->V1}};
    }

    texture *tex = (texture *)io.Fonts->TexID.GetTexID();
    return {tex, uv};
}

void editor_resources::init(resource_manager *res_mgr, gpu *gpu)
{
    m_res_mgr = res_mgr;
    m_gpu     = gpu;

    for (auto &[id, code, _] : g_icon_map)
    {
        m_icon_codepoints.add_or_set(id, code);
    }
}

void editor_resources::shutdown() {}

editor_icon editor_resources::get_icon_for_asset_type(asset_type type)
{
    switch (type)
    {
    case asset_type::script:
        return editor_icon::lua_script;
    case asset_type::mesh:
        return editor_icon::mesh;
    case asset_type::texture:
        return editor_icon::texture;
    case asset_type::audio:
        return editor_icon::audio;
    case asset_type::scene:
        return editor_icon::zfile;
    case asset_type::shader:
        return editor_icon::shader;
    case asset_type::material:
        return editor_icon::material;
    default:
        return editor_icon::file;
    }
}

} // namespace zabato::editor
