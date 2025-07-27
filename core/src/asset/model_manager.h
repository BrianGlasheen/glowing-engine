#pragma once

#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "shader.h"
#include "util/aabb.h"
#include "asset/model_indirect.h"

typedef uint32_t model_handle;

namespace Model_Manager {
    glm::mat4 assimp_to_glm(const aiMatrix4x4& ai_mat);

    void init(std::string path);
    void cleanup();

    //uint32_t get_num_meshes();
    //uint32_t get_num_models();

    bool indirect_model_loaded(const std::string& full_path, model_handle& model_index);

    model_handle load_rigged_model(const std::string& path);
    model_handle load_model_indirect(const std::string& path, bool rigged = false);
    void process_node(aiNode* node, const aiScene* scene, Model_Indirect& model_ind, const std::string& path, const glm::mat4& parent_transform, bool rigged, uint32_t base_bone);
    Mesh_Indirect process_mesh(const aiMesh* mesh, const aiScene* scene, const std::string& path);
    Mesh_Indirect process_rigged_mesh(const aiMesh* mesh, const aiScene* scene, const std::string& path, uint32_t base_bone);

    Material_Indirect load_material(const aiMesh* mesh, const aiScene* scene, const std::string& path);
    uint32_t find_or_create_global_bone(const aiBone* bone, const aiScene* scene, uint32_t base_bone);

    Model_Indirect get_model_ind(uint32_t idx);
    Model_Indirect get_skinned_model(uint32_t idx);
    Util::AABB get_aabb_indirect(const model_handle& model_id);

    void setup_buffers();
    //void upload_data();

    uint32_t get_big_vao();
    uint32_t get_rigged_vao();
}
