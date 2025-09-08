#pragma once

#include "core/entity.h"
#include "asset/skybox.h"
#include "core/terrain.h" // todo maybe an asset?

#include "core/opengl.h"

#include <vector>
#include <string>
#include <cstdint>

struct GPU_Entity { // todo can maybe just be pos if using BS not aabb
    glm::mat4 transform;
    uint32_t is_dirty;
    uint32_t animation_command_index;
    uint32_t any_mesh_visible;
    uint32_t padding;
};

struct GPU_Mesh {
    glm::mat4 transform;
    int32_t base_vertex;
    uint32_t vertex_count;
    uint32_t base_index;
    uint32_t index_count;
    glm::vec4 bounding_sphere; // bounding sphere pos, r
    uint32_t entity_index;
    uint32_t skinned_to_static_offset;
    uint32_t padding[2];
};

class Scene {
public:
    Scene();
    ~Scene();

    void init(const std::string& path);
    void include(Entity& ntitty);
    void upload_buffers();
    void update_dirty();
    // returns the number of hits
    //int cast_ray(const glm::vec3& pos, const glm::vec3& dir, glm::vec3& hit_pos);

    std::vector<Entity> entities;
    std::vector<Entity> timed_entities;
    Skybox skybox;
    Terrain terrain;

    uint32_t gpu_mesh_ssbo, gpu_entity_ssbo, per_mesh_ssbo, animated_mesh_to_all_mesh_mapping_ssbo;
    std::vector<GPU_Mesh> gpu_meshes;
    std::vector<GPU_Entity> gpu_entities;
    std::vector<Per_Object_Data> per_mesh_data;
    std::vector<uint32_t> animated_mesh_to_all_mesh_mapping;
};
