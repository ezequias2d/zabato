#include "zabato/shared_ptr.hpp"
#include <zabato/assimp/importer.hpp>
#include <zabato/mesh.hpp>

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

bool assimp_importer::is_resource_type(const zabato::rtti &type) const
{
    return type.is_exactly(zabato::mesh::TYPE);
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
    bool flip_uvs         = true; // Default to true often

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

    string extension     = path.substr(path.rfind('.'));
    const aiScene *scene = importer.ReadFileFromMemory(
        buf.data(), buf.size(), ai_flags, extension.c_str());

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode)
    {
        return report_error(error_code::unable_to_read,
                            importer.GetErrorString());
    }

    if (scene->mNumMeshes == 0)
    {
        return report_error(error_code::value, "No meshes found in file");
    }

    // Process the first mesh
    aiMesh *amesh             = scene->mMeshes[0];
    shared_ptr<mesh> res_mesh = make_shared<mesh>();

    mesh_flags flags = mesh_flags::none;
    if (amesh->HasNormals())
        flags = flags | mesh_flags::normal;
    if (amesh->HasTextureCoords(0))
        flags = flags | mesh_flags::tex;
    if (amesh->HasVertexColors(0))
        flags = flags | mesh_flags::color;
    if (amesh->HasBones())
        flags = flags | mesh_flags::bone;

    res_mesh->init(flags, primitive_type::triangles);
    res_mesh->set_vertex_count(amesh->mNumVertices);
    res_mesh->set_primitive_count(amesh->mNumFaces);

    // Copy vertices
    for (unsigned int i = 0; i < amesh->mNumVertices; i++)
    {
        res_mesh->set_position(i,
                               vec3<real>(amesh->mVertices[i].x,
                                          amesh->mVertices[i].y,
                                          amesh->mVertices[i].z));

        if (amesh->HasNormals())
        {
            res_mesh->set_normal(i,
                                 vec3<real>(amesh->mNormals[i].x,
                                            amesh->mNormals[i].y,
                                            amesh->mNormals[i].z));
        }

        if (amesh->HasTextureCoords(0))
        {
            // Assimp has 3D texture coords, we take first 2
            res_mesh->set_texcoord(i,
                                   vec2<real>(amesh->mTextureCoords[0][i].x,
                                              amesh->mTextureCoords[0][i].y));
        }

        if (amesh->HasVertexColors(0))
        {
            res_mesh->set_color(i,
                                color(amesh->mColors[0][i].r,
                                      amesh->mColors[0][i].g,
                                      amesh->mColors[0][i].b,
                                      amesh->mColors[0][i].a));
        }
    }

    // Copy indices
    for (unsigned int i = 0; i < amesh->mNumFaces; i++)
    {
        aiFace face = amesh->mFaces[i];
        if (face.mNumIndices != 3)
            continue; // Should be triangulated already

        triangle_primitive prim;
        prim.v0 = face.mIndices[0];
        prim.v1 = face.mIndices[1];
        prim.v2 = face.mIndices[2];
        res_mesh->set_primitive(i, prim);
    }

    // Process Bones if present
    if (amesh->HasBones())
    {
        // Vectors to store bone weights per vertex
        vector<vector<bone_weight>> vertex_weights(amesh->mNumVertices);

        // Assimp stores weights per Bone, referencing vertices.
        // We need weights per Vertex, referencing bones.

        res_mesh->set_bone_count(amesh->mNumBones);

        for (unsigned int bone_idx = 0; bone_idx < amesh->mNumBones; bone_idx++)
        {
            aiBone *bone = amesh->mBones[bone_idx];

            bone_info b_info;
            b_info.name    = bone->mName.C_Str();
            b_info.bone_id = bone_idx;
            b_info.offset_transform =
                ICE_MAT4X4_R16(to_mat4(bone->mOffsetMatrix));

            res_mesh->set_bone(bone_idx, b_info);

            for (unsigned int w_idx = 0; w_idx < bone->mNumWeights; w_idx++)
            {
                unsigned int vertex_id = bone->mWeights[w_idx].mVertexId;
                float weight           = bone->mWeights[w_idx].mWeight;

                if (vertex_id < amesh->mNumVertices)
                {
                    bone_weight bw;
                    bw.bone_id = bone_idx;
                    bw.weight  = ICE_R16(weight);
                    vertex_weights[vertex_id].push_back(bw);
                }
            }
        }

        // Now flatten vertex weights to the fixed array of 4
        assert(amesh->mNumVertices <= 4);
        for (unsigned int i = 0; i < amesh->mNumVertices && i < 4; i++)
        {
            bone_weight final_weights[4];
            auto &weights = vertex_weights[i];

            for (int k = 0; k < 4; k++)
            {
                if (k < weights.size())
                {
                    final_weights[k] = weights[k];
                }
                else
                {
                    final_weights[k].bone_id = -1;
                    final_weights[k].weight  = ICE_R16(0.0);
                }
            }
            res_mesh->set_boneweight(i, final_weights);
        }
    }

    return result<shared_ptr<resource>>(move(res_mesh));
}

vector<importer_option>
assimp_importer::get_options(const tinyxml2::XMLElement *settings) const
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
        opt.description   = "Generate smooth normals if missing.";
        opt.type_default  = value(true);
        opt.current_value = value(gen_normals);
        opts.push_back(opt);
    }
    {
        importer_option opt;
        opt.name          = "flip_uvs";
        opt.description   = "Flip UV coordinates on Y axis.";
        opt.type_default  = value(true);
        opt.current_value = value(flip_uvs);
        opts.push_back(opt);
    }

    return opts;
}

} // namespace zabato::assimp
