#include <vector>
#include <string>
#include <cassert>

#include <stb_image.h>
#include <algorithm>

#include "asset/texture_manager.h"
#include "asset/model_manager.h"
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

struct Bone {
    std::string name;
    uint32_t parent_bone; // idx into bone array of parent
    glm::mat4 inverse_bind;
};

struct Temp_Bone { // struct that will be allocated to map vertices to bones, then nuked
    uint32_t bone_index; // index into global bones
    float bone_weight;
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
    static std::vector<Model_Indirect> m_rigged_models(0);
    static std::vector<std::string> m_rigged_model_names(0);

    //uint32_t get_num_meshes() { return num_meshes;  }

    bool indirect_model_loaded(const std::string& full_path, model_handle& model_index) {
        for (size_t i = 0; i < m_indirect_model_names.size(); i++) {
            printf("%d %s\n", i, m_indirect_model_names[i].c_str());
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
        
        if (rigged) {
            model_index = m_rigged_models.size();

            m_rigged_models.push_back(model_ind);
            m_rigged_model_names.push_back(full_path);

            printf("num rigged verts after model %d\n", g_rigged_vertices.size());
            printf("num rigged idx after model %d\n", g_rigged_indices.size());
            printf("num bones after model %d\n", g_bones.size());
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
        mesh_ind.rigged = true;

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
        //new_bone.parent_bone = find_parent_bone_index(bone_name, scene); todo

        g_bones.push_back(new_bone);
        return g_bones.size() - 1;
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

    uint32_t get_big_vao() { return big_buffer_vao; }
    uint32_t get_rigged_vao() { return rigged_vao; }
}
