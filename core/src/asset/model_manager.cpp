#include "asset/model_manager.h"

#include "glow.h"

#include <string>
#include <cassert>
#include <algorithm>
#include <set>

#include <stb_image.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "asset/texture_manager.h"
#include "asset/material_manager.h"
#include "asset/shader_manager.h"

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

struct GPU_Bone { // combine this bone with animation data to get skinned bone
    glm::mat4 inverse_bind;
    uint32_t parent_bone;
    uint32_t padding[3];
};

struct Temp_Bone { // struct that will be allocated to map vertices to bones, then nuked
    uint32_t bone_index; // index into global bones
    float bone_weight;
};

struct Animation {
    uint32_t base_bone_animation;
    uint32_t bone_animation_count;
    float duration;
    //bool loop;
};

struct Bone_Animation {
    uint32_t bone_index;
    uint32_t base_position_keyframe;
    uint32_t position_keyframe_count;
    uint32_t base_rotation_keyframe;
    uint32_t rotation_keyframe_count;
    uint32_t base_scale_keyframe;
    uint32_t scale_keyframe_count;
    uint32_t padding;
};

struct Animation_Command {
    uint32_t base_bone;
    uint32_t bone_count;
    uint32_t bone_offset;
    uint32_t base_leaf;

    uint32_t leaf_count;
    uint32_t base_bone_animation; // first bone animation, fill from g_animations
    uint32_t bone_animation_count; // num of bone animations
    float duration;

    uint32_t leaf_thread_offset;
    bool loop;
    uint32_t padding[2];
};

struct Position_Keyframe { // todo compress?
    glm::vec3 position;
    float time;
};

struct Rotation_Keyframe { // todo pack
    glm::quat rotation;
    float time;
    uint32_t padding[3];
};

struct Scale_Keyframe { // todo compress?
    glm::vec3 scale;
    float time;
};

// todo skeleton, maybe not!

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

    glm::mat4 cgltf_to_glm(const cgltf_float* matrix) {
        return glm::mat4(
            matrix[0], matrix[4], matrix[8], matrix[12],
            matrix[1], matrix[5], matrix[9], matrix[13],
            matrix[2], matrix[6], matrix[10], matrix[14],
            matrix[3], matrix[7], matrix[11], matrix[15]
        );
    }

    glm::vec3 cgltf_to_vec3(const cgltf_float* data) {
        return glm::vec3(data[0], data[1], data[2]);
    }

    glm::vec2 cgltf_to_vec2(const cgltf_float* data) {
        return glm::vec2(data[0], data[1]);
    }

    glm::quat cgltf_to_quat(const cgltf_float* data) {
        return glm::quat(data[3], data[0], data[1], data[2]); // w, x, y, z
    }

    void init(std::string path) {
        base_path = path;
        model_handle mh = load_model("teapot.obj");
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

    // animated stuff
    static uint32_t num_animated_meshes = 0;
    static uint32_t rigged_vao, r_vbo, r_ebo;

    static std::vector<Rigged_Vertex> g_rigged_vertices(0);
    //static std::vector<Vertex> g_skinned_vertices(0); // rigged verts get written here
    static std::vector<uint32_t> g_animated_indices(0); // same index buffer for skinned and non
    static std::vector<Animated_Model> m_animated_models(0);
    static std::vector<std::string> m_animated_model_names(0); // todo think about how to store names

    static std::vector<Bone> g_rigged_bones(0); // one set of bones can produce many sets of skinned
    static std::vector<uint32_t> g_leaf_bones(0);
    static uint32_t num_skinned_bones = 0;
    
    static std::vector<std::string> g_animation_names(0);
    static std::vector<Animation> g_animations(0);
    static std::vector<Bone_Animation> g_bone_animations(0);
    static std::vector<Position_Keyframe> position_keyframes(0);
    static std::vector<Rotation_Keyframe> rotation_keyframes(0);
    static std::vector<Scale_Keyframe> scale_keyframes(0);

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


    bool animated_model_loaded(const std::string& full_path, model_handle& model_index) {
        for (int i = m_animated_model_names.size() - 1; i >= 0 ; i--) { // search back to front to get most recently added for correct offset and stuff
            if (full_path == m_animated_model_names[i]) {
                model_index = i;
                return true;
            }
        }
        return false;
    }

    model_handle load_model(const std::string& path) {
        printf("num verts before model %d\n", g_vertices.size());
        printf("num idx before model %d\n", g_indices.size());

        const std::string full_path = base_path + path;

        model_handle model_index;
        if (indirect_model_loaded(full_path, model_index))
            return model_index;

        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(full_path, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_ValidateDataStructure | aiProcess_PopulateArmatureData);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            printf("ERROR::ASSIMP::%s\n", import.GetErrorString());
            return false;
        }

        const std::string path_without_filename = full_path.substr(0, full_path.find_last_of("/") + 1);

        Model_Indirect model_ind;
        model_ind.m_name = path;
        process_node(scene->mRootNode, scene, model_ind, path_without_filename, glm::mat4(1.0f));

        model_ind.calculate_aabb();
        
        model_index = m_indirect_models.size();

        m_indirect_models.push_back(model_ind);
        m_indirect_model_names.push_back(full_path);

        printf("num verts after model %d\n", g_vertices.size());
        printf("num idx after model %d\n", g_indices.size());

        return model_index;
    }

    model_handle load_animated_model(const std::string& path) {
        printf("num rigged verts before model %d\n", g_rigged_vertices.size());
        printf("num rigged idx before model %d\n", g_animated_indices.size());
        printf("num bones before model %d\n", g_rigged_bones.size());

        const std::string full_path = base_path + path;

        model_handle model_index;
        if (animated_model_loaded(full_path, model_index)) {
            printf("OYYOYOYOYOYOYOOY\n\n\n\n\nYOOO");
            Animated_Model loaded = m_animated_models[model_index];
            //num_skinned_bones += model.bone_count;

            Animated_Model copy;

            copy.m_meshes = loaded.m_meshes; // todo change when duplicating verts
            copy.m_aabb = loaded.m_aabb;
            copy.base_bone = loaded.base_bone;
            copy.bone_count = loaded.bone_count;
            copy.bone_offset = num_skinned_bones - loaded.base_bone; assert(copy.bone_offset >= 0);
            num_skinned_bones += copy.bone_count;
            copy.base_leaf = loaded.base_leaf;
            copy.leaf_count = loaded.leaf_count;
            copy.base_animation = loaded.base_animation;
            copy.animation_count = loaded.animation_count;

            model_index = m_animated_models.size();
            m_animated_models.push_back(copy);
            m_animated_model_names.push_back(full_path);

            //printf("offset: %d, tot: %d\n", copy.bone_offset, num_skinned_bones);
            printf("num rigged verts after model %d\n", g_rigged_vertices.size());
            printf("num rigged idx after model %d\n", g_animated_indices.size());
            printf("num bones after model %d\n", g_rigged_bones.size());
            // leaf bones
            // maybe kf's
            printf("here\n");
            printf("num animations loaded %d\n", g_animations.size());
            printf("not here\n");

            return model_index;
        }
         

        Assimp::Importer import;
        import.SetPropertyBool(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);
        const aiScene* scene = import.ReadFile(full_path, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            printf("ERROR::ASSIMP::%s\n", import.GetErrorString());
            return false;
        }

        const std::string path_without_filename = full_path.substr(0, full_path.find_last_of("/") + 1);

        Animated_Model model;
        model.m_name = path;

        uint32_t base_bone = g_rigged_bones.size();
        process_animated_node(scene->mRootNode, scene, model, path_without_filename, glm::mat4(1.0f), base_bone);

        model.base_bone = base_bone;
        model.bone_count = g_rigged_bones.size() - base_bone;
        model.bone_offset = num_skinned_bones - model.base_bone; assert(model.bone_offset >= 0);
        num_skinned_bones += model.bone_count;

        if (scene->mNumAnimations > 0) {
            update_bone_parents(scene, base_bone, g_rigged_bones.size());

            model.base_leaf = g_leaf_bones.size();
            add_leaf_bones(base_bone, g_rigged_bones.size());
            model.leaf_count = g_leaf_bones.size() - model.base_leaf;

            printf("LOADING ANIMATIONS\n");

            model.base_animation = g_animations.size();
            load_animations_from_scene(scene, base_bone);
            model.animation_count = g_animations.size() - model.base_animation;

            printf("base bone %d\n", model.base_bone);
            printf("bone count%d\n", model.bone_count);
            printf("bone offset%d\n", model.bone_offset);
            printf("base leaf%d\n", model.base_leaf);
            printf("leaf count %d\n", model.leaf_count);
            printf("base animation %d\n", model.base_animation);
            printf("animation count %d\n", model.base_animation);
        }

        model_index = m_animated_models.size();

        m_animated_models.push_back(model);
        m_animated_model_names.push_back(full_path);

        printf("num rigged verts after model %d\n", g_rigged_vertices.size());
        printf("num rigged idx after model %d\n", g_animated_indices.size());
        printf("num bones after model %d\n", g_rigged_bones.size());
        // leaf bones
        // maybe kf's
        //printf("num animations loaded %d\n", g_animations.size());
        return model_index;
    }

    void process_node(aiNode* node, const aiScene* scene, Model_Indirect& model, const std::string& path, const glm::mat4& parent_transform) {
        glm::mat4 current_transform = parent_transform * assimp_to_glm(node->mTransformation);

        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];

            Mesh_Indirect mesh = process_mesh(ai_mesh, scene, path);

            mesh.transform = current_transform;
            mesh.aabb.max = glm::vec3(current_transform * glm::vec4(mesh.aabb.max, 1.0f));
            mesh.aabb.min = glm::vec3(current_transform * glm::vec4(mesh.aabb.min, 1.0f));

            model.add_mesh(mesh);
            num_meshes++;
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            process_node(node->mChildren[i], scene, model, path, current_transform);
        }
    }

    void process_animated_node(aiNode* node, const aiScene* scene, Animated_Model& model, const std::string& path, const glm::mat4& parent_transform, uint32_t base_bone) {
        glm::mat4 current_transform = parent_transform * assimp_to_glm(node->mTransformation);

        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];

            Animated_Mesh mesh = process_animated_mesh(ai_mesh, scene, path, base_bone);

            mesh.transform = current_transform;
            mesh.aabb.max = glm::vec3(current_transform * glm::vec4(mesh.aabb.max, 1.0f));
            mesh.aabb.min = glm::vec3(current_transform * glm::vec4(mesh.aabb.min, 1.0f));

            model.add_mesh(mesh);
            num_animated_meshes++;
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            process_animated_node(node->mChildren[i], scene, model, path, current_transform, base_bone);
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

    Animated_Mesh process_animated_mesh(const aiMesh* mesh, const aiScene* scene, const std::string& path, uint32_t base_bone) {
        Animated_Mesh mesh_ind = { 0 };
        mesh_ind.name = std::string(mesh->mName.C_Str());
        printf("loading RIGGED mesh %s\n", mesh_ind.name.c_str());
        //mesh_ind.rigged = true;

        mesh_ind.aabb.min = glm::vec3(FLT_MAX);
        mesh_ind.aabb.max = glm::vec3(-FLT_MAX);

        uint32_t num_vertices = mesh->mNumVertices;

        //// process bones
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
            std::sort(vertex_bones.begin(), vertex_bones.end(), [](Temp_Bone f1, Temp_Bone f2) { return f1.bone_weight > f2.bone_weight; });
            for (uint32_t i = 0; i < num_bones; i++) {
                vertex.bone_ids[i] = vertex_bones[i].bone_index;
                vertex.bone_weights[i] = vertex_bones[i].bone_weight;
            }
            // todo maybe normalize weights?

            g_rigged_vertices.push_back(vertex);
        }

        //// process indices
        mesh_ind.base_index = g_animated_indices.size();

        uint32_t num_idcs = 0;
        for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
                g_animated_indices.push_back(face.mIndices[j]); // todo probably just reserver size and copy big chunk

            num_idcs += face.mNumIndices;
        }
        mesh_ind.index_count = num_idcs;

        //// process material
        Material_Indirect material = load_material(mesh, scene, path);
        mesh_ind.material_index = Material_Manager::add_material(material);

        return mesh_ind;
    }

    Material_Indirect load_material(const aiMesh* mesh, const aiScene* scene, const std::string& path) {
        Material_Indirect mesh_mat{ 0 };
        printf("path is :%s\n", path.c_str());
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            //#define AI_MATKEY_BASE_COLOR "$clr.base", 0, 0

            if (material->GetTextureCount(aiTextureType_BASE_COLOR)) {
                aiString str;
                material->GetTexture(aiTextureType_BASE_COLOR, 0, &str);
                #if BINDLESS
                    mesh_mat.albedo = Texture_Manager::load_bindless_from_path(path + str.C_Str());
                #else
                    mesh_mat.albedo = Texture_Manager::load_from_path(path + str.C_Str());
                #endif
            }
            if (material->GetTextureCount(aiTextureType_NORMALS)) {
                aiString str;
                material->GetTexture(aiTextureType_NORMALS, 0, &str);
                #if BINDLESS
                    mesh_mat.normal = Texture_Manager::load_bindless_from_path(path + str.C_Str());
                #else
                    mesh_mat.normal = Texture_Manager::load_from_path(path + str.C_Str());
                #endif
            }

            // todo why metallic 1?
            float metallic = 0.0f;
            float roughness = 1.0f;
            if (material->GetTextureCount(aiTextureType_GLTF_METALLIC_ROUGHNESS)) {
                aiString str;
                material->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &str);
                // printf("MET ROUGHESNSENSENESNESNE %s\n", str.C_Str());
                #if BINDLESS
                    mesh_mat.met_rough = Texture_Manager::load_bindless_from_path(path + str.C_Str());
                #else
                    mesh_mat.met_rough = Texture_Manager::load_from_path(path + str.C_Str());
                #endif
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
                #if BINDLESS
                    mesh_mat.emissive = Texture_Manager::load_bindless_from_path(path + str.C_Str());
                #else
                    mesh_mat.emissive = Texture_Manager::load_from_path(path + str.C_Str());
                #endif
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
                #if BINDLESS
                    mesh_mat.amb_occ = Texture_Manager::load_bindless_from_path(path + str.C_Str());
                #else
                    mesh_mat.amb_occ = Texture_Manager::load_from_path(path + str.C_Str());
                #endif
            }
        }

        return mesh_mat;
    }

    uint32_t find_or_create_global_bone(const aiBone* bone, const aiScene* scene, uint32_t base_bone) {
        std::string bone_name(bone->mName.C_Str());
        
        // base bone is g_bones.size() before loading a model
        for (uint32_t i = base_bone; i < g_rigged_bones.size(); i++) {
            if (g_rigged_bones[i].name == bone_name)
                return i;
        }

        printf("[BONE] adding bone %s\n", bone_name.c_str());

        Bone new_bone;
        new_bone.name = bone_name;
        new_bone.inverse_bind = assimp_to_glm(bone->mOffsetMatrix);
        new_bone.parent_bone = UINT32_MAX;

        g_rigged_bones.push_back(new_bone);
        return g_rigged_bones.size() - 1;
    }

    void load_animations_from_scene(const aiScene* scene, uint32_t base_bone) {
        for (uint32_t anim_idx = 0; anim_idx < scene->mNumAnimations; ++anim_idx) {
            aiAnimation* ai_anim = scene->mAnimations[anim_idx];

            Animation animation;
            animation.duration = static_cast<float>(ai_anim->mDuration / ai_anim->mTicksPerSecond);
            animation.base_bone_animation = g_bone_animations.size();

            printf("Loading animation: %s, duration: %.2f seconds\n", ai_anim->mName.C_Str(), animation.duration);

            for (uint32_t channel_idx = 0; channel_idx < ai_anim->mNumChannels; ++channel_idx) {
                aiNodeAnim* ai_channel = ai_anim->mChannels[channel_idx];

                uint32_t bone_index = find_bone_index(ai_channel->mNodeName.C_Str(), base_bone);

                Bone_Animation bone_anim;
                bone_anim.bone_index = bone_index;
                bone_anim.base_position_keyframe = position_keyframes.size();
                bone_anim.base_rotation_keyframe = rotation_keyframes.size();
                bone_anim.base_scale_keyframe = scale_keyframes.size();

                load_keyframes_from_channel(ai_channel, ai_anim->mTicksPerSecond);

                bone_anim.position_keyframe_count = position_keyframes.size() - bone_anim.base_position_keyframe;
                bone_anim.rotation_keyframe_count = rotation_keyframes.size() - bone_anim.base_rotation_keyframe;
                bone_anim.scale_keyframe_count = scale_keyframes.size() - bone_anim.base_scale_keyframe;

                g_bone_animations.push_back(bone_anim);
            }

            animation.bone_animation_count = g_bone_animations.size() - animation.base_bone_animation;

            g_animations.push_back(animation);
            g_animation_names.push_back(std::string(ai_anim->mName.C_Str()));
        }
    }

    void load_keyframes_from_channel(aiNodeAnim* channel, double ticks_per_second) {
        for (uint32_t i = 0; i < channel->mNumPositionKeys; ++i) {
            Position_Keyframe keyframe;
            aiVector3D pos = channel->mPositionKeys[i].mValue;
            keyframe.position = glm::vec3(pos.x, pos.y, pos.z);
            keyframe.time = static_cast<float>(channel->mPositionKeys[i].mTime / ticks_per_second);
            position_keyframes.push_back(keyframe);
        }

        for (uint32_t i = 0; i < channel->mNumRotationKeys; ++i) {
            Rotation_Keyframe keyframe;
            aiQuaternion rot = channel->mRotationKeys[i].mValue;
            keyframe.rotation = glm::quat(rot.w, rot.x, rot.y, rot.z);
            keyframe.time = static_cast<float>(channel->mRotationKeys[i].mTime / ticks_per_second);
            rotation_keyframes.push_back(keyframe);
        }

        for (uint32_t i = 0; i < channel->mNumScalingKeys; ++i) {
            Scale_Keyframe keyframe;
            aiVector3D scale = channel->mScalingKeys[i].mValue;
            keyframe.scale = glm::vec3(scale.x, scale.y, scale.z);
            keyframe.time = static_cast<float>(channel->mScalingKeys[i].mTime / ticks_per_second);
            scale_keyframes.push_back(keyframe);
        }
    }

    uint32_t find_bone_index(const std::string& bone_name, uint32_t base_bone) {
        for (uint32_t i = base_bone; i < g_rigged_bones.size(); ++i) {
            if (g_rigged_bones[i].name == bone_name) {
                return i;
            }
        }
        printf("bone %s not found\n", bone_name.c_str());
        assert(false);
    }

    Model_Indirect get_model_ind(uint32_t idx) {
        return m_indirect_models[idx];
    }
    
    Animated_Model get_animated_model(uint32_t idx) {
        return m_animated_models[idx];
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, g_animated_indices.size() * sizeof(uint32_t), &g_animated_indices[0], GL_STATIC_DRAW);

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

        printf("Uploaded [rigged] %zu vertices, %zu indices\n", g_rigged_vertices.size(), g_animated_indices.size());
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
        for (uint32_t i = base_bone; i < g_rigged_bones.size(); i++) {
            if (g_rigged_bones[i].name == name) {
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
                for (uint32_t i = base_bone; i < g_rigged_bones.size(); i++) {
                    if (g_rigged_bones[i].name == parent_name) {
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
            uint32_t parent_idx = find_parent_bone_index(g_rigged_bones[i].name, scene, base_bone);
            g_rigged_bones[i].parent_bone = parent_idx;

            if (parent_idx != UINT32_MAX) {
                printf("[BONE] %s parent is %s (index %d)\n",
                    g_rigged_bones[i].name.c_str(),
                    g_rigged_bones[parent_idx].name.c_str(),
                    parent_idx);
            }
            else {
                printf("[BONE] %s is a root bone\n", g_rigged_bones[i].name.c_str());
            }
        }
    }

    void add_leaf_bones(uint32_t base_bone, uint32_t end_bone) {
        for (uint32_t bone = base_bone; bone < end_bone; bone++) {
            bool leaf = true;

            for (uint32_t other_bone = base_bone; other_bone < end_bone; other_bone++) {
                if (g_rigged_bones[other_bone].parent_bone == bone) {
                    leaf = false;
                    break;
                }
            }

            if (leaf) {
                g_leaf_bones.push_back(bone);
                printf("bone: %s is a leaf\n", g_rigged_bones[bone].name.c_str());
            }
        }
    }


    uint32_t animation_commands, bone_ssbo, skinned_bone_ssbo, pos_keys_ssbo, rot_keys_ssbo, scale_keys_ssbo, bone_animation_ssbo, leaf_bones_ssbo, absolute_bone_transform_ssbo, transform_time_ssbo;

    void setup_ssbos() {
        Shader_Manager::load_compute("animate_skeleton");

        // setup rigged bones
        // setup skinned bones
        //g_skinned_bones.reserve(g_rigged_bones.size());
        std::vector<glm::mat4> absolute_transforms(num_skinned_bones);
        std::vector<float> absolute_transform_times(num_skinned_bones);

        for (size_t i = 0; i < num_skinned_bones; i++) {
            //g_skinned_bones[i].transform = glm::mat4(1.0f);
            absolute_transforms[i] = glm::mat4(1.0f);
            absolute_transform_times[i] = 0.0f;
        }

        glGenBuffers(1, &skinned_bone_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, skinned_bone_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, num_skinned_bones * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, skinned_bone_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        //printf("Created bone SSBO with %zu bytes\n", num_skinned_bones * sizeof(glm::mat4));

        glGenBuffers(1, &absolute_bone_transform_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, absolute_bone_transform_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, absolute_transforms.size() * sizeof(glm::mat4), absolute_transforms.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, absolute_bone_transform_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glGenBuffers(1, &transform_time_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, transform_time_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, absolute_transform_times.size() * sizeof(float), absolute_transform_times.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, transform_time_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        //bone_ssbo
        std::vector<GPU_Bone> rigged_bones_temp;
        rigged_bones_temp.reserve(g_rigged_bones.size());
        for (const Bone& b : g_rigged_bones)
            rigged_bones_temp.push_back({ b.inverse_bind, b.parent_bone });

        glGenBuffers(1, &bone_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bone_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, rigged_bones_temp.size() * sizeof(GPU_Bone), rigged_bones_temp.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bone_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        
        glGenBuffers(1, &animation_commands);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, animation_commands);
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_animated_models.size() * sizeof(Animation_Command), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bone_animation_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        //bone_animation_ssbo
        glGenBuffers(1, &bone_animation_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bone_animation_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, g_bone_animations.size() * sizeof(Bone_Animation), g_bone_animations.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bone_animation_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glGenBuffers(1, &pos_keys_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, pos_keys_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, position_keyframes.size() * sizeof(Position_Keyframe), position_keyframes.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pos_keys_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glGenBuffers(1, &rot_keys_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, rot_keys_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, rotation_keyframes.size() * sizeof(Rotation_Keyframe), rotation_keyframes.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, rot_keys_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glGenBuffers(1, &scale_keys_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, scale_keys_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, scale_keyframes.size() * sizeof(Scale_Keyframe), scale_keyframes.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scale_keys_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        //leaf_bones_ssbo;
        glGenBuffers(1, &leaf_bones_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, leaf_bones_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, g_leaf_bones.size() * sizeof(uint32_t), g_leaf_bones.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, leaf_bones_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    static bool once = true;
    static uint32_t num_leafs = 0;
    static uint32_t n_cmds = 0;

    void update_bones_from_animation_compute(uint32_t animation_index, float time) {
        //const Animation& anim = g_animations[animation_index];

        if (once) {
            once = false;
            std::vector<Animation_Command> cmds(0);

            for (uint32_t i = 0; i < Model_Manager::get_num_animated_models(); i++) {
                Animated_Model m = m_animated_models[i];

                //printf("%s base anim %d, num animations %d\n", m.m_name.c_str(), m.base_animation, m.animation_count);

                Animation a = g_animations[m.base_animation];

                Animation_Command cmd = { m.base_bone, m.bone_count, m.bone_offset, m.base_leaf, m.leaf_count, a.base_bone_animation, a.bone_animation_count, a.duration, num_leafs };
                cmds.push_back(cmd);

                num_leafs += m.leaf_count;

                // todo accumalte number of leafd nodes for dispatch
                // thread leaf offset = accumulated - base_leaf base
                n_cmds++;
            }


            glNamedBufferData(animation_commands, cmds.size() * sizeof(Animation_Command), cmds.data(), GL_DYNAMIC_DRAW);

            for (uint32_t i = 0; i < g_animations.size(); i++) {
                printf("%s: ", g_animation_names[i].c_str());
                printf("%d | %d\n", g_animations[i].base_bone_animation, g_animations[i].bone_animation_count);
            }
        }


        Compute_Shader* skeleton = Shader_Manager::get_compute("animate_skeleton");
        skeleton->use();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, animation_commands);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, leaf_bones_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bone_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, skinned_bone_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, absolute_bone_transform_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, transform_time_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bone_animation_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, pos_keys_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, rot_keys_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, scale_keys_ssbo);

         //printf("Dispatching animation at time: %.3f\n", time);

        skeleton->set_float("dispatch_time", time);
        skeleton->set_uint("num_animation_cmds", n_cmds);
        //skeleton->set_uint("max_leaf", (uint32_t)g_leaf_bones.size());
        //skeleton->set_uint("animation_count", (uint32_t)g_animations[0].bone_animations.size());

        skeleton->dispatch_and_wait((num_leafs + 63) / 64, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
    }

    uint32_t get_bone_ssbo() { return bone_ssbo; }
    uint32_t get_skinned_bone_ssbo() { return skinned_bone_ssbo; }
    uint32_t get_num_animated_models() { return m_animated_models.size(); }
    uint32_t get_animation_command_ssbo() { return animation_commands; }

    uint32_t get_big_vao() { return big_buffer_vao; }
    uint32_t get_rigged_vao() { return rigged_vao; }
}
