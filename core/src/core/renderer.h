#pragma once

#include <glm/glm.hpp>
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
    glm::mat4 model_matrix;
    glm::mat4 normal_matrix;
    glm::vec4 color;
    uint64_t albedo;
    uint64_t normal;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    int init();
    bool setup_buffers();

    void Renderer::setup_indirect();
    void Renderer::render_indirect(Player& player, float delta_time);

    void render_scene(Player& player, Scene& scene, float delta_time);
    void depth_prepass(Player& player, Scene& scene);
    void build_cluster_pass(Player& player);
    void cull_cluster_pass(Player& player);
    void shadow_pass(Scene& scene, const Player& player);
    void render(Player& player, Scene& scene, float delta_time, SSBO& particles);
    void particle_pass(float delta_time, SSBO& particle_ssbo, Player& player);
    void bloom_pass();
    void composite();
    void render_debug(Player& player);

    void render_skybox(const Skybox& skybox, const glm::mat4& view, const glm::mat4& projection);

    void render_crosshair(const Crosshair& crosshair);
    void render_hud_text(const Text& text);

    //todo rm
    void debug_sphere_at(float x, float y, float z);
    void debug_sphere_at(glm::vec3 pos);

    void imgui_pass();

    void shutdown();


// private:

    int scr_width = 1600, scr_height = 900;
    Renderer_Debug debug_renderer;

    Light spotlight, directional_light, point_light;
    std::vector<GPU_Light> lights;
    SSBO light_ssbo; // todo maybe remove ssbo class? raw code not bad
    SSBO cluster_ssbo;
    Compute_Shader cluster_build, cluster_cull;
    shader_handle slice_vis;

    shader_handle pbr_shader;
    shader_handle skybox_shader;
    shader_handle debug_shader;
    shader_handle shadow_map_shader, point_shadow_map_shader;
    shader_handle hud_text_shader;
    shader_handle crosshair_shader;

    shader_handle outline_shader;

    Shader toon;


    float penis = 25.0f;
    float close_plane = 0.5f;
    float ambient_light = 0.01f;

    bool use_alpha_clipping = true;
    bool shadows_enabled = false;
    float alpha_cutoff = 0.5f;
    int num_lights = 50;
    bool forward_plus = true;
    bool indirect_rendering = true;

    bool use_depth_prepass = false;
    shader_handle depth_prepass_shader;

    // deferred pipeline
    //Shader deferred_shader, deferred_lighting_shader, debug_gbuffer_shader;
    //uint32_t g_buffer, g_position, g_normal, g_albedo_specular;

    uint32_t render_target, render_depth_buffer;
    texture_handle scene_texture, bright_texture;
    shader_handle quad_shader;

    // todo move particle stuff
    Compute_Shader bloom_down, bloom_up, particle;
    shader_handle particle_shader;

    glm::vec3 emitter_position = glm::vec3(0.0f, 25.0f, 0.0f);
    glm::vec3 acceleration_direction = glm::vec3(0.0f, 1.0f, 0.0f);
    float acceleration_force = 9.8f;
    
    glm::vec2 life_range = glm::vec2(3.0f, 6.0f);
    glm::vec4 color_start_base = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 color_end_base = glm::vec4(1.0, 1.0, 1.0f, 0.0f);
    glm::vec3 velocity_base = glm::vec3(0.0f);           // base spawn velocity
    glm::vec3 velocity_random_bias = glm::vec3(0.0f);
    float velocity_mag = 0.0f;

    float emission_rate = 5;
    int max_particles = 10000;

    uint32_t draw_command_buffer, per_object_ssbo;
    std::vector<Draw_Elements_Indirect_Command> draw_commands;
    std::vector<Per_Object_Data> per_object_data;
    shader_handle indirect_shader;

    uint32_t quadVAO;
};
