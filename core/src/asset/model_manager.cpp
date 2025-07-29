#include "asset/model_manager.h"

#include <string>
#include <cassert>
#include <algorithm>
#include <set>

#include <stb_image.h>

#include "asset/texture_manager.h"
#include "asset/material_manager.h"

#include "core/opengl.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

struct Rigged_Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::uvec4 bone_ids; // index into global bone array, todo maybe more than 4 bones
    glm::vec4 bone_weights;
};

struct Bone { // cpu bone
    std::string name;
    uint32_t parent_bone; // idx into bone array of parent
    glm::mat4 inverse_bind;
};

//struct GPU_Bone { // combine this bone with animation data to get skinned bone
//    uint32_t parent_bone;
//    glm::mat4 inverse_bind;
//};

struct GPU_Bone_Skinned { // all a bone needs is a transform once it has been skinned
    glm::mat4 transform;  // this will be gpu ssbo that vertices index into
};

struct Temp_Bone { // struct that will be allocated to map vertices to bones, then nuked
    uint32_t bone_index; // index into global bones
    float bone_weight;
};

struct Animation {
    std::string name;
    std::vector<Bone_Animation> bone_animations;
    float duration;
    bool loop;
};

// todo skeleton

namespace Model_Manager {

    static std::string base_path;

    glm::mat4 assimp_to_glm(const aiMatrix4x4& ai_mat) {
        return glm::mat4(
            ai_mat.a1, ai_mat.b1, ai_mat.c1, ai_mat.d1,
            ai_mat.a2, ai_mat.b2, ai_mat.c2, ai_mat.d2,
            ai_mat.a3, ai_mat.b3, ai_mat.c3, ai_mat.d3,
            ai_mat.a4, ai_mat.b4, ai_mat.c4, ai_mat.d4
        );
    }

    void init(std::string path) {
        base_path = path;
        model_handle mh = load_model_indirect("teapot.obj");
    }

    void cleanup() {
        // todo Lol
    }

    // indirect stuff
    // todo change to some kind of block manager thing so i cna get rid of cpu verticies when uploaded but still track them
    static uint32_t num_meshes = 0;
    static uint32_t big_buffer_vao, vbo, ebo;

    static std::vector<Vertex> g_vertices(0);
    static std::vector<uint32_t> g_indices(0);
    static std::vector<Model_Indirect> m_indirect_models(0);
    static std::vector<std::string> m_indirect_model_names(0);

    static uint32_t num_rigged_meshes = 0;
    static uint32_t rigged_vao, r_vbo, r_ebo;
    static std::vector<Rigged_Vertex> g_rigged_vertices(0);
    static std::vector<uint32_t> g_rigged_indices(0);
    static std::vector<Bone> g_bones(0);
    static std::vector<GPU_Bone_Skinned> g_skinned_bones(0); // todo compute
    static std::vector<Model_Indirect> m_rigged_models(0);
    static std::vector<std::string> m_rigged_model_names(0);

    static std::vector<Animation> g_animations(0);
    static std::vector<std::string> g_animation_names(0);

    //uint32_t get_num_meshes() { return num_meshes;  }

    bool indirect_model_loaded(const std::string& full_path, model_handle& model_index) {
        for (size_t i = 0; i < m_indirect_model_names.size(); i++) {
            if (full_path == m_indirect_model_names[i]) {
                model_index = i;
                return true;
            }
        }
        return false;
    }

    model_handle load_rigged_model(const std::string& path) {
        return load_model_indirect(path, true);
    }

    model_handle load_model_indirect(const std::string& path, bool rigged) {
        if (rigged) {
            printf("num rigged verts before model %d\n", g_rigged_vertices.size());
            printf("num rigged idx before model %d\n", g_rigged_indices.size());
            printf("num bones before model %d\n", g_bones.size());
        }
        else {
            printf("num verts before model %d\n", g_vertices.size());
            printf("num idx before model %d\n", g_indices.size());

        }

        const std::string full_path = base_path + path;

        model_handle model_index;
        if (indirect_model_loaded(full_path, model_index))
            return model_index;
        
        Model_Indirect model_ind;
        model_ind.m_name = path;

        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(full_path, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            printf("ERROR::ASSIMP::%s\n", import.GetErrorString());
            return false;
        }

        const std::string path_without_filename = full_path.substr(0, full_path.find_last_of("/") + 1);
        uint32_t base_bone = g_bones.size();
        process_node(scene->mRootNode, scene, model_ind, path_without_filename, glm::mat4(1.0f), rigged, base_bone);

        model_ind.calculate_aabb();


        if (rigged && scene->mNumAnimations > 0) {
            update_bone_parents(scene, base_bone, g_bones.size());

            printf("LOADING ANIMATIONS\n");
            load_animations_from_scene(scene, base_bone);
            for (Animation a : g_animations)
                printf("%s, %d\n", a.name.c_str(), a.duration);
        }
        
        if (rigged) {
            model_index = m_rigged_models.size();

            m_rigged_models.push_back(model_ind);
            m_rigged_model_names.push_back(full_path);

            printf("num rigged verts after model %d\n", g_rigged_vertices.size());
            printf("num rigged idx after model %d\n", g_rigged_indices.size());
            printf("num bones after model %d\n", g_bones.size());
            printf("num animations loaded %d\n", g_animations.size());
        }
        else {
            model_index = m_indirect_models.size();

            m_indirect_models.push_back(model_ind);
            m_indirect_model_names.push_back(full_path);

            printf("num verts after model %d\n", g_vertices.size());
            printf("num idx after model %d\n", g_indices.size());
        }

        return model_index;
    }

    void process_node(aiNode* node, const aiScene* scene, Model_Indirect& model_ind, const std::string& path, const glm::mat4& parent_transform, bool rigged, uint32_t base_bone) {
        glm::mat4 current_transform = parent_transform * assimp_to_glm(node->mTransformation);

        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            Mesh_Indirect mesh_ind = rigged ? process_rigged_mesh(mesh, scene, path, base_bone) : process_mesh(mesh, scene, path);

            mesh_ind.transform = current_transform;
            mesh_ind.aabb.max = glm::vec3(current_transform * glm::vec4(mesh_ind.aabb.max, 1.0f));
            mesh_ind.aabb.min = glm::vec3(current_transform * glm::vec4(mesh_ind.aabb.min, 1.0f));

            model_ind.add_mesh(mesh_ind);
            if (rigged)
                num_rigged_meshes++;
            else
                num_meshes++;
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            process_node(node->mChildren[i], scene, model_ind, path, current_transform, rigged, base_bone);
        }
    }

    Mesh_Indirect process_mesh(const aiMesh* mesh, const aiScene* scene, const std::string& path) {
        Mesh_Indirect mesh_ind = { 0 };
        mesh_ind.name = std::string(mesh->mName.C_Str());
        printf("loading mesh %s\n", mesh_ind.name.c_str());

        mesh_ind.aabb.min = glm::vec3(FLT_MAX);
        mesh_ind.aabb.max = glm::vec3(-FLT_MAX);

        mesh_ind.base_vertex = g_vertices.size();// 
        uint32_t vertex_count = mesh->mNumVertices;

        g_vertices.reserve(mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            
            vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            mesh_ind.aabb.min = glm::min(vertex.position, mesh_ind.aabb.min);
            mesh_ind.aabb.max = glm::max(vertex.position, mesh_ind.aabb.max);

            if (mesh->HasNormals())
                vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            else
                vertex.normal = glm::vec3(0.0f);

            if (mesh->mTextureCoords[0]) {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.tex_coords = vec;

                const aiVector3D& pTangent = mesh->mTangents[i];
                vertex.tangent = glm::vec3(pTangent.x, pTangent.y, pTangent.z);

                const aiVector3D& pBitangent = mesh->mBitangents[i];
                vertex.bitangent = glm::vec3(pBitangent.x, pBitangent.y, pBitangent.z);
            }
            else {
                vertex.tex_coords = glm::vec2(0.0f, 0.0f);
                vertex.tangent = glm::vec3(0.0f);
                vertex.bitangent = glm::vec3(0.0f);
            }

            g_vertices.push_back(vertex);
        }

        // process indices
        mesh_ind.base_index = g_indices.size();
        
        uint32_t num_idcs = 0;
        for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
                g_indices.push_back(face.mIndices[j]); // todo probably just reserver size and copy big chunk

            num_idcs += face.mNumIndices;
        }
        mesh_ind.index_count = num_idcs;

        // process material
        Material_Indirect material = load_material(mesh, scene, path);
        mesh_ind.material_index = Material_Manager::add_material(material);

        return mesh_ind;
    }

    Mesh_Indirect process_rigged_mesh(const aiMesh* mesh, const aiScene* scene, const std::string& path, uint32_t base_bone) {
        Mesh_Indirect mesh_ind = { 0 };
        mesh_ind.name = std::string(mesh->mName.C_Str());
        printf("loading RIGGED mesh %s\n", mesh_ind.name.c_str());
        //mesh_ind.rigged = true;

        mesh_ind.aabb.min = glm::vec3(FLT_MAX);
        mesh_ind.aabb.max = glm::vec3(-FLT_MAX);

        uint32_t num_vertices = mesh->mNumVertices;

        // process bones
        // create mapping from verticies to bones
        // [v0 -> [b1, b2, b3, ...], v1 -> [b1, b2], v2 -> [b1]]
        std::vector<std::vector<Temp_Bone>> vertex_bone_influences(num_vertices);

        for (uint32_t bone_idx = 0; bone_idx < mesh->mNumBones; bone_idx++) {
            const aiBone* bone = mesh->mBones[bone_idx];

            // add to g_bones if doesnt exist, or return idx to it
            uint32_t global_bone_idx = find_or_create_global_bone(bone, scene, base_bone);

            for (uint32_t weight_idx = 0; weight_idx < bone->mNumWeights; weight_idx++) {
                uint32_t vertex_id = bone->mWeights[weight_idx].mVertexId;
                float weight = bone->mWeights[weight_idx].mWeight;

                //if (weight > 0.0f) {
                    vertex_bone_influences[vertex_id].push_back({ global_bone_idx, weight });
                //}
            }
        }

        mesh_ind.base_vertex = g_rigged_vertices.size();

        g_rigged_vertices.reserve(num_vertices);
        for (uint32_t i = 0; i < num_vertices; i++) {
            Rigged_Vertex vertex;

            vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            mesh_ind.aabb.min = glm::min(vertex.position, mesh_ind.aabb.min);
            mesh_ind.aabb.max = glm::max(vertex.position, mesh_ind.aabb.max);

            if (mesh->HasNormals())
                vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            else
                vertex.normal = glm::vec3(0.0f);

            if (mesh->mTextureCoords[0]) {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.tex_coords = vec;

                const aiVector3D& pTangent = mesh->mTangents[i];
                vertex.tangent = glm::vec3(pTangent.x, pTangent.y, pTangent.z);

                const aiVector3D& pBitangent = mesh->mBitangents[i];
                vertex.bitangent = glm::vec3(pBitangent.x, pBitangent.y, pBitangent.z);
            }
            else {
                vertex.tex_coords = glm::vec2(0.0f, 0.0f);
                vertex.tangent = glm::vec3(0.0f);
                vertex.bitangent = glm::vec3(0.0f);
            }

            // process bones
            std::vector<Temp_Bone>& vertex_bones = vertex_bone_influences[i];
            if (vertex_bones.size() > 4) {
                // maybe do something
                printf("\n\n-----SUS MORE THAN 4 BONES [%d]-----\n\n", vertex_bones.size());
            }

            uint32_t num_bones = vertex_bones.size() > 4 ? 4 : vertex_bones.size();
            std::sort(vertex_bones.begin(), vertex_bones.end(), [](Temp_Bone f1, Temp_Bone f2){ return f1.bone_weight > f2.bone_weight; });
            for (uint32_t i = 0; i < num_bones; i++) {
                vertex.bone_ids[i] = vertex_bones[i].bone_index;
                vertex.bone_weights[i] = vertex_bones[i].bone_weight;
            }
            // todo maybe normalize weights?

            g_rigged_vertices.push_back(vertex);
        }

        // process indices
        mesh_ind.base_index = g_rigged_indices.size();

        uint32_t num_idcs = 0;
        for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
                g_rigged_indices.push_back(face.mIndices[j]); // todo probably just reserver size and copy big chunk

            num_idcs += face.mNumIndices;
        }
        mesh_ind.index_count = num_idcs;

        // process material
        Material_Indirect material = load_material(mesh, scene, path);
        mesh_ind.material_index = Material_Manager::add_material(material);

        return mesh_ind;
    }

    Material_Indirect load_material(const aiMesh* mesh, const aiScene* scene, const std::string& path) {
        Material_Indirect mesh_mat{ 0 };

        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            //#define AI_MATKEY_BASE_COLOR "$clr.base", 0, 0

            if (material->GetTextureCount(aiTextureType_BASE_COLOR)) {
                aiString str;
                material->GetTexture(aiTextureType_BASE_COLOR, 0, &str);
                //albedo = Texture_Manager::load_from_path(path + str.C_Str());
                mesh_mat.albedo = Texture_Manager::load_bindless_from_path(path + str.C_Str());
            }

            if (material->GetTextureCount(aiTextureType_NORMALS)) {
                aiString str;
                material->GetTexture(aiTextureType_NORMALS, 0, &str);
                mesh_mat.normal = Texture_Manager::load_bindless_from_path(path + str.C_Str());
            }

            // todo why metallic 1?
            float metallic = 0.0f;
            float roughness = 1.0f;
            if (material->GetTextureCount(aiTextureType_GLTF_METALLIC_ROUGHNESS)) {
                aiString str;
                material->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &str);
                // printf("MET ROUGHESNSENSENESNESNE %s\n", str.C_Str());
                mesh_mat.met_rough = Texture_Manager::load_bindless_from_path(path + str.C_Str());

                aiReturn metallicResult = material->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
                aiReturn roughnessResult = material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
            }
            mesh_mat.metallic_factor = metallic;
            mesh_mat.roughness_factor = roughness;

            printf("MET: %f, ROG: %f\n", metallic, roughness);

            // aiTextureType_EMISSIVE
            if (material->GetTextureCount(aiTextureType_EMISSIVE)) {
                aiString str;
                material->GetTexture(aiTextureType_EMISSIVE, 0, &str);
                mesh_mat.emissive = Texture_Manager::load_bindless_from_path(path + str.C_Str());
            }

            aiColor3D emissiveColor;
            if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS) {
                float strength = 1.0f;
                aiReturn intensityResult = material->Get(AI_MATKEY_EMISSIVE_INTENSITY, strength);

                mesh_mat.emissive_factor = glm::vec4(emissiveColor.r, emissiveColor.g, emissiveColor.b, strength);
                //printf("Emissive color: R=%.3f G=%.3f B=%.3f, w=%f\n", emissiveColor.r, emissiveColor.g, emissiveColor.b, strength);
            }
            else {
                mesh_mat.emissive_factor = glm::vec4(1.0f);
            }

            aiColor4D pbrBaseColor(0.0f, 0.0f, 0.0f, 0.0f);
            if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &pbrBaseColor) == AI_SUCCESS) {
                // Use PBR base color
                mesh_mat.base_color = glm::vec4(pbrBaseColor.r, pbrBaseColor.g, pbrBaseColor.b, pbrBaseColor.a);
            }
            else {
                mesh_mat.base_color = glm::vec4(1.0f);
            }

            //  AO map
            if (material->GetTextureCount(aiTextureType_LIGHTMAP)) {
                aiString str;
                material->GetTexture(aiTextureType_LIGHTMAP, 0, &str);
                mesh_mat.amb_occ = Texture_Manager::load_bindless_from_path(path + str.C_Str());
            }
        }

        return mesh_mat;
    }

    uint32_t find_or_create_global_bone(const aiBone* bone, const aiScene* scene, uint32_t base_bone) {
        std::string bone_name(bone->mName.C_Str());
        
        // base bone is g_bones.size() before loading a model
        for (uint32_t i = base_bone; i < g_bones.size(); i++) {
            if (g_bones[i].name == bone_name)
                return i;
        }

        printf("[BONE] adding bone %s\n", bone_name.c_str());

        Bone new_bone;
        new_bone.name = bone_name;
        new_bone.inverse_bind = assimp_to_glm(bone->mOffsetMatrix);
        new_bone.parent_bone = UINT32_MAX;

        g_bones.push_back(new_bone);
        return g_bones.size() - 1;
    }

    void load_animations_from_scene(const aiScene* scene, uint32_t base_bone) {
        for (uint32_t anim_idx = 0; anim_idx < scene->mNumAnimations; ++anim_idx) {
            aiAnimation* ai_anim = scene->mAnimations[anim_idx];

            Animation animation;
            animation.name = ai_anim->mName.C_Str();
            animation.duration = static_cast<float>(ai_anim->mDuration / ai_anim->mTicksPerSecond);

            printf("Loading animation: %s, duration: %.2f seconds\n", animation.name.c_str(), animation.duration);

            for (uint32_t channel_idx = 0; channel_idx < ai_anim->mNumChannels; ++channel_idx) {
                aiNodeAnim* ai_channel = ai_anim->mChannels[channel_idx];

                uint32_t bone_index = find_bone_index(ai_channel->mNodeName.C_Str(), base_bone);

                Bone_Animation bone_anim;
                bone_anim.bone_index = bone_index;

                load_keyframes_from_channel(ai_channel, bone_anim, ai_anim->mTicksPerSecond);

                animation.bone_animations.push_back(bone_anim);
            }

            g_animations.push_back(animation);
            g_animation_names.push_back(animation.name);
        }
    }

    void load_keyframes_from_channel(aiNodeAnim* channel, Bone_Animation& bone_anim, double ticks_per_second) {
        for (uint32_t i = 0; i < channel->mNumPositionKeys; ++i) {
            Position_Keyframe keyframe;
            aiVector3D pos = channel->mPositionKeys[i].mValue;
            keyframe.position = glm::vec3(pos.x, pos.y, pos.z);
            keyframe.time = static_cast<float>(channel->mPositionKeys[i].mTime / ticks_per_second);
            bone_anim.position_keyframes.push_back(keyframe);
        }

        for (uint32_t i = 0; i < channel->mNumRotationKeys; ++i) {
            Rotation_Keyframe keyframe;
            aiQuaternion rot = channel->mRotationKeys[i].mValue;
            keyframe.rotation = glm::quat(rot.w, rot.x, rot.y, rot.z);
            keyframe.time = static_cast<float>(channel->mRotationKeys[i].mTime / ticks_per_second);
            bone_anim.rotation_keyframes.push_back(keyframe);
        }

        for (uint32_t i = 0; i < channel->mNumScalingKeys; ++i) {
            Scale_Keyframe keyframe;
            aiVector3D scale = channel->mScalingKeys[i].mValue;
            keyframe.scale = glm::vec3(scale.x, scale.y, scale.z);
            keyframe.time = static_cast<float>(channel->mScalingKeys[i].mTime / ticks_per_second);
            bone_anim.scale_keyframes.push_back(keyframe);
        }
    }

    uint32_t find_bone_index(const std::string& bone_name, uint32_t base_bone) {
        for (uint32_t i = base_bone; i < g_bones.size(); ++i) {
            if (g_bones[i].name == bone_name) {
                return i;
            }
        }
        printf("bone %s not found\n", bone_name.c_str());
        assert(false);
    }

    Model_Indirect get_model_ind(uint32_t idx) {
        return m_indirect_models[idx];
    }
    
    Model_Indirect get_skinned_model(uint32_t idx) {
        return m_rigged_models[idx];
    }

    Util::AABB get_aabb_indirect(const model_handle& model_id) {
        return m_indirect_models[model_id].get_aabb();
    }

    void setup_buffers() {
        glGenVertexArrays(1, &big_buffer_vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(big_buffer_vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, g_vertices.size() * sizeof(Vertex), &g_vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, g_indices.size() * sizeof(uint32_t), &g_indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

        glBindVertexArray(0);

        printf("Uploaded %zu vertices, %zu indices\n", g_vertices.size(), g_indices.size());

        // todo do same for animated stuff
        // vertex data, + ssbo for all bone data prob
        glGenVertexArrays(1, &rigged_vao);
        glGenBuffers(1, &r_vbo);
        glGenBuffers(1, &r_ebo);

        glBindVertexArray(rigged_vao);
        glBindBuffer(GL_ARRAY_BUFFER, r_vbo);

        glBufferData(GL_ARRAY_BUFFER, g_rigged_vertices.size() * sizeof(Rigged_Vertex), &g_rigged_vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, g_rigged_indices.size() * sizeof(uint32_t), &g_rigged_indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Rigged_Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Rigged_Vertex), (void*)offsetof(Rigged_Vertex, normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Rigged_Vertex), (void*)offsetof(Rigged_Vertex, tex_coords));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Rigged_Vertex), (void*)offsetof(Rigged_Vertex, tangent));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Rigged_Vertex), (void*)offsetof(Rigged_Vertex, bitangent));

        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_UNSIGNED_INT, sizeof(Rigged_Vertex), (void*)offsetof(Rigged_Vertex, bone_ids));

        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Rigged_Vertex), (void*)offsetof(Rigged_Vertex, bone_weights));

        glBindVertexArray(0);

        printf("Uploaded [rigged] %zu vertices, %zu indices\n", g_rigged_vertices.size(), g_rigged_indices.size());
    }

    //void upload_data() {
    //    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //    glBufferData(GL_ARRAY_BUFFER,
    //        g_vertices.size() * sizeof(Vertex),
    //        g_vertices.data(),
    //        GL_STATIC_DRAW);
    //    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    //    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
    //        g_indices.size() * sizeof(uint32_t),
    //        g_indices.data(),
    //        GL_STATIC_DRAW);
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //}

    bool is_bone_name(const std::string& name, uint32_t base_bone) {
        for (uint32_t i = base_bone; i < g_bones.size(); i++) {
            if (g_bones[i].name == name) {
                return true;
            }
        }
        return false;
    }

    aiNode* find_node_by_name(aiNode* node, const std::string& name) {
        if (node->mName.C_Str() == name) {
            return node;
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            aiNode* found = find_node_by_name(node->mChildren[i], name);
            if (found) return found;
        }

        return nullptr;
    }

    uint32_t find_parent_bone_index(const std::string& bone_name, const aiScene* scene, uint32_t base_bone) {
        aiNode* bone_node = find_node_by_name(scene->mRootNode, bone_name);
        if (!bone_node || !bone_node->mParent) {
            return UINT32_MAX; // No parent or root node
        }

        aiNode* current_parent = bone_node->mParent;
        while (current_parent) {
            std::string parent_name(current_parent->mName.C_Str());

            if (is_bone_name(parent_name, base_bone)) {
                for (uint32_t i = base_bone; i < g_bones.size(); i++) {
                    if (g_bones[i].name == parent_name) {
                        return i;
                    }
                }
            }

            current_parent = current_parent->mParent;
        }

        return UINT32_MAX; // bone is parent
    }

    void update_bone_parents(const aiScene* scene, uint32_t base_bone, uint32_t end_bone) {
        printf("[BONE] Fixing parent relationships for bones %d to %d\n", base_bone, end_bone - 1);

        for (uint32_t i = base_bone; i < end_bone; i++) {
            uint32_t parent_idx = find_parent_bone_index(g_bones[i].name, scene, base_bone);
            g_bones[i].parent_bone = parent_idx;

            if (parent_idx != UINT32_MAX) {
                printf("[BONE] %s parent is %s (index %d)\n",
                    g_bones[i].name.c_str(),
                    g_bones[parent_idx].name.c_str(),
                    parent_idx);
            }
            else {
                printf("[BONE] %s is a root bone\n", g_bones[i].name.c_str());
            }
        }
    }

    uint32_t bone_ssbo;

    void setup_bone_ssbo() {
        g_skinned_bones.resize(g_bones.size());

        for (size_t i = 0; i < g_skinned_bones.size(); i++) {
            g_skinned_bones[i].transform = glm::mat4(1.0f);
        }

        glGenBuffers(1, &bone_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bone_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            g_skinned_bones.size() * sizeof(GPU_Bone_Skinned),
            g_skinned_bones.data(),
            GL_DYNAMIC_DRAW);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bone_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        printf("Created bone SSBO with %zu bones\n", g_skinned_bones.size());
    }
    
    //
    //

    glm::mat4 get_bone_local_transform_from_animation(uint32_t bone_index, uint32_t animation_index, float time) {
        if (animation_index >= g_animations.size()) {
            return glm::mat4(1.0f);
        }
        const Animation& anim = g_animations[animation_index];

        glm::vec3 position(0.0f);
        glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale(1.0f);

        for (const Bone_Animation& bone_anim : anim.bone_animations) {
            if (bone_anim.bone_index == bone_index) {
                position = sample_position_keyframes(bone_anim.position_keyframes, time);
                rotation = sample_rotation_keyframes(bone_anim.rotation_keyframes, time);
                scale = sample_scale_keyframes(bone_anim.scale_keyframes, time);
                break;
            }
        }

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rotation_mat = glm::mat4_cast(rotation);
        glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), scale);

        return translation * rotation_mat * scale_mat;
    }

    glm::mat4 get_bone_world_transform_naive(uint32_t bone_index, uint32_t animation_index, float time) {
        if (bone_index >= g_bones.size()) {
            return glm::mat4(1.0f);
        }

        glm::mat4 local_transform = get_bone_local_transform_from_animation(bone_index, animation_index, time);

        uint32_t parent_index = g_bones[bone_index].parent_bone;
        if (parent_index != UINT32_MAX && parent_index < g_bones.size()) {
            glm::mat4 parent_world = get_bone_world_transform_naive(parent_index, animation_index, time);
            return parent_world * local_transform;
        }

        return local_transform;
    }

    void update_bones_from_animation(uint32_t animation_index, float time) {
        if (animation_index < g_animations.size()) {
            const Animation& anim = g_animations[animation_index];
            if (anim.loop && time > anim.duration)
                time = fmod(time, anim.duration);
            else if (time > anim.duration)
                time = anim.duration;
        }

        for (size_t i = 0; i < g_bones.size(); i++)
            g_skinned_bones[i].transform = get_bone_world_transform_naive(i, animation_index, time) * g_bones[i].inverse_bind;

        // printf("%f\n", time);
        // const glm::mat4& m = g_skinned_bones[10].transform;
        // printf("  [%.3f %.3f %.3f %.3f]\n", m[0][0], m[1][0], m[2][0], m[3][0]);
        // printf("  [%.3f %.3f %.3f %.3f]\n", m[0][1], m[1][1], m[2][1], m[3][1]);
        // printf("  [%.3f %.3f %.3f %.3f]\n", m[0][2], m[1][2], m[2][2], m[3][2]);
        // printf("  [%.3f %.3f %.3f %.3f]\n", m[0][3], m[1][3], m[2][3], m[3][3]);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bone_ssbo);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, g_skinned_bones.size() * sizeof(glm::mat4), g_skinned_bones.data());
    }



    glm::vec3 sample_position_keyframes(const std::vector<Position_Keyframe>& keyframes, float time) {
        if (keyframes.empty()) return glm::vec3(0.0f);
        if (keyframes.size() == 1) return keyframes[0].position;

        for (size_t i = 0; i < keyframes.size() - 1; i++) {
            if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
                float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
                return glm::mix(keyframes[i].position, keyframes[i + 1].position, t);
            }
        }
        return keyframes.back().position;
    }

    glm::quat sample_rotation_keyframes(const std::vector<Rotation_Keyframe>& keyframes, float time) {
        if (keyframes.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        if (keyframes.size() == 1) return keyframes[0].rotation;

        for (size_t i = 0; i < keyframes.size() - 1; i++) {
            if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
                float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
                return glm::normalize(glm::slerp(keyframes[i].rotation, keyframes[i + 1].rotation, t));
            }
        }
        return keyframes.back().rotation;
    }

    glm::vec3 sample_scale_keyframes(const std::vector<Scale_Keyframe>& keyframes, float time) {
        if (keyframes.empty()) return glm::vec3(1.0f);
        if (keyframes.size() == 1) return keyframes[0].scale;

        for (size_t i = 0; i < keyframes.size() - 1; i++) {
            if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
                float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
                return glm::mix(keyframes[i].scale, keyframes[i + 1].scale, t);
            }
        }
        return keyframes.back().scale;
    }

    uint32_t get_bone_ssbo() { return bone_ssbo; }
    uint32_t get_big_vao() { return big_buffer_vao; }
    uint32_t get_rigged_vao() { return rigged_vao; }
}
