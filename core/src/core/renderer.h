#pragma once

#include "glm/glm.hpp"
#include <glm/gtc/quaternion.hpp>

#include "renderer_debug.h"
#include "scene.h"
#include "light.h"
#include "core/ssbo.h"

#include "asset/compute_shader.h"
#include "asset/crosshair.h"
#include "asset/shader.h"
#include "asset/shader_manager.h"
#include "asset/text.h"

#include "player/player.h"

// todo move to light class maybe, maybe not
struct Cluster {
    glm::vec4 minPoint; // 16 bytes
    glm::vec4 maxPoint; // 16 (32)
    uint32_t count;     // 4 (36)
    uint32_t* lightIndices[199]; // 396 (432 / 16 = 27)
};

struct Draw_Elements_Indirect_Command {
    uint32_t  count;
    uint32_t  instance_count;
    uint32_t  first_index;
    int       base_vertex;
    uint32_t  base_instance;
};

struct Per_Object_Data {
    glm::mat4 model_matrix; // 64
    glm::mat4 normal_matrix; // 64
    glm::vec4 base_color;
    glm::vec4 emissive_factor; // 16

    uint64_t albedo; // 8
    uint64_t normal; // 8    
    uint64_t met_rough; // 8
    uint64_t emissive; // 8
    uint64_t amb_occ;
    uint64_t padding;

    float alpha_cutoff;
    float metallic_factor; // 4
    float roughness_factor; // 4
    uint32_t bone_offset;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    int init();
    void resize(const int width, const int height);

    bool setup_buffers();
    void setup_ssao();

    void setup_indirect();
    void build_command_buffer(Player& player, Scene& scene, float delta_time); // todo scene maybe const
    void indirect_depth_prepass(Player& player);
    void render_indirect(Player& player, Scene& scene);
    void sort_blended_draws();

    void build_cluster_pass(Player& player);
    void cull_cluster_pass(Player& player);
    void shadow_setup(const Player& player);
    void shadow_pass(Scene& scene, const Player& player);
    void render(Player& player, Scene& scene, float delta_time, SSBO& particles);
    void particle_pass(float delta_time, SSBO& particle_ssbo, Player& player);
    void draw_light_quads(Player& player); // debug
    void bloom_pass();
    void ssao_pass(Player& player);
    void composite();
    void render_debug(Player& player);

    void render_skybox(const Skybox& skybox, const glm::mat4& view, const glm::mat4& projection);

    void render_crosshair(const Crosshair& crosshair);
    void render_hud_text(const Text& text);

    void imgui_pass();

    void shutdown();


// private:

    int scr_width = 1600, scr_height = 900;
    Renderer_Debug debug_renderer;

    Light spotlight, directional_light, point_light;
    std::vector<GPU_Light> lights;
    SSBO light_ssbo; // todo maybe remove ssbo class? raw code not bad
    SSBO cluster_ssbo;

    float penis = 25.0f;
    float close_plane = 0.5f;
    float ambient_light = 0.01f;
    float sun_strength = 0.5f;

    bool use_alpha_clipping = true;
    bool shadows_enabled = false;
    bool bloom_enabled = true;
    float alpha_cutoff = 0.5f;
    int num_lights = 4;
    bool forward_plus = true;
    
    bool ssao_enabled = true;
    float ssao_radius = 0.5;
    float ssao_bias = 0.025;
    int ssao_samples = 64;
    float min_depth = 0.0001;
    float power = 1.2;

    bool use_depth_prepass = false;
    bool do_draw_light_quads = false;

    // deferred pipeline
    //Shader deferred_shader, deferred_lighting_shader, debug_gbuffer_shader;
    //uint32_t g_buffer, g_position, g_normal, g_albedo_specular;

    uint32_t render_target, render_depth_buffer;
    texture_handle depth_texture, scene_texture, bright_texture, ssao_texture, ssao_noise_texture;
    shader_handle quad_shader;

    uint32_t csm_fbo;
    texture_handle csm_texture;
    std::vector<std::vector<Draw_Elements_Indirect_Command>> csm_draw_commands;
    std::vector<std::vector<Per_Object_Data>> csm_per_object_data;

    glm::vec3 emitter_position = glm::vec3(0.0f, 25.0f, 0.0f);
    glm::vec3 acceleration_direction = glm::vec3(0.0f, 1.0f, 0.0f);
    float acceleration_force = 9.8f;
    
    glm::vec2 life_range = glm::vec2(3.0f, 6.0f);
    glm::vec4 color_start_base = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 color_end_base = glm::vec4(1.0, 1.0, 1.0f, 0.0f);
    glm::vec3 velocity_base = glm::vec3(0.0f);           // base spawn velocity
    glm::vec3 velocity_random_bias = glm::vec3(0.0f);
    float velocity_mag = 0.0f;

    float emission_rate = 1;
    int max_particles = 10000;
    ////////

    uint32_t draw_command_buffer, per_object_ssbo;
    std::vector<Draw_Elements_Indirect_Command> draw_commands;
    std::vector<Per_Object_Data> per_object_data;

    // todo
    uint32_t blended_draw_command_buffer, per_object_ssbo_blended;
    std::vector<Draw_Elements_Indirect_Command> draw_commands_blended;
    std::vector<Per_Object_Data> per_object_data_blended;
    std::vector<uint32_t> blended_draw_command_indices;
    std::vector<float> blended_draw_command_distances;

    uint32_t draw_command_buffer_skinned, per_object_ssbo_skinned;
    std::vector<Draw_Elements_Indirect_Command> draw_commands_skinned;
    std::vector<Per_Object_Data> per_object_data_skinned;
    uint32_t bone = 0;

    // todo maybe blended skinned draws

    uint32_t quadVAO;
};
