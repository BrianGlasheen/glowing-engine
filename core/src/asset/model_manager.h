#pragma once

#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/quaternion.hpp>

#include "shader.h"
#include "util/aabb.h"
#include "asset/model_indirect.h"

typedef uint32_t model_handle;

struct Position_Keyframe {
    glm::vec3 position;
    float time;
};
struct Rotation_Keyframe {
    glm::quat rotation;
    float time;
};
struct Scale_Keyframe{
    glm::vec3 scale;
    float time;
};

struct Bone_Animation {
    uint32_t bone_index; // bone this animation is for, maybe dont need if stored in flat array with bones
    float duration; // todo maybe rm
    std::vector<Position_Keyframe> position_keyframes;
    std::vector<Rotation_Keyframe> rotation_keyframes;
    std::vector<Scale_Keyframe> scale_keyframes;
};

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

    void load_animations_from_scene(const aiScene* scene, uint32_t base_bone);
    void load_keyframes_from_channel(aiNodeAnim* channel, Bone_Animation& bone_anim, double ticks_per_second);
    glm::vec3 interpolate_position(aiNodeAnim* channel, double time);
    glm::quat interpolate_rotation(aiNodeAnim* channel, double time);
    glm::vec3 interpolate_scale(aiNodeAnim* channel, double time);
    uint32_t find_bone_index(const std::string& bone_name, uint32_t base_bone);

    Model_Indirect get_model_ind(uint32_t idx);
    Model_Indirect get_skinned_model(uint32_t idx);
    Util::AABB get_aabb_indirect(const model_handle& model_id);

    void setup_buffers();
    //void upload_data();

    void setup_bone_ssbo(); // todo rm everything below
    bool is_bone_name(const std::string& name, uint32_t base_bone);
    aiNode* find_node_by_name(aiNode* node, const std::string& name);
    uint32_t find_parent_bone_index(const std::string& bone_name, const aiScene * scene, uint32_t base_bone);
    void update_bone_parents(const aiScene* scene, uint32_t base_bone, uint32_t end_bone);

    glm::vec3 sample_position_keyframes(const std::vector<Position_Keyframe>& keyframes, float time);
    glm::quat sample_rotation_keyframes(const std::vector<Rotation_Keyframe>& keyframes, float time);
    glm::vec3 sample_scale_keyframes(const std::vector<Scale_Keyframe>& keyframes, float time);

    glm::mat4 get_bone_local_transform_from_animation(uint32_t bone_index, uint32_t animation_index, float time);
    glm::mat4 get_bone_world_transform_naive(uint32_t bone_index, uint32_t animation_index, float time);
    void update_bones_from_animation(uint32_t animation_index, float time);

    uint32_t get_bone_ssbo();
    uint32_t get_big_vao();
    uint32_t get_rigged_vao();
}
