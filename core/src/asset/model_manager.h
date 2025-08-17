#pragma once

#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/quaternion.hpp>

#include "shader.h"
#include "util/aabb.h"
#include "asset/model_indirect.h" // todo could maybe combine still
#include "asset/animated_model.h"

typedef uint32_t model_handle;

namespace Model_Manager {
    glm::mat4 assimp_to_glm(const aiMatrix4x4& ai_mat);

    void init(std::string path);
    void cleanup();

    //uint32_t get_num_meshes();
    //uint32_t get_num_models();

    bool indirect_model_loaded(const std::string& full_path, model_handle& model_index);
    bool animated_model_loaded(const std::string& full_path, model_handle& model_index);

    model_handle load_model(const std::string& path);
    model_handle load_animated_model(const std::string& path);

    void process_node(aiNode* node, const aiScene* scene, Model_Indirect& model, const std::string& path, const glm::mat4& parent_transform);
    void process_animated_node(aiNode* node, const aiScene* scene, Animated_Model& model, const std::string& path, const glm::mat4& parent_transform, uint32_t base_bone);

    Mesh_Indirect process_mesh(const aiMesh* mesh, const aiScene* scene, const std::string& path);
    Animated_Mesh process_animated_mesh(const aiMesh* mesh, const aiScene* scene, const std::string& path, uint32_t base_bone);

    Material_Indirect load_material(const aiMesh* mesh, const aiScene* scene, const std::string& path);
    uint32_t find_or_create_global_bone(const aiBone* bone, const aiScene* scene, uint32_t base_bone);

    void load_animations_from_scene(const aiScene* scene, uint32_t base_bone);
    void load_keyframes_from_channel(aiNodeAnim* channel, double ticks_per_second);
    glm::vec3 interpolate_position(aiNodeAnim* channel, double time);
    glm::quat interpolate_rotation(aiNodeAnim* channel, double time);
    glm::vec3 interpolate_scale(aiNodeAnim* channel, double time);
    uint32_t find_bone_index(const std::string& bone_name, uint32_t base_bone);

    Model_Indirect get_model_ind(uint32_t idx);
    Animated_Model get_animated_model(uint32_t idx);
    Util::AABB get_aabb_indirect(const model_handle& model_id);

    void setup_buffers();
    //void upload_data();

    void setup_ssbos(); // todo rm everything below
    bool is_bone_name(const std::string& name, uint32_t base_bone);
    aiNode* find_node_by_name(aiNode* node, const std::string& name);
    uint32_t find_parent_bone_index(const std::string& bone_name, const aiScene * scene, uint32_t base_bone);
    void update_bone_parents(const aiScene* scene, uint32_t base_bone, uint32_t end_bone);
    void add_leaf_bones(uint32_t base_bone, uint32_t end_bone);

    void update_bones_from_animation_compute(uint32_t animation_index, float time);

    uint32_t get_bone_ssbo();
    uint32_t get_skinned_bone_ssbo();
    uint32_t get_num_animated_models();
    uint32_t get_animation_command_ssbo();

    uint32_t get_big_vao();
    uint32_t get_rigged_vao();
}
