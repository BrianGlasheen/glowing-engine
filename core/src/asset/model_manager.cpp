#include <vector>
#include <string>
#include <cassert>

#include <stb_image.h>

#include "model_manager.h"
#include "asset/model.h"
#include "asset/material_manager.h"

#include "core/opengl.h"

namespace Model_Manager {

    static std::vector<Model> models;
    static std::vector<std::string> names;
    static std::string base_path;

    glm::mat4 assimp_to_glm(const aiMatrix4x4& ai_mat) {
        return glm::mat4(
            ai_mat.a1, ai_mat.b1, ai_mat.c1, ai_mat.d1,
            ai_mat.a2, ai_mat.b2, ai_mat.c2, ai_mat.d2,
            ai_mat.a3, ai_mat.b3, ai_mat.c3, ai_mat.d3,
            ai_mat.a4, ai_mat.b4, ai_mat.c4, ai_mat.d4
        );
    }

    static bool loaded_already(const std::string& new_model_name, size_t& existing_idx) {
        for (size_t i = 0; i < names.size(); i++) {
            if (new_model_name == names[i]) {
                existing_idx = i;
                return true;
            }
        }
        return false;
    }

    void init(std::string path) {
        base_path = path;
        model_handle mh = load_model("teapot.obj", 0);
    }

    void cleanup() {
        // need to free resources that model owns, prob implement in destructor
        //models.clear();
        //names.clear();
    }

    model_handle load_model(const std::string& model_name, int gltf) {
        size_t existing_idx;
        if (loaded_already(model_name, existing_idx)) {
            printf("[MODEL] Already loaded: %s\n", model_name.c_str());
            return existing_idx;
        }

        std::string full_path;
        //if (gltf)
        //    full_path = base_path + model_name + "/scene.gltf";
        //else
            full_path = base_path + model_name;
        
        printf("[MODEL] Loading: %s\n", full_path.c_str());

        Model model;
        int fail = model.load_model(full_path);
        if (fail)
            return 0; // default model

        size_t new_idx = models.size();
        models.push_back(std::move(model));
        names.push_back(model_name);

        return new_idx;
    }

    Model& get_model_by_name(const std::string& model_name) {
        for (size_t i = 0; i < names.size(); i++) {
            if (model_name == names[i]) {
                return models[i];
            }
        }

        assert(false);
    }

    Model& get_model(const model_handle model_id) {
        return models[model_id];
    }

    void draw(const Shader* shader, const model_handle model_id, bool shadow_pass) {
        models[model_id].draw(shader, shadow_pass);
    }


    //Model& get_model_by_name_load(const std::string& model_name) {
    //    size_t index;
    //    if (!loaded_already(model_name, index)) {
    //        index = load_model(model_name);
    //    }
    //    return models[index];
    //}

    size_t get_model_count() {
        return models.size();
    }

    std::string get_name(const model_handle& model_id) {
        return names[model_id];
    }

    Util::AABB get_aabb(const model_handle& model_id) {
        return models[model_id].get_aabb();
    }

    // indirect stuff
    // todo change to some kind of block manager thing so i cna get rid of cpu verticies when uploaded but still track them
    static uint32_t num_models = 0;
    static uint32_t num_meshes = 0;
    static uint32_t big_buffer_vao, vbo, ebo;

    static std::vector<Model_Indirect> m_indirect_models(0);
    static std::vector<std::string> m_indirect_model_names(0);
    static std::vector<Vertex> g_vertices(0);
    // mesh has a base vertex, and a num verticies
    static std::vector<uint32_t> g_indices(0);
    // mesh has a base index, num indices
    // ssbo for per mesh data (model, texture, etc)

    //uint32_t get_num_meshes() { return num_meshes;  }
    //uint32_t get_num_models() { return num_models; }

    bool indirect_model_loaded(const std::string& full_path, model_handle& model_index) {
        for (size_t i = 0; i < m_indirect_model_names.size(); i++) {
            if (full_path == m_indirect_model_names[i]) {
                model_index = i;
                return true;
            }
        }
        return false;
    }

    model_handle load_model_indirect(const std::string& path) {
        printf("num verts before model %d\n", g_vertices.size());
        printf("num idx before model %d\n", g_indices.size());

        const std::string full_path = base_path + path;

        model_handle model_index;
        if (indirect_model_loaded(full_path, model_index))
            return model_index;
        
        Model_Indirect model_ind;

        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(full_path, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            printf("ERROR::ASSIMP::%s\n", import.GetErrorString());
            return false;
        }

        const std::string path_without_filename = full_path.substr(0, full_path.find_last_of("/") + 1);

        process_node(scene->mRootNode, scene, model_ind, path_without_filename, glm::mat4(1.0f));
        model_ind.calculate_aabb();

        model_index = m_indirect_models.size();
        m_indirect_models.push_back(model_ind);
        num_models++;

        m_indirect_model_names.push_back(full_path);

        printf("num verts after model %d\n", g_vertices.size());
        printf("num idx after model %d\n", g_indices.size());
        return model_index;
    }

    void process_node(aiNode* node, const aiScene* scene, Model_Indirect& model_ind, const std::string& path, const glm::mat4& parent_transform) {
        glm::mat4 current_transform = parent_transform * assimp_to_glm(node->mTransformation);

        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            Mesh_Indirect mesh_ind = process_mesh(mesh, scene, path);

            mesh_ind.transform = current_transform;
            mesh_ind.aabb.max = glm::vec3(current_transform * glm::vec4(mesh_ind.aabb.max, 1.0f));
            mesh_ind.aabb.min = glm::vec3(current_transform * glm::vec4(mesh_ind.aabb.min, 1.0f));

            model_ind.add_mesh(mesh_ind);
            num_meshes++;
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            process_node(node->mChildren[i], scene, model_ind, path, current_transform);
        }
    }

    Mesh_Indirect process_mesh(aiMesh* mesh, const aiScene* scene, const std::string& path)
    {
        Mesh_Indirect mesh_ind;
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
        Material_Indirect mesh_mat {0};
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

        mesh_ind.material_index = Material_Manager::add_material(mesh_mat);
        
        return mesh_ind;
    }

    Model_Indirect get_model_ind(uint32_t idx) {
        return m_indirect_models[idx];
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

        printf("Uploaded %zu vertices, %zu indices\n",
            g_vertices.size(), g_indices.size());
    }

    void upload_data() {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
            g_vertices.size() * sizeof(Vertex),
            g_vertices.data(),
            GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            g_indices.size() * sizeof(uint32_t),
            g_indices.data(),
            GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    uint32_t get_big_vao() {
        return big_buffer_vao;
    }

    uint32_t get_vbo() {
        return vbo;
    }

}
