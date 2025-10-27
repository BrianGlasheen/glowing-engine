#pragma once

#include "scene/entity.h"
#include "scene/skybox.h"
#include "scene/terrain.h" // todo maybe an asset?
#include "scene/light.h"
#include "util/math.h"

#include "core/opengl.h"

#include <vector>
#include <string>
#include <cstdint>

const uint32_t NUM_CASCADE = 4;

struct GPU_Entity { // todo can maybe just be pos if using BS not aabb
    mat4 transform;
    uint32_t is_dirty;
    uint32_t animation_command_index;
    uint32_t any_mesh_visible;
    uint32_t padding;
};

struct GPU_Mesh {
    mat4 transform;
    int32_t base_vertex;
    uint32_t vertex_count;
    uint32_t base_index;
    uint32_t index_count;
    vec4 bounding_sphere; // bounding sphere pos, r
    uint32_t entity_index;
    uint32_t skinned_to_static_offset;
    uint32_t bone_offset;
    uint32_t transparent;
};

class Scene {
public:
    Scene();
    ~Scene();

    void init(const std::string& path);
    void create_buffers();
    
    void include(Entity& ntitty);
    void add_entity_to_gpu_buffers(Entity& ntitty);
    void refresh();

    void upload_buffers();
    void update_dirty();

    void serialize(std::string path = "../resources/scenes/scene.yaml");
    void load_from_file(std::string path = "../resources/scenes/scene.yaml");
    // returns the number of hits
    //int cast_ray(const vec3& pos, const vec3& dir, vec3& hit_pos);

    // gloabl scene stuff
    Skybox skybox;
    float cascade_ends[NUM_CASCADE + 1] = { -5.0f, 25.0f, 100.0f, 350.0f, 1000.0f }; // same
    vec3 sun_direction = normalize(vec3(0.0, -1.0f, -1.0f)); // todo this belongs to scene
    vec3 sun_color = vec3(1.0f);
    float sun_strength = 0.5f;
    
    std::vector<Entity> entities;
    std::vector<Entity> timed_entities;
    // std::vector<Light> lights; todo!!
    // cameras?
    // player(s)
    // probes (reflection & gi)
    // triggers
    // terrain(s?)
    Terrain terrain;


// private?
    uint32_t gpu_mesh_ssbo, gpu_entity_ssbo, per_mesh_ssbo, animated_mesh_to_all_mesh_mapping_ssbo;
    std::vector<GPU_Mesh> gpu_meshes;
    std::vector<GPU_Entity> gpu_entities;
    std::vector<Per_Object_Data> per_mesh_data;
    std::vector<uint32_t> animated_mesh_to_all_mesh_mapping;
};
