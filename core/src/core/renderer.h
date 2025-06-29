#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "renderer_debug.h"
#include "scene.h"
#include "light.h"

#include "asset/crosshair.h"
#include "asset/model.h"
#include "asset/shader.h"
#include "asset/shader_manager.h"
#include "asset/text.h"

#include "player/player.h"

#include "util/decompose.h"
#include "util/frustum.h"
#include "util/colors.h"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    int init();
    bool setup_buffers();

    void shadow_pass(Scene& scene, const Player& player);
    void render_scene(Player& player, Scene& scene, float delta_time);
    void render(Player& player, Scene& scene, float delta_time);
    void render_debug(Player& player);

    void render_skybox(const Skybox& skybox, const glm::mat4& view, const glm::mat4& projection);

    void render_crosshair(const Crosshair& crosshair);

    void render_hud_text(const Text& text);

    //todo rm
    void debug_sphere_at(float x, float y, float z);
    void debug_sphere_at(glm::vec3 pos);

    void shutdown();


// private:

    int scr_width = 1600, scr_height = 900;
    Renderer_Debug debug_renderer;

    Light spotlight, directional_light, point_light;

    shader_handle pbr_shader;
    shader_handle skybox_shader;
    shader_handle debug_shader;
    shader_handle shadow_map_shader, point_shadow_map_shader;
    shader_handle hud_text_shader;
    shader_handle crosshair_shader;

    shader_handle outline_shader;

    Shader toon;


    float penis = 25.0f;
    float ambient_light = 0.01f;

    bool use_alpha_clipping = true;
    float alpha_cutoff = 0.5f;

    // deferred pipeline
    Shader deferred_shader, deferred_lighting_shader, debug_gbuffer_shader;
    uint32_t g_buffer, g_position, g_normal, g_albedo_specular;
    uint32_t quadVAO;
};
