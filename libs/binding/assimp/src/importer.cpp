#include <zabato/animation.hpp>
#include <zabato/asset_bundle.hpp>
#include <zabato/assimp/importer.hpp>
#include <zabato/hash_map.hpp>
#include <zabato/ice.hpp>
#include <zabato/mesh.hpp>
#include <zabato/model.hpp>
#include <zabato/node.hpp>
#include <zabato/object_resource.hpp>
#include <zabato/path.hpp>
#include <zabato/shared_ptr.hpp>
#include <zabato/transformation.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>

namespace zabato::assimp
{

void register_importer()
{
    importer_registry::register_importer(make_shared<assimp_importer>());
}

bool assimp_importer::supports(const string &extension) const
{
    // Common extensions supported by Assimp
    static const vector<string> supported_exts = {
        ".fbx", ".obj", ".gltf", ".glb", ".dae", ".blend"};

    for (const auto &ext : supported_exts)
    {
        if (extension == ext)
            return true;
    }
    return false;
}

bool assimp_importer::is_resource_type(const rtti &type) const
{
    return type.is_derived(asset_bundle::TYPE) || type.is_derived(mesh::TYPE) ||
           type.is_derived(animation::TYPE) ||
           type.is_derived(object_resource::TYPE);
}

static mat4<real> to_mat4(const aiMatrix4x4 &m)
{
    mat4<real> res;
    res.m[0][0] = m.a1;
    res.m[0][1] = m.b1;
    res.m[0][2] = m.c1;
    res.m[0][3] = m.d1;
    res.m[1][0] = m.a2;
    res.m[1][1] = m.b2;
    res.m[1][2] = m.c2;
    res.m[1][3] = m.d2;
    res.m[2][0] = m.a3;
    res.m[2][1] = m.b3;
    res.m[2][2] = m.c3;
    res.m[2][3] = m.d3;
    res.m[3][0] = m.a4;
    res.m[3][1] = m.b4;
    res.m[3][2] = m.c4;
    res.m[3][3] = m.d4;
    return res;
}

static pointer<node> parse_node(aiNode *aiNode,
                                const aiScene *scene,
                                pointer<node> parent,
                                shared_ptr<asset_bundle> &bundle,
                                const string &path,
                                hash_map<struct aiNode *, spatial *> &node_map)
{
    auto create_model = [](size_t i,
                           struct aiNode *aiNode,
                           const aiScene *scene,
                           shared_ptr<asset_bundle> &bundle,
                           const string &path)
    {
        pointer<model> m = new model();
        auto mesh_idx    = aiNode->mMeshes[i];

        auto am         = scene->mMeshes[mesh_idx];
        string m_name   = am->mName.length > 0 ? am->mName.C_Str()
                                               : ("mesh_" + to_string(mesh_idx));
        string sub_path = path + "@" + m_name;
        m->set_mesh(sub_path.c_str());
        m->set_name(m_name.c_str());
        return m;
    };

    pointer<node> current = new node();
    current->set_name(aiNode->mName.C_Str());
    auto numMeshes = aiNode->mNumMeshes;
    if (numMeshes != 0)
    {
        for (size_t i = 0; i < numMeshes; i++)
        {
            auto m = create_model(i, aiNode, scene, bundle, path);
            current->attach_child(m);
        }
    }

    aiVector3D scale, pos;
    aiQuaternion rot;
    aiNode->mTransformation.Decompose(scale, rot, pos);

    transformation t;
    t.set_translate(vec3<real>(pos.x, pos.y, pos.z));
    t.set_rotate(quat<real>(rot.x, rot.y, rot.z, rot.w));
    t.set_scale(vec3<real>(scale.x, scale.y, scale.z));
    current->set_local(t);

    node_map.add(aiNode, current.get());

    if (parent)
        parent->attach_child(current);

    auto numChildren = aiNode->mNumChildren;
    for (size_t i = 0; i < numChildren; ++i)
        parse_node(
            aiNode->mChildren[i], scene, current, bundle, path, node_map);
    return current;
}

static aiNode *find_lowest_common_ancestor(aiNode *n1, aiNode *n2)
{
    if (!n1 || !n2)
        return nullptr;

    auto get_depth = [](aiNode *n)
    {
        int d = 0;
        while (n)
        {
            d++;
            n = n->mParent;
        }
        return d;
    };

    int d1 = get_depth(n1);
    int d2 = get_depth(n2);

    while (d1 > d2)
    {
        n1 = n1->mParent;
        d1--;
    }
    while (d2 > d1)
    {
        n2 = n2->mParent;
        d2--;
    }

    while (n1 != n2 && n1 && n2)
    {
        n1 = n1->mParent;
        n2 = n2->mParent;
    }
    return n1;
}

static void bind_models_skeleton(const vector<model *> &models,
                                 const aiScene *scene,
                                 spatial *default_root,
                                 const hash_map<aiNode *, spatial *> &node_map)
{
    for (auto *m : models)
    {
        shared_ptr<mesh> mesh_ptr = m->get_mesh();
        spatial *skeleton_root    = default_root;

        if (mesh_ptr)
        {
            const auto &bones = mesh_ptr->get_bones();
            if (!bones.empty())
            {
                aiNode *common_ancestor = nullptr;
                for (const auto &bone : bones)
                {
                    aiNode *bone_node =
                        scene->mRootNode->FindNode(bone.name.c_str());
                    if (bone_node)
                    {
                        if (!common_ancestor)
                            common_ancestor = bone_node;
                        else
                            common_ancestor = find_lowest_common_ancestor(
                                common_ancestor, bone_node);
                    }
                }

                if (common_ancestor)
                {
                    spatial *out_val = nullptr;
                    if (node_map.try_get_value(common_ancestor, out_val))
                    {
                        skeleton_root = out_val;
                    }
                }
            }
        }
        m->bind_skeleton(skeleton_root);
    }
}

result<shared_ptr<resource>>
assimp_importer::import(class resource_manager &manager,
                        const string &path,
                        const tinyxml2::XMLElement *settings)
{
    Assimp::Importer importer;

    // Set some default properties for optimization
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,
                                aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    auto fs = manager.get_file_system();
    if (!fs)
        return report_error(error_code::value,
                            "File system not set in resource_manager");

    if (!fs->exists(path))
        return report_error(error_code::file_not_found, path.c_str());

    vector<uint8_t> buf = fs->read_all_bytes(path);
    if (buf.empty())
        return report_error(error_code::unable_to_read, path.c_str());

    unsigned int ai_flags = aiProcess_Triangulate |
                            aiProcess_JoinIdenticalVertices |
                            aiProcess_SortByPType | aiProcess_LimitBoneWeights |
                            aiProcess_ValidateDataStructure;

    bool generate_normals = true;
    bool flip_uvs         = true;

    if (settings)
    {
        for (const tinyxml2::XMLElement *param =
                 settings->FirstChildElement("param");
             param;
             param = param->NextSiblingElement("param"))
        {
            const char *name = param->Attribute("name");
            if (name)
            {
                if (string_view(name) == "generate_normals")
                    param->QueryBoolAttribute("value", &generate_normals);
                else if (string_view(name) == "flip_uvs")
                    param->QueryBoolAttribute("value", &flip_uvs);
            }
        }
    }

    if (generate_normals)
        ai_flags |= aiProcess_GenSmoothNormals;
    if (flip_uvs)
        ai_flags |= aiProcess_FlipUVs;

    auto dot         = path.rfind('.');
    string extension = "";
    if (dot != string::npos)
        extension = path.substr(dot);
    const aiScene *scene = importer.ReadFileFromMemory(
        buf.data(),
        buf.size(),
        ai_flags,
        extension.empty() ? nullptr : extension.c_str());

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode)
    {
        return report_error(error_code::unable_to_read,
                            importer.GetErrorString());
    }

    if (scene->mNumMeshes == 0 && scene->mNumAnimations == 0)
    {
        return report_error(error_code::value,
                            "No meshes or animations found in file");
    }

    auto bundle    = make_shared<asset_bundle>();
    auto numMeshes = scene->mNumMeshes;
    for (size_t m_idx = 0; m_idx < numMeshes; ++m_idx)
    {
        auto am = scene->mMeshes[m_idx];
        auto m  = make_shared<mesh>();

        mesh_flags flags = mesh_flags::none;
        if (am->HasNormals())
            flags = flags | mesh_flags::normal;
        if (am->HasTextureCoords(0))
            flags = flags | mesh_flags::tex;
        if (am->HasVertexColors(0))
            flags = flags | mesh_flags::color;
        if (am->HasBones())
            flags = flags | mesh_flags::bone;

        m->init(flags, primitive_type::triangles);
        m->set_vertex_count(am->mNumVertices);
        m->set_primitive_count(am->mNumFaces);

        size_t numVertices = am->mNumVertices;
        for (size_t i = 0; i < numVertices; i++)
        {
            m->set_position(i,
                            vec3<real>(am->mVertices[i].x,
                                       am->mVertices[i].y,
                                       am->mVertices[i].z));

            if (am->HasNormals())
            {
                m->set_normal(i,
                              vec3<real>(am->mNormals[i].x,
                                         am->mNormals[i].y,
                                         am->mNormals[i].z));
            }

            if (am->HasTextureCoords(0))
            {
                m->set_texcoord(i,
                                vec2<real>(am->mTextureCoords[0][i].x,
                                           am->mTextureCoords[0][i].y));
            }

            if (am->HasVertexColors(0))
            {
                m->set_color(i,
                             color(am->mColors[0][i].r,
                                   am->mColors[0][i].g,
                                   am->mColors[0][i].b,
                                   am->mColors[0][i].a));
            }
        }

        // Copy indices
        for (unsigned int i = 0; i < am->mNumFaces; i++)
        {
            auto face = am->mFaces[i];
            if (face.mNumIndices != 3)
                continue;

            triangle_primitive prim;
            prim.v0 = face.mIndices[0];
            prim.v1 = face.mIndices[1];
            prim.v2 = face.mIndices[2];
            m->set_primitive(i, prim);
        }

        // Copy bones
        if (am->HasBones())
        {
            auto numBones = am->mNumBones;
            vector<vector<bone_weight>> vertex_weights(numVertices);

            m->set_bone_count(numBones);
            for (size_t b_idx = 0; b_idx < numBones; b_idx++)
            {
                auto ab = am->mBones[b_idx];

                bone_info b;
                b.name             = ab->mName.C_Str();
                b.offset_transform = ICE_MAT4X4_R16(to_mat4(ab->mOffsetMatrix));
                m->set_bone(b_idx, b);

                auto numWeights = ab->mNumWeights;
                for (size_t w_idx = 0; w_idx < numWeights; w_idx++)
                {
                    auto aw = ab->mWeights[w_idx];
                    if (aw.mVertexId < numVertices)
                    {
                        bone_weight bw;
                        bw.bone_id = b_idx;
                        bw.weight  = ICE_R16(aw.mWeight);
                        vertex_weights[aw.mVertexId].push_back(bw);
                    }
                }
            }

            for (size_t v_idx = 0; v_idx < numVertices; v_idx++)
            {
                bone_weight final_weights[4];
                auto &weights      = vertex_weights[v_idx];
                size_t num_weights = weights.size();

                for (size_t w_idx = 0; w_idx < 4; w_idx++)
                {
                    if (w_idx < num_weights)
                        final_weights[w_idx] = weights[w_idx];
                    else
                        final_weights[w_idx] = {0, ICE_R16(0.0)};
                }
                m->set_boneweight(v_idx, final_weights);
            }
        }

        string mesh_name = am->mName.length > 0 ? am->mName.C_Str()
                                                : ("mesh_" + to_string(m_idx));
        bundle->add_resource(mesh_name, shared_ptr<resource>(m));
    }

    // Animations
    auto numAnimations = scene->mNumAnimations;
    for (size_t a_idx = 0; a_idx < numAnimations; ++a_idx)
    {
        auto aa = scene->mAnimations[a_idx];
        auto a  = make_shared<animation>();

        a->set_duration((real)aa->mDuration);
        a->set_ticks_per_second(
            aa->mTicksPerSecond != 0.0 ? (real)aa->mTicksPerSecond : real(25));

        auto numChannels = aa->mNumChannels;
        vector<animation_track> tracks(numChannels);
        for (size_t c_idx = 0; c_idx < numChannels; ++c_idx)
        {
            auto ac     = aa->mChannels[c_idx];
            auto &t     = tracks[c_idx];
            t.bone_name = ac->mNodeName.C_Str();

            if (settings)
            {
                string opt_name = string("map_bone_") + t.bone_name.c_str();
                for (const tinyxml2::XMLElement *p =
                         settings->FirstChildElement("param");
                     p;
                     p = p->NextSiblingElement("param"))
                {
                    const char *n = p->Attribute("name");
                    if (n && opt_name == string(n))
                    {
                        const char *v = p->Attribute("value");
                        if (v && strlen(v) > 0)
                            t.bone_name = v;
                        break;
                    }
                }
            }

            t.pre_state  = (anim_behaviour)ac->mPreState;
            t.post_state = (anim_behaviour)ac->mPostState;

            auto numPositionKeys = ac->mNumPositionKeys;
            t.positions.resize(numPositionKeys);
            for (size_t k_idx = 0; k_idx < numPositionKeys; ++k_idx)
            {
                auto &p  = t.positions[k_idx];
                auto &ap = ac->mPositionKeys[k_idx];

                p.timestamp = ICE_R16((real)ap.mTime);
                p.position  = ICE_VEC3_R16(
                    (real)ap.mValue.x, (real)ap.mValue.y, (real)ap.mValue.z);
            }

            auto numRotationKeys = ac->mNumRotationKeys;
            t.rotations.resize(numRotationKeys);
            for (size_t k_idx = 0; k_idx < numRotationKeys; ++k_idx)
            {
                auto &r  = t.rotations[k_idx];
                auto &ar = ac->mRotationKeys[k_idx];

                r.timestamp = ICE_R16((real)ar.mTime);
                r.rotation  = ICE_QUAT_R16({(real)ar.mValue.x,
                                            (real)ar.mValue.y,
                                            (real)ar.mValue.z,
                                            (real)ar.mValue.w});
            }

            auto numScaleKeys = ac->mNumScalingKeys;
            t.scales.resize(numScaleKeys);
            for (size_t k_idx = 0; k_idx < numScaleKeys; ++k_idx)
            {
                auto &s  = t.scales[k_idx];
                auto &as = ac->mScalingKeys[k_idx];

                s.timestamp = ICE_R16((real)as.mTime);
                s.scale     = ICE_VEC3_R16(
                    (real)as.mValue.x, (real)as.mValue.y, (real)as.mValue.z);
            }
        }

        a->set_tracks(tracks);

        string anim_name = aa->mName.length > 0 ? aa->mName.C_Str()
                                                : ("anim_" + to_string(a_idx));

        bundle->add_resource(anim_name, a);
    }

    pointer<node> root_node = nullptr;
    if (scene->mRootNode)
    {
        hash_map<aiNode *, spatial *> node_map;
        root_node = parse_node(
            scene->mRootNode, scene, nullptr, bundle, path, node_map);
        auto root_name = scene->mRootNode->mName.length > 0
                             ? scene->mRootNode->mName.C_Str()
                             : fs::filename(path);

        root_node->set_name(root_name);

        // Bind skeletons for all models in the hierarchy
        vector<model *> models;
        auto find_models =
            [](spatial *s, vector<model *> &out_models, auto &self) -> void
        {
            if (auto *m = c_dynamic_cast<model>(s))
                out_models.push_back(m);
            if (auto *n = c_dynamic_cast<node>(s))
            {
                for (size_t i = 0; i < n->quantity(); ++i)
                    self(n->child_at(i).get(), out_models, self);
            }
        };
        find_models(root_node.get(), models, find_models);

        bind_models_skeleton(models, scene, root_node.get(), node_map);

        auto res = make_shared<object_resource>();
        res->set_object(root_node.get());
        bundle->add_resource(root_name, res);
    }

    return result<shared_ptr<resource>>(bundle);
}

vector<importer_option>
assimp_importer::get_options(class resource_manager &manager,
                             const string &path,
                             const tinyxml2::XMLElement *settings) const
{
    vector<importer_option> opts;

    bool gen_normals = true;
    bool flip_uvs    = true;

    if (settings)
    {
        for (const tinyxml2::XMLElement *param =
                 settings->FirstChildElement("param");
             param;
             param = param->NextSiblingElement("param"))
        {
            const char *name = param->Attribute("name");
            if (name)
            {
                if (string_view(name) == "generate_normals")
                    param->QueryBoolAttribute("value", &gen_normals);
                else if (string_view(name) == "flip_uvs")
                    param->QueryBoolAttribute("value", &flip_uvs);
            }
        }
    }

    {
        importer_option opt;
        opt.name          = "generate_normals";
        opt.display_name  = "Generate Normals";
        opt.description   = "Generate smooth normals if missing.";
        opt.type_default  = value(true);
        opt.current_value = value(gen_normals);
        opt.group         = "General";
        opts.push_back(opt);
    }
    {
        importer_option opt;
        opt.name          = "flip_uvs";
        opt.display_name  = "Flip UVs";
        opt.description   = "Flip UV coordinates on Y axis.";
        opt.type_default  = value(true);
        opt.current_value = value(flip_uvs);
        opt.group         = "General";
        opts.push_back(opt);
    }

    if (!path.empty())
    {
        auto fs = manager.get_file_system();
        if (fs && fs->exists(path))
        {
            vector<uint8_t> buf = fs->read_all_bytes(path);
            if (!buf.empty())
            {
                ::Assimp::Importer file_importer;
                unsigned int flags = aiProcess_JoinIdenticalVertices |
                                     aiProcess_ValidateDataStructure;

                file_importer.SetPropertyBool(
                    AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);
                file_importer.SetPropertyInteger(
                    AI_CONFIG_PP_RVC_FLAGS,
                    aiComponent_CAMERAS | aiComponent_LIGHTS |
                        aiComponent_MATERIALS | aiComponent_TEXTURES);

                auto dot         = path.rfind('.');
                string extension = "";
                if (dot != string::npos)
                    extension = path.substr(dot);

                const aiScene *scene = file_importer.ReadFileFromMemory(
                    buf.data(),
                    buf.size(),
                    flags,
                    extension.empty() ? nullptr : extension.c_str());

                if (scene)
                {
                    vector<string> all_bones;

                    for (unsigned int a = 0; a < scene->mNumAnimations; ++a)
                    {
                        aiAnimation *anim = scene->mAnimations[a];
                        for (unsigned int c = 0; c < anim->mNumChannels; ++c)
                        {
                            string bName =
                                anim->mChannels[c]->mNodeName.C_Str();
                            bool found = false;
                            for (const auto &b : all_bones)
                            {
                                if (b == bName)
                                {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                                all_bones.push_back(bName);
                        }
                    }

                    for (const auto &bone : all_bones)
                    {
                        string opt_name   = "map_bone_" + bone;
                        value current_val = value("");

                        if (settings)
                        {
                            for (const tinyxml2::XMLElement *p =
                                     settings->FirstChildElement("param");
                                 p;
                                 p = p->NextSiblingElement("param"))
                            {
                                const char *n = p->Attribute("name");
                                if (n && opt_name == string(n))
                                {
                                    const char *v = p->Attribute("value");
                                    if (v)
                                        current_val = value(v);
                                    break;
                                }
                            }
                        }

                        importer_option opt;
                        opt.name        = opt_name;
                        opt.description = string("Map animation bone '") +
                                          bone + "' to specific node name";
                        opt.type_default  = value("");
                        opt.current_value = current_val;
                        opt.group         = "Bone Mappings";
                        opt.display_name  = bone;
                        opts.push_back(opt);
                    }
                }
            }
        }
    }

    return opts;
}

} // namespace zabato::assimp
