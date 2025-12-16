#include <zabato/animator.hpp>

namespace zabato
{
void animator::play_animation(animation *anim, const mesh &mesh_ref, bool loop)
{
    m_current_animation = anim;
    m_loop              = loop;
    m_current_time      = real(0);
    m_bone_id_to_anim_bone_index.clear();

    for (auto &mat : m_final_bone_matrices)
        mat = mat4<real>::identity();

    if (!anim)
        return;

    const auto model_bone_count = mesh_ref.get_bone_count();
    if (model_bone_count == 0)
        return;

    int32_t max_bone_id = -1;
    for (uint16_t i = 0; i < model_bone_count; ++i)
    {
        bone_info bone = {};
        mesh_ref.get_bone(i, bone);
        if (bone.bone_id > max_bone_id)
            max_bone_id = bone.bone_id;
    }

    if (max_bone_id > -1)
    {
        m_bone_id_to_anim_bone_index.resize(max_bone_id + 1, -1);
        for (uint16_t i = 0; i < model_bone_count; ++i)
        {
            bone_info bone = {};
            mesh_ref.get_bone(i, bone);

            auto anim_bone = anim->find_bone_index(bone.name.c_str());
            m_bone_id_to_anim_bone_index[bone.bone_id] = anim_bone;
        }
    }
}

void animator::calculate_bone_transform(const animation_node *node,
                                        const mat4<real> &parent_transform)
{
    assert(node);

    auto animation            = m_current_animation;
    auto &animation_bones     = animation->get_bones();
    mat4<real> node_transform = node->transform;
    auto bone_info            = node->bone;
    int16_t anim_bone_index   = -1;
    anim_bone *anim_bone      = nullptr;

    if (bone_info)
    {
        int16_t mesh_bone_id = bone_info->bone_id;
        if (mesh_bone_id < m_bone_id_to_anim_bone_index.size())
        {
            anim_bone_index = m_bone_id_to_anim_bone_index[mesh_bone_id];
            if (anim_bone_index >= 0)
                anim_bone = &animation_bones[anim_bone_index];
        }
    }

    if (anim_bone)
    {
        anim_bone->update(m_current_time);
        node_transform = anim_bone->local_transform;
    }

    mat4<real> global_transform = parent_transform * node_transform;
    if (bone_info && anim_bone_index >= 0)
    {
        if (anim_bone_index < m_final_bone_matrices.size())
        {
            m_final_bone_matrices[anim_bone_index] =
                animation->get_global_inverse_transform() * global_transform *
                (mat4<real>)bone_info->offset_transform;
        }
    }

    for (const auto &child : node->children)
        calculate_bone_transform(&child, global_transform);
}
} // namespace zabato