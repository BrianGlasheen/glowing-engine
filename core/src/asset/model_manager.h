#pragma once

#include <string>

#include "model.h"
#include "shader.h"
#include "util/aabb.h"
#include "asset/model_indirect.h"

typedef size_t model_handle;

namespace Model_Manager {
    glm::mat4 assimp_to_glm(const aiMatrix4x4& ai_mat);

    void init(std::string path);
    void cleanup();

    model_handle load_model(const std::string& model_name, int gltf = 1);
    Model& get_model_by_name(const std::string& model_name);
    Model& get_model(const model_handle model_id);

    //Model& get_model_by_name_load(const std::string& model_name);
    void draw(const Shader* shader, const model_handle model_id, bool shadow_pass = false);

    size_t get_model_count();
    std::string get_name(const model_handle& model_id);
    Util::AABB get_aabb(const model_handle& model_id);

    uint32_t get_num_meshes();
    uint32_t get_num_models();
    bool load_model_indirect(const std::string& path);
    void process_node(aiNode* node, const aiScene* scene, Model_Indirect& model_ind, const std::string& path, const glm::mat4& parent_transform);
    Mesh_Indirect process_mesh(aiMesh* mesh, const aiScene* scene, const std::string& path);

    Model_Indirect get_model_ind(uint32_t idx);

    void setup_buffers();
    void upload_data();

    uint32_t get_big_vao();
    uint32_t get_vbo();
}
