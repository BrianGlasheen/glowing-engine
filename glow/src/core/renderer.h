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


    int init();
    void resize(const int width, const int height);
    void shutdown();

    void setup();
    void setup_shaders();
    void setup_buffers();
    void setup_ssao();

    void begin_frame(Scene& scene, const glm::mat4& cull_view, const glm::mat4& cull_proj);

    // setting up stuff for rendering
    void build_cluster_pass(const glm::mat4& inv_proj);
    void cull_cluster_pass(const glm::mat4& view);
    void shadow_setup(const glm::mat4& view, const glm::mat4& inv_view, const float& aspect_ratio, const float& zoom);

    // rendering
    void depth_prepass(const glm::mat4& viewproj);
    void shadow_pass(Scene& scene);
    void draw(Scene& scene, const glm::vec3& view_pos, const glm::mat4& view, const glm::mat4& viewproj, const glm::mat4& cull_view, const glm::mat4& cull_proj, bool wireframe);
    void particle_pass(float delta_time, const glm::mat4& proj, const glm::mat4& view);
    void render_skybox(const Skybox& skybox, const glm::mat4& view, const glm::mat4& projection);

    // rendering 2d / screenspace
    void render_crosshair(const Crosshair& crosshair);
    void render_hud_text(const Text& text);

    // post processing
    void bloom_pass();
    void ssao_pass(const glm::mat4& proj, const glm::mat4& inv_proj);
    void composite();

    // debug
    void debug_cascades(Scene& scene);
    void draw_light_quads(const glm::mat4& proj, const glm::mat4& view);
    void render_debug(const glm::mat4& view, const glm::mat4& proj, Scene& scene);
    void debug_skeletons(Scene& scene, const glm::mat4& vp);

    // utility
    void imgui_pass();
    glm::vec4 normalize_plane(glm::vec4 p);

// private:
    int scr_width = 1600, scr_height = 900;
    Renderer_Debug debug_renderer;

    //Light spotlight, directional_light, point_light;
    std::vector<GPU_Light> lights;
    SSBO light_ssbo; // todo maybe remove ssbo class? raw code not bad

    //float ambient_light = 0.01f;
    float sun_strength = 0.5f;

    bool use_alpha_clipping = true;
    bool shadows_enabled = false;
    bool bloom_enabled = true;
    float alpha_cutoff = 0.5f;
    int num_lights = 4;
    bool forward_plus = true;
    
    bool ssao_enabled = false;
    float ssao_radius = 0.5;
    float ssao_bias = 0.025;
    int ssao_samples = 64;
    float min_depth = 0.0001;
    float power = 1.2;

    bool use_depth_prepass = false;
    bool do_draw_light_quads = false;
    bool draw_skeletons = true;
    bool cascade_vis = false;
    uint32_t terrain_draw_type = 0;

    // deferred pipeline
    //Shader deferred_shader, deferred_lighting_shader, debug_gbuffer_shader;
    //uint32_t g_buffer, g_position, g_normal, g_albedo_specular;
    SSBO cluster_ssbo;

    uint32_t render_target, render_depth_buffer;
    texture_handle depth_texture, scene_texture, bright_texture, ssao_texture, ssao_noise_texture;

    uint32_t csm_fbo;
    texture_handle csm_texture;

    // buffers for compute culling
    uint32_t opaque_draw_commands, blended_draw_commands, csm_draw_commands;
    uint32_t num_commands;

    uint32_t quadVAO;

    std::vector<glm::vec3> samples;
};
