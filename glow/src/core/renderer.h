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

// todo move to light class maybe, maybe not
struct Cluster {
    glm::vec4 minPoint; // 16 bytes
    glm::vec4 maxPoint; // 16 (32)
    uint32_t count;     // 4 (36)
    uint32_t* lightIndices[99]; // 396 (432 / 16 = 27)
};

struct Draw_Elements_Indirect_Command {
    uint32_t  count;
    uint32_t  instance_count;
    uint32_t  first_index;
    int       base_vertex;
    uint32_t  base_instance;
};

enum Render_Pass {
    opaque_scene = 0, // depth pre pass can use these
    blended_scene,
    opaque_holding,
    blended_holding,
    CSM,
    hud,
    NUM_RENDER_PASSES
};

// todo add some kind of state for this!?
struct Render_Command {
    Draw_Elements_Indirect_Command command;
    Per_Object_Data object_data;
    Render_Pass pass;
    // shader
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    void resize(const int width, const int height);

    int init();

    bool setup_buffers();
    void setup_ssao();

    void init_shaders(); // todo!

    void setup_indirect(); // todo move to other func ^
    //void build_command_buffer(Player& player, Scene& scene, float delta_time); // todo scene maybe const
    void indirect_depth_prepass(const glm::mat4& viewproj);
    void sort_blended_draws();

    void build_cluster_pass(const glm::mat4& inv_proj);
    void cull_cluster_pass(const glm::mat4& view);

    void shadow_setup(const glm::mat4& view, const glm::mat4& inv_view, const float& aspect_ratio, const float& zoom);
    void shadow_pass(Scene& scene);

    // todo grab random stuff from
    //void render(Player& player, Scene& scene, float delta_time, SSBO& particles); // todo rm

    void particle_pass(float delta_time, SSBO& particle_ssbo, const glm::mat4& proj, const glm::mat4& view);
    
    void draw_light_quads(const glm::mat4& proj, const glm::mat4& view);
    void bloom_pass();
    void ssao_pass(const glm::mat4& proj, const glm::mat4& inv_proj);
    void composite();

    void render_debug(const glm::mat4& view, const glm::mat4& proj);

    void render_skybox(const Skybox& skybox, const glm::mat4& view, const glm::mat4& projection);

    void render_crosshair(const Crosshair& crosshair);
    void render_hud_text(const Text& text);

    void imgui_pass();

    void shutdown();

    void begin_frame();
    void submit_render_command(Draw_Elements_Indirect_Command draw_command, const Per_Object_Data object_data, const Blend_Mode blend_mode, const glm::vec3 view_pos, const Util::AABB aabb);
    void submit_animated_render_command(Draw_Elements_Indirect_Command draw_command, const Per_Object_Data object_data);
    void upload_render_commands();

    void submit_shadow_command(Draw_Elements_Indirect_Command draw_command, Per_Object_Data object_data, Blend_Mode blend_mode);
    // todo probably move CSM to some kind of light system along with other lights
    int get_cascade_level(const Entity& entity, glm::mat4 view); // returns which cascade an object belongs to, -1 if no cascade

    void draw(Scene& scene, const glm::mat4& view, const glm::mat4& viewproj, const glm::vec3& view_pos, const glm::mat4& proj);

    void compute_cull_draw(Scene& scene, const glm::vec3& view_pos, const glm::mat4& view, const glm::mat4& viewproj, const glm::mat4& cull_view, const glm::mat4& cull_proj);

    void debug_cascades();

    glm::vec4 normalize_plane(glm::vec4 p);

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

    uint32_t opaque_draw_command_ssbo, opaque_object_ssbo;
    std::vector<Draw_Elements_Indirect_Command> opaque_draw_commands;
    std::vector<Per_Object_Data> opaque_object_data;
    uint32_t opaque_draw_count;

    // todo
    uint32_t blended_draw_command_ssbo, blended_object_ssbo;
    std::vector<Draw_Elements_Indirect_Command> blended_draw_commands;
    std::vector<Per_Object_Data> blended_object_data;
    std::vector<uint32_t> blended_draw_command_indices;
    std::vector<float> blended_draw_command_distances;
    uint32_t blended_draw_count;

    uint32_t skinned_draw_commands_ssbo, skinned_object_ssbo;
    std::vector<Draw_Elements_Indirect_Command> skinned_draw_commands;
    std::vector<Per_Object_Data> skinned_object_data;
    uint32_t skinned_draw_count;
    // todo maybe blended skinned draws

    uint32_t compute_culled_commands, num_compute_culled_commands;

    uint32_t quadVAO;

    std::vector<glm::vec3> samples;
};
