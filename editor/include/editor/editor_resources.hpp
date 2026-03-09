#pragma once

#include <editor/asset_database.hpp>
#include <zabato/gpu.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/resource.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/string.hpp>

namespace zabato::editor
{

enum class editor_icon
{
    file,
    folder,
    mesh,
    font,
    lua_script,
    camera,
    light,
    hand,
    move,
    rotate,
    scale,
    directonal_light,
    spot_light,
    point_light,
    texture,
    audio,
    material,
    shader,
    zfile,
    dot0,
    dot1,
    dot2,
    dot3,
    dot4,
    dot5,
    dot6,
    dot7,
    dot10,
    dot11,
    dot12,
    dot13,
    dot14,
    dot15,
    dot16,
    dot17,
    unknown
};

class editor_resources
{
public:
    void init(zabato::resource_manager *res_mgr, zabato::gpu *gpu);
    void shutdown();

    editor_icon get_icon_for_asset_type(asset_type type);

    static void install_custom_icons();
    const char *get_icon_str(editor_icon icon);
    uint32_t get_icon_codepoint(editor_icon icon);
    tuple<texture *, box2<real>> get_icon(editor_icon icon);

private:
    resource_manager *m_res_mgr = nullptr;
    gpu *m_gpu                  = nullptr;
    hash_map<editor_icon, uint32_t> m_icon_codepoints;
    char m_tmp_utf8[5] = {0};
};

} // namespace zabato::editor
