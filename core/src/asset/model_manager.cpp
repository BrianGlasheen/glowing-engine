#include <vector>
#include <string>
#include <cassert>

#include <stb_image.h>

#include "model_manager.h"
#include "asset/model.h"

#include "core/opengl.h"

namespace Model_Manager {

    static std::vector<Model> models;
    static std::vector<std::string> names;
    static std::string base_path;

    static bool loaded_already(const std::string& new_model_name, size_t& existing_idx) 
    {
        for (size_t i = 0; i < names.size(); i++) {
            if (new_model_name == names[i]) {
                existing_idx = i;
                return true;
            }
        }
        return false;
    }

    void init(std::string path) 
    {
        base_path = path;
        model_handle mh = load_model("teapot.obj", 0);
    }

    void cleanup() 
    {
        // need to free resources that model owns, prob implement in destructor
        //models.clear();
        //names.clear();
    }

    model_handle load_model(const std::string& model_name, int gltf) 
    {
        size_t existing_idx;
        if (loaded_already(model_name, existing_idx)) {
            printf("[MODEL] Already loaded: %s\n", model_name.c_str());
            return existing_idx;
        }

        std::string full_path;
        if (gltf)
            full_path = base_path + model_name + "/scene.gltf";
        else
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

    Model& get_model_by_name(const std::string& model_name) 
    {
        for (size_t i = 0; i < names.size(); i++) {
            if (model_name == names[i]) {
                return models[i];
            }
        }

        assert(false);
    }

    Model& get_model(const model_handle model_id) 
    {
        return models[model_id];
    }

    void draw(const Shader* shader, const model_handle model_id, bool shadow_pass) 
    {
        models[model_id].draw(shader, shadow_pass);
    }


    //Model& get_model_by_name_load(const std::string& model_name) {
    //    size_t index;
    //    if (!loaded_already(model_name, index)) {
    //        index = load_model(model_name);
    //    }
    //    return models[index];
    //}

    size_t get_model_count() 
    {
        return models.size();
    }

    std::string get_name(const model_handle& model_id) 
    {
        return names[model_id];
    }

    Util::AABB get_aabb(const model_handle& model_id) 
    {
        return models[model_id].get_aabb();
    }

    // indirect stuff
    // todo change to some kind of block manager thing so i cna get rid of cpu verticies when uploaded but still track them
    static uint32_t num_models = 0;
    static uint32_t num_meshes = 0;
    static uint32_t big_buffer_vao, vbo, ebo;

    static std::vector<Model_Indirect> m_indirect_models(0);
    static std::vector<Vertex> g_vertices(0);
    // mesh has a base vertex, and a num verticies
    static std::vector<uint32_t> g_indices(0);
    // mesh has a base index, num indices
    // ssbo for per mesh data (model, texture, etc)

    uint32_t get_num_meshes() { return num_meshes;  }
    uint32_t get_num_models() { return num_models; }

    bool load_model_indirect(const std::string& path) {
        printf("num verts before model %d\n", g_vertices.size());
        printf("num idx before model %d\n", g_indices.size());
        
        Model_Indirect model_ind;

        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(path, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            printf("ERROR::ASSIMP::%s\n", import.GetErrorString());
            return false;
        }

        process_node(scene->mRootNode, scene, model_ind);
        model_ind.calculate_aabb();
        m_indirect_models.push_back(model_ind);
        num_models++;
        printf("num verts after model %d\n", g_vertices.size());
        printf("num idx after model %d\n", g_indices.size());
    }

    void process_node(aiNode* node, const aiScene* scene, Model_Indirect& model_ind) // todo add transform hierarchy
    {
        // process all the node's meshes (if any)
        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            model_ind.add_mesh(process_mesh(mesh, scene));
            num_meshes++;
        }

        // then do the same for each of its children
        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            process_node(node->mChildren[i], scene, model_ind);
        }
    }

    Mesh_Indirect process_mesh(aiMesh* mesh, const aiScene* scene)
    {
        Mesh_Indirect mesh_ind;
        mesh_ind.name = std::string(mesh->mName.C_Str());
        printf("loading mesh %s\n", mesh_ind.name.c_str());

        mesh_ind.aabb.max = glm::vec3(FLT_MIN);
        mesh_ind.aabb.min = glm::vec3(FLT_MAX);

        mesh_ind.base_vertex = g_vertices.size();// 
        uint32_t vertex_count = mesh->mNumVertices;

        for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            
            vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            // not the prettiest
            if (vertex.position.x < mesh_ind.aabb.min.x) mesh_ind.aabb.min.x = vertex.position.x;
            if (vertex.position.y < mesh_ind.aabb.min.y) mesh_ind.aabb.min.y = vertex.position.y;
            if (vertex.position.z < mesh_ind.aabb.min.z) mesh_ind.aabb.min.z = vertex.position.z;
            if (vertex.position.x > mesh_ind.aabb.max.x) mesh_ind.aabb.max.x = vertex.position.x;
            if (vertex.position.y > mesh_ind.aabb.max.y) mesh_ind.aabb.max.y = vertex.position.y;
            if (vertex.position.z > mesh_ind.aabb.max.z) mesh_ind.aabb.max.z = vertex.position.z;

            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

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

        return mesh_ind;
    }

    Model_Indirect get_model_ind(uint32_t idx) {
        return m_indirect_models[idx];
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
