#pragma once

#include <filesystem>
#include <ctime>
#include <cfloat>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <dearimgui/imgui.h>
#include <dearimgui/imgui_impl_glfw.h>
#include <dearimgui/imgui_impl_opengl3.h>
#include <imguizmo/ImGuizmo.h>

#include "renderer_debug.h"
#include "scene.h"
#include "light.h"
#include "asset/shader.h"
#include "asset/model.h"
#include "asset/crosshair.h"
#include "asset/shader_manager.h"
#include "asset/text.h"
#include "player/player.h"
#include "util/decompose.h"
#include "util/frustum.h"
#include "util/colors.h"

const float FAR_PLANE = 500.0f;


// point light shadow mapping
struct camera_dir {
    GLenum face;
    glm::vec3 direction;
    glm::vec3 up;
};
static camera_dir camera_directions[] = {
    { GL_TEXTURE_CUBE_MAP_POSITIVE_X, glm::vec3(1.0f, 0.0f, 0.0f),  glm::vec3(0.0f, 1.0f, 0.0f) },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, glm::vec3(0.0f, 1.0f, 0.0f),  glm::vec3(0.0f, 0.0f, -1.0f) },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, glm::vec3(0.0f, 0.0f, 1.0f),  glm::vec3(0.0f, 1.0f, 0.0f) },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f) }
};
// point light shadow mapping


enum class ortho_view {
    TOP_DOWN = 0,
    FRONT,
    SIDE,
    SCENE
};

enum gizmo_modes {
    NONE = 0,
    TRANSLATE,
    ROTATE,
    SCALE
};
static std::string gize_mode_strs[]{"none", "translate", "rotate", "scale"};

struct ortho_view_data {
    ortho_view type;

    // TODO make not shit
    Font FIX_font;

    // Camera positioning
    float zoom_level;           // Orthographic size multiplier (smaller = more zoomed in)
    glm::vec2 pan_offset;       // X/Y offset for panning in view space
    float camera_distance;      // Distance from target point

    // View bounds and limits
    float min_zoom;            // Minimum zoom level (max zoom in)
    float max_zoom;            // Maximum zoom level (max zoom out)
    float zoom_speed;          // How fast zoom responds to input
    float pan_speed;           // How fast panning responds to input
    glm::vec2 pan_limits;      // Maximum pan distance from center

    // Input state
    bool is_panning;           // Currently panning with mouse
    glm::vec2 last_mouse_pos;  // Last mouse position for delta calculation
    bool is_zooming;           // Currently zooming
    gizmo_modes gizmo_mode;

    // Visual settings
    bool show_grid;            // Show grid overlay
    float grid_size;           // Grid cell size in world units
    glm::vec3 grid_color;      // Grid line color
    bool show_axes;            // Show world axes
    bool show_bounds;          // Show scene bounds
    Text view_text;

    ortho_view_data(ortho_view view_type = ortho_view::TOP_DOWN)
        : type(view_type)
        , zoom_level(1.0f)
        , pan_offset(0.0f, 0.0f)
        , camera_distance(50.0f)
        , min_zoom(0.1f)
        , max_zoom(10.0f)
        , zoom_speed(0.1f)
        , pan_speed(0.1f)
        , pan_limits(100.0f, 100.0f)
        , is_panning(false)
        , last_mouse_pos(0.0f, 0.0f)
        , is_zooming(false)
        , gizmo_mode(gizmo_modes::NONE)
        , show_grid(true)
        , grid_size(1.0f)
        , grid_color(0.3f, 0.3f, 0.3f)
        , show_axes(true)
        , show_bounds(false)
    {
    }

    void init_text(std::string text) {
        FIX_font = Font("tx02");
        view_text.load(FIX_font, text, 0, 1, 100.0f, glm::vec3(1.0f));
    }

    // Calculate the actual orthographic size based on zoom
    float get_ortho_size() const {
        return 20.0f * zoom_level; // Base size * zoom multiplier
    }

    glm::vec3 get_camera_position() const {
        switch (type) {
        case ortho_view::TOP_DOWN:
            return glm::vec3(0.0f, camera_distance, 0.0f);
        case ortho_view::FRONT:
            return glm::vec3(0.0f, 0.0f, camera_distance);
        case ortho_view::SIDE:
            return glm::vec3(camera_distance, 0.0f, 0.0f);
        default:
            assert(false);
        }
    }

    glm::vec3 get_target_position() const {
        glm::vec3 target = glm::vec3(0.0f);

        switch (type) {
        case ortho_view::TOP_DOWN:
            target.x += pan_offset.x;
            target.z += pan_offset.y;
            break;
        case ortho_view::FRONT:
            target.x += pan_offset.x;
            target.y += pan_offset.y;
            break;
        case ortho_view::SIDE:
            target.z += pan_offset.x;
            target.y += pan_offset.y;
            break;
        }

        return target;
    }

    glm::vec3 get_up_vector() const {
        switch (type) {
        case ortho_view::TOP_DOWN:
            return glm::vec3(0.0f, 0.0f, -1.0f);
        case ortho_view::FRONT:
        case ortho_view::SIDE:
            return glm::vec3(0.0f, 1.0f, 0.0f);
        default:
            return glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    void handle_zoom(float zoom_delta) {
        if (zoom_delta != 0.0f) {
            is_zooming = true;
            zoom_level = glm::clamp(zoom_level + zoom_delta * zoom_speed, min_zoom, max_zoom);
        }
        else {
            is_zooming = false;
        }
    }

    // Handle pan input
    void handle_pan(const glm::vec2& mouse_delta) {
        if (is_panning) {
            glm::vec2 pan_delta = mouse_delta * pan_speed * zoom_level;
            
            switch (type) {
                case ortho_view::TOP_DOWN:
                    pan_delta *= -1;
                    break;
                case ortho_view::FRONT:
                    pan_delta.x *= -1;
                    break;
                case ortho_view::SIDE:
                    //pan_delta.y *= -1;
                    break;
                default:
                    assert(false);
            }
            pan_offset.x = glm::clamp(pan_offset.x + pan_delta.x, -pan_limits.x, pan_limits.x);
            pan_offset.y = glm::clamp(pan_offset.y + pan_delta.y, -pan_limits.y, pan_limits.y);
        }
    }

    // Start panning
    void start_pan(const glm::vec2& mouse_pos) {
        is_panning = true;
        last_mouse_pos = mouse_pos;
    }

    // Stop panning
    void stop_pan() {
        is_panning = false;
    }

    void set_gizmo_mode(gizmo_modes gm) {
        gizmo_mode = gm;
        view_text.updateText(gize_mode_strs[gm]);
    }
};

struct editor_viewports_struct {
    ortho_view_data top;
    ortho_view_data side;
    ortho_view_data front;
    ortho_view_data scene;

    editor_viewports_struct()
        : top(ortho_view::TOP_DOWN)
        , side(ortho_view::SIDE)
        , front(ortho_view::FRONT)
        , scene(ortho_view::SCENE)
    {
    }
};

class Renderer {
public:
    Renderer(){};
    ~Renderer(){};

    bool init() {

        // make viewports
        editor_viewports.top.init_text  ("top------"); // pad to 9 xD
        editor_viewports.front.init_text("front----");
        editor_viewports.side.init_text ("side-----");
        editor_viewports.scene.init_text("scene----");

        // configure global opengl state
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        // LIGHTs
        // makes opengl calls
        spotlight = Light::create_spot(glm::vec3(0.0f, 5.0f, -5.0f), glm::vec3(0.0f, -1.0f, -0.5f), glm::vec3(1.0f), 15.0f, 25.0f, 45.0f, 1024, 1024);
        directional_light = Light::create_directional(glm::vec3(0.0f, -0.25f, 0.25f), glm::vec3(1.0f), 0.1f);
        point_light = Light::create_point(glm::vec3(0.0f, -5.0f, 0.0f), glm::vec3(1.0f), 1.0f, 1024);

        // SHADERS
        Shader_Manager::init("../resources/shaders/");

        pbr_shader = Shader_Manager::load_from_paths("pbr", "vertex.glsl", "fragment.glsl");
        skybox_shader = Shader_Manager::load_from_name("skybox");
        debug_shader = Shader_Manager::load_from_name("debug");
        editor_shader = Shader_Manager::load_from_name("editor");
        shadow_map_shader = Shader_Manager::load_from_name("shadow_map");
        point_shadow_map_shader = Shader_Manager::load_from_name("shadow_map_point");
        hud_text_shader = Shader_Manager::load_from_name("text_hud");
        //debug_shader.init("../resources/shaders/debug_v.glsl", "../resources/shaders/debug_f.glsl");
        outline_shader = Shader_Manager::load_from_name("outline");
        
        //setup_buffers(); // defferd g buffer setup
        //deferred_shader.init("../resources/shaders/deferred_v.glsl", "../resources/shaders/deferred_f.glsl");
        //deferred_lighting_shader.init("../resources/shaders/deferred_light_v.glsl", "../resources/shaders/deferred_light_f.glsl");
        //debug_gbuffer_shader.init("../resources/shaders/deferred_light_v.glsl", "../resources/shaders/deferred_lighting_debug_f.glsl");

        crosshair_shader = Shader_Manager::load_from_name("crosshair");

        //toon.init("../resources/shaders/vertex.glsl", "../resources/shaders/toon.glsl");

        debug_renderer.init();

        return true;
    }



    bool setup_buffers() {
        // Create and bind G-buffer framebuffer
        glGenFramebuffers(1, &g_buffer);
        glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
    
        // 1. Position buffer
        glGenTextures(1, &g_position);
        glBindTexture(GL_TEXTURE_2D, g_position);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scr_width, scr_height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_position, 0);
        
        // 2. Normal buffer
        glGenTextures(1, &g_normal);
        glBindTexture(GL_TEXTURE_2D, g_normal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scr_width, scr_height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_normal, 0);
        
        // 3. Color + specular buffer
        glGenTextures(1, &g_albedo_specular);
        glBindTexture(GL_TEXTURE_2D, g_albedo_specular);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scr_width, scr_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g_albedo_specular, 0);
        
        // Tell OpenGL which color attachments we'll use for rendering
        GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers(3, attachments);
        
        // Create and attach depth buffer
        GLuint rboDepth;
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, scr_width, scr_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
        
        // Check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Framebuffer not complete!" << std::endl;
            return false;
        }
        
        // Unbind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GLuint quadVBO;
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        
        // Print debug info
        printf("Quad VAO created: %u\n", quadVAO);
        
        return true;
    }

    //void draw_player_model(Player& player, Model& player_model) {
    //    Shader* shader = Shader_Manager::get_shader(pbr_shader);
    //    shader->use();

    //    shader->set_vec3("lightPos", glm::vec3(2.0f, 2.0f, 2.0f));
    //    shader->set_vec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

    //    shader->set_vec3("viewPos", player.camera.position);

    //    glm::mat4 projection = glm::perspective(glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
    //    shader->set_mat4("projection", projection);
    //    glm::mat4 view = player.camera.get_view_matrix();
    //    shader->set_mat4("view", view);

    //    glm::mat4 model = player.get_model_matrix();
    //    shader->set_mat4("model", model);
    //
    //    shader->set_vec3("objectColor", glm::vec3(0.0f, 0.5f, 0.0f));
    //
    //    player_model.draw(shader);
    //}

    void shadow_pass(Scene& scene, const Player& player) {
        spotlight.bind_fbo_write();
        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);

        // use shadow shader
        Shader* shader = Shader_Manager::get_shader(shadow_map_shader);
        shader->use();
        glm::mat4 projection = glm::perspective(glm::radians(spotlight.outer_fov * 2.0f), (float)spotlight.width / (float)spotlight.height, 0.1f, 50.0f);
        if (player.key_toggles['l'])
            debug_renderer.draw_frustum(spotlight.position, spotlight.direction, glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(spotlight.outer_fov * 2.0f), (float)spotlight.width / (float)spotlight.height, 0.1f, 50.0f);

        shader->set_mat4("projection", projection);
        glm::mat4 view = glm::lookAt(spotlight.position, spotlight.position + spotlight.direction, glm::vec3(0.0f, 1.0f, 0.0f));
        shader->set_mat4("view", view);

        Util::Frustum frustum(spotlight.position, spotlight.direction, glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(spotlight.outer_fov * 2.0f), (float)spotlight.width / (float)spotlight.height, 0.1f, 50.0f);
        // frusutm cull objects + check move?
        for (Entity& entity : scene.entities) {
            if (entity.physics_enabled) {
                Util::AABB box = Physics::get_world_AABB(entity.physics_id);
                if (frustum.intersectsAABB(box.min, box.max)) {
                    glm::mat4 model = entity.get_model_matrix();
                    shader->set_mat4("model", model);

                    bool shadow_pass = true;
                    entity.draw(shader, shadow_pass);

                }
            }
        }
        // dir light, maybe scene BB
        directional_light.bind_fbo_write();
        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);

        float scene_size = 50.0f;
        float light_distance = 50.0f;
        glm::vec3 scene_center = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 light_pos = scene_center - directional_light.direction * light_distance;
        glm::mat4 dir_projection = glm::ortho(-scene_size, scene_size, -scene_size, scene_size, 0.1f, 150.0f);
        shader->set_mat4("projection", dir_projection);
        glm::mat4 dir_view = glm::lookAt(light_pos, light_pos + directional_light.direction, glm::vec3(0.0f, 1.0f, 0.0f));
        shader->set_mat4("view", dir_view);

        // frusutm cull objects + check move?
        for (Entity& entity : scene.entities) {
            glm::mat4 model = entity.get_model_matrix();
            shader->set_mat4("model", model);

            bool shadow_pass = true;
            entity.draw(shader, shadow_pass);
        }

        // point light shadow mapping
        // frustum culling stuff

        shader = Shader_Manager::get_shader(point_shadow_map_shader);
        shader->use();

        glEnable(GL_DEPTH_TEST);
        glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

        projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, penis);
        shader->set_mat4("projection", projection);

        shader->set_vec3("point_light_position", point_light.position);
        //shader->set_float("point_light_far_plane", 25.0f);

        for (size_t i = 0; i < 6; i++) {
            point_light.bind_cubemap_face_write(camera_directions[i].face);
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            glm::vec3 target = point_light.position + camera_directions[i].direction;
            view = glm::lookAt(point_light.position, target, camera_directions[i].up);
            shader->set_mat4("view", view);

            if (player.key_toggles['l'])
                debug_renderer.draw_frustum(point_light.position, camera_directions[i].direction, camera_directions[i].up, glm::radians(90.0f), 1.0f, 0.01f, penis);

            Util::Frustum frustum2(point_light.position, camera_directions[i].direction, camera_directions[i].up, glm::radians(90.0f), 1.0f, 0.01f, penis);
            
            for (Entity& entity : scene.entities) {
                if (entity.physics_enabled) {
                    Util::AABB box = Physics::get_world_AABB(entity.physics_id);
                    if (frustum2.intersectsAABB(box.min, box.max)) {

                        glm::mat4 model = entity.get_model_matrix();
                        shader->set_mat4("model", model);

                        bool shadow_pass = true;
                        entity.draw(shader, shadow_pass);
                    }
                }
            }
        }

    }

    void render(Player& player, Scene& scene, float delta_time) {

        if (editor_mode) {
            render_scene_editor(player, scene, delta_time);
        }
        else {
            shadow_pass(scene, player);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, scr_width, scr_height);

            render_scene(player, scene, delta_time);
        }
    }

    void render_scene(Player& player, Scene& scene, float delta_time) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        

        glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE * (player.out_of_body ? 1.5f : 1.0f));
        glm::mat4 view = player.get_view_matrix();
        render_skybox(scene.skybox, view, projection); // not efficient but gets outlines ontop of skybox


        //Shader used_shader = toon;
        Shader* shader = Shader_Manager::get_shader(pbr_shader);
        shader->use();
        glStencilMask(0x00);

        shader->set_bool("use_alpha_clipping", use_alpha_clipping);
        shader->set_float("alpha_cutoff", alpha_cutoff);

        // spotlight
        spotlight.bind_fbo_read(3);
        shader->set_int("shadow_map", 3);
        glm::mat4 lprojection = glm::perspective(glm::radians(spotlight.outer_fov * 2.0f), (float)spotlight.width / (float)spotlight.height, 0.1f, 50.0f);
        shader->set_mat4("light_projection", lprojection);
        glm::mat4 lview = glm::lookAt(spotlight.position, spotlight.position + spotlight.direction, glm::vec3(0.0f, 1.0f, 0.0f));
        shader->set_mat4("light_view", lview);

        shader->set_vec3("spot_light_position", spotlight.position);
        shader->set_vec3("spot_light_direction", spotlight.direction);
        shader->set_vec3("spot_light_color", spotlight.color);
        shader->set_float("spot_light_intensity", spotlight.intensity);
        shader->set_float("spot_light_inner_cone", glm::cos(glm::radians(spotlight.inner_fov)));
        shader->set_float("spot_light_outer_cone", glm::cos(glm::radians(spotlight.outer_fov)));
        debug_renderer.add_sphere(spotlight.position, 0.1f, spotlight.color);
        debug_renderer.add_line(spotlight.position, spotlight.position + spotlight.direction, spotlight.color);

        // dir light
        directional_light.bind_fbo_read(4);
        shader->set_int("directional_shadow_map", 4);
        float scene_size = 50.0f;
        float light_distance = 50.0f;
        glm::vec3 scene_center = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 light_pos = scene_center - directional_light.direction * light_distance;
        glm::mat4 dir_projection = glm::ortho(-scene_size, scene_size, -scene_size, scene_size, 0.1f, 150.0f);
        shader->set_mat4("dir_light_projection", dir_projection);
        glm::mat4 dir_view = glm::lookAt(light_pos, light_pos + directional_light.direction, glm::vec3(0.0f, 1.0f, 0.0f));
        shader->set_mat4("dir_light_view", dir_view);

        shader->set_vec3("directional_light_direction", directional_light.direction);
        shader->set_vec3("directional_light_color", directional_light.color);
        shader->set_float("directional_light_intensity", directional_light.intensity);
        debug_renderer.add_line(glm::vec3(0.0f, 10.f, 0.0f), glm::vec3(0.0f, 10.f, 0.0f) + directional_light.direction, spotlight.color);
        ///////////

        shader->set_vec3("point_light_position", point_light.position);
        shader->set_vec3("point_light_color", point_light.color);
        shader->set_float("point_light_intensity", point_light.intensity);
        shader->set_float("point_light_far_plane", 50.0f);
        debug_renderer.add_sphere(point_light.position, 0.1f, glm::vec3(1.0f));

        point_light.bind_fbo_read(5);
        shader->set_int("point_shadow_map", 5);


        shader->set_float("ambient_light", ambient_light);
        /////


        //glm::projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE * (player.out_of_body ? 1.5f : 1.0f));
        shader->set_mat4("projection", projection);
        
        //glm::mat4 view = player.get_view_matrix();
        shader->set_mat4("view", view);
        shader->set_vec3("view_position", player.get_view_position());

        if (player.out_of_body) {
            debug_renderer.add_sphere(player.camera.position, 1.0f, glm::vec3(1.0f));
            debug_renderer.draw_frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
        }

        Util::Frustum frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
        int count = 0;
        for (Entity& entity : scene.entities) {

            //if (&entity == &scene.entities[target_entity]) {
            //    count++;
            //    continue;
            //}   
            //else {
            //    glStencilMask(0x00);
            //}

            Util::AABB box;
            if (entity.physics_enabled) {
                box = Physics::get_world_AABB(entity.physics_id);
                if (frustum.intersectsAABB(box.min, box.max)) {
                    count++;

                    glm::mat4 model = entity.get_model_matrix();
                    shader->set_mat4("model", model);

                    glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
                    shader->set_mat3("normal_matrix", normal_matrix);
                    entity.draw(shader);
                }
            }
            else {
                count++;

                glm::mat4 model = entity.get_model_matrix();
                shader->set_mat4("model", model);

                glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
                shader->set_mat3("normal_matrix", normal_matrix);

                entity.draw(shader);
            }
            
            
            /////////////////////////////////////////////////////////////////////////////////////////////////
            //debug_renderer.add_axes(entity.get_physics_position(), entity.rotation);
            if (entity.physics_enabled) {
                if (player.out_of_body) {
                    debug_renderer.add_bbox(box.min, box.max, glm::vec3(0.0f, 0.0f, 1.0f));
                }
                else {
                    Util::OBB collision_box = Physics::get_world_OBB(entity.physics_id);
                    debug_renderer.add_obb(collision_box, glm::vec3(0.0f, 1.0f, 0.0f)); // Green for physics collision box
                }
            }
        }
        //printf("drawing %d entities\n", count);

        //if (editor_mode)
            render_selected_outlined(player, scene);


        // flush(); !!
    }


    void render_selected_outlined(Player& player, const Scene& scene) {

        for (size_t selected : selected_entites) {
            glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Entity selected_entity = scene.entities[selected];

            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

            Shader* shader = Shader_Manager::get_shader(outline_shader);
            shader->use();

            glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
            shader->set_mat4("projection", projection);
            shader->set_mat4("view", player.get_view_matrix());

            glm::mat4 model = selected_entity.get_model_matrix();
            shader->set_mat4("model", model);
            //glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
            //shader->set_mat3("normal_matrix", normal_matrix);
            shader->set_float("scale", 0.0);
            selected_entity.draw(shader);

            shader->set_vec3("color", Util::orange);
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            //glDisable(GL_DEPTH_TEST);
            //glEnable(GL_DEPTH_TEST);
            //glCullFace(GL_FRONT);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

            //shader->set_mat4("model", glm::scale(model, glm::vec3(outline_scale)));
            shader->set_float("scale", outline_scale);
            selected_entity.draw(shader);

            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            //glCullFace(GL_BACK);
            glEnable(GL_DEPTH_TEST);
        }

    }

    void render_scene_ortho(Player& player, Scene& scene, float deltaTime, const ortho_view_data& view_data) {
        Shader* shader = Shader_Manager::get_shader(editor_shader);
        shader->use();

        int half_width = scr_width / 2;
        int half_height = scr_height / 2;

        float aspect_ratio = (float)half_width / (float)half_height;
        float ortho_size = view_data.get_ortho_size();

        glm::mat4 projection = glm::ortho(
            -ortho_size * aspect_ratio, ortho_size * aspect_ratio,  // left, right
            -ortho_size, ortho_size,                                // bottom, top
            0.1f, FAR_PLANE                                         // near, far
        );

        shader->set_mat4("projection", projection);

        glm::vec3 target_pos = view_data.get_target_position();
        glm::vec3 view_camera_pos = target_pos + view_data.get_camera_position();
        glm::vec3 up_vector = view_data.get_up_vector();

        glm::mat4 view = glm::lookAt(view_camera_pos, target_pos, up_vector);

        shader->set_mat4("view", view);
        //used_shader.set_vec3("view_position", view_camera_pos);
        
        for (Entity& entity : scene.entities) {
            glm::mat4 model = entity.get_model_matrix();
            shader->set_mat4("model", model);

            glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
            shader->set_mat3("normal_matrix", normal_matrix);

            entity.draw(shader);

 /*           if (entity.physics_enabled) {
                Util::OBB collision_box = Physics::get_world_OBB(entity.physics_id);
                debug_renderer.add_obb(collision_box, glm::vec3(0.0f, 1.0f, 0.0f));
            }*/
        }
        
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        render_hud_text(view_data.view_text);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    void render_scene_editor(Player& player, Scene& scene, float delta_time) {
        int half_width = scr_width / 2;
        int half_height = scr_height / 2;
        //float quadrant_aspect_ratio = (float)half_width / (float)half_height;

        glViewport(0, half_height, half_width, half_height); // Top-left quadrant
        render_scene(player, scene, delta_time);
        //render_gizmo(scene, player, half_width, half_height);
        render_hud_text(editor_viewports.scene.view_text);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        // Top-Right
        glViewport(half_width, half_height, half_width, half_height);
        render_scene_ortho(player, scene, delta_time, editor_viewports.top);

        // Bottom-Left
        glViewport(0, 0, half_width, half_height);
        render_scene_ortho(player, scene, delta_time, editor_viewports.side);

        // Bottom-Right
        glViewport(half_width, 0, half_width, half_height);
        render_scene_ortho(player, scene, delta_time, editor_viewports.front);

        glViewport(0, 0, scr_width, scr_height);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    //void render_gizmo(const Scene& scene, const Player& player) {
    //    if (editor_viewports.scene.gizmo_mode != gizmo_modes::NONE && target_entity != -1) {
    //        float w = scr_width / 2;
    //        float h = scr_height / 2;

    //        ImGuizmo::BeginFrame();

    //        ImGui::SetNextWindowPos(ImVec2(0, 0));
    //        ImGui::SetNextWindowSize(ImVec2(w, h));
    //        ImGui::Begin("gizmode",
    //            nullptr,
    //            ImGuiWindowFlags_NoTitleBar |
    //            ImGuiWindowFlags_NoResize |
    //            ImGuiWindowFlags_NoMove |
    //            ImGuiWindowFlags_NoScrollbar |
    //            ImGuiWindowFlags_NoBackground);

    //        ImGuizmo::SetOrthographic(false);
    //        ImGuizmo::SetDrawlist();

    //        ImGuizmo::SetRect(0.0f, 0.0f, w, h);

    //        glm::mat4 projection = glm::perspective(glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
    //        glm::mat4 view = player.camera.get_view_matrix();
    //        glm::mat4 model = scene.entities[target_entity].get_model_matrix();

    //        ImGuizmo::OPERATION guizmo_op;
    //        if (editor_viewports.scene.gizmo_mode == gizmo_modes::TRANSLATE)
    //            guizmo_op = ImGuizmo::OPERATION::TRANSLATE;
    //        else if (editor_viewports.scene.gizmo_mode == gizmo_modes::ROTATE)
    //            guizmo_op = ImGuizmo::OPERATION::ROTATE;
    //        else if (editor_viewports.scene.gizmo_mode == gizmo_modes::SCALE)
    //            guizmo_op = ImGuizmo::OPERATION::SCALE;
    //        else
    //            assert(false);

    //        // no snap
    //        bool smooth = false;//glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    //        float snap_value = 0.5f;
    //        if (guizmo_op == ImGuizmo::OPERATION::ROTATE)
    //            snap_value = 15.0f;
    //        float snap_values[3] = { snap_value, snap_value, snap_value };

    //        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), guizmo_op, ImGuizmo::LOCAL,
    //            glm::value_ptr(model), nullptr, smooth ? nullptr : snap_values)) {

    //            glm::vec3 position, scale, rotation;
    //            Util::decompose(model, position, scale, rotation);

    //            // todo change
    //            Physics::set_body_position(scene.entities[target_entity].physics_id, position);
    //            Physics::set_body_rotation(scene.entities[target_entity].physics_id, glm::quat(rotation));
    //        }
    //        ImGui::End();
    //    }
    //}
    
    void render_debug(Player& player) {
        Shader* shader = Shader_Manager::get_shader(debug_shader);
        glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
        shader->set_mat4("projection", projection);
        glm::mat4 view = player.get_view_matrix();
        shader->set_mat4("view", view);

        if (editor_mode) {
            int half_width = scr_width / 2;
            int half_height = scr_height / 2;
            glViewport(0, half_height, half_width, half_height);
            debug_renderer.render(shader, projection, view);
            glViewport(0, 0, scr_width, scr_height);
        } 
        else 
            debug_renderer.render(shader, projection, view);

    }

    //void render_scene_deferred(Player& player, Scene& scene, float delta_time) {
    //    // 1. Geometry Pass: Render scene to G-buffer
    //    glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
    //    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    //    
    //    // Prepare matrices
    //    glm::mat4 projection = glm::perspective(glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
    //    glm::mat4 view = player.camera.get_view_matrix();

    //    // Use deferred geometry shader for G-buffer pass
    //    deferred_shader.use();
    //    deferred_shader.set_mat4("projection", projection);
    //    deferred_shader.set_mat4("view", view);

    //    // Render scene entities to G-buffer
    //    for (Entity& entity : scene.entities) {
    //        glm::mat4 model = entity.get_model_matrix();
    //        deferred_shader.set_mat4("model", model);
    //        
    //        // Calculate normal matrix (inverse transpose of the model matrix)
    //        glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
    //        deferred_shader.set_mat3("normal_matrix", normal_matrix);
    //        
    //        entity.draw(deferred_shader);
    //    }

    //    // 2. Lighting Pass: Render lighting using G-buffer
    //    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to default framebuffer
    //    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //    glDisable(GL_DEPTH_TEST);
    //    
    //    // Use lighting shader
    //    deferred_lighting_shader.use();
    //    
    //    // Bind G-buffer textures
    //    glActiveTexture(GL_TEXTURE0);
    //    glBindTexture(GL_TEXTURE_2D, g_position);
    //    glActiveTexture(GL_TEXTURE1);
    //    glBindTexture(GL_TEXTURE_2D, g_normal);
    //    glActiveTexture(GL_TEXTURE2);
    //    glBindTexture(GL_TEXTURE_2D, g_albedo_specular);

    //    // Set lighting uniforms
    //    deferred_lighting_shader.set_vec3("viewPos", player.camera.position);
    //    
    //    // Set light parameters
    //    deferred_lighting_shader.set_vec3("light.Position", light.position);
    //    deferred_lighting_shader.set_vec3("light.Color", light.color);
    //    deferred_lighting_shader.set_float("light.Linear", 0.09f);
    //    deferred_lighting_shader.set_float("light.Quadratic", 0.032f);
    //    deferred_lighting_shader.set_float("light.Intensity", light.intensity);

    //    // glUniform1i(glGetUniformLocation(deferred_lighting_shader.ID, "debug_mode"), 999);

    //    // Render a screen-filling quad
    //    // render_quad();
    //    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //    glBindVertexArray(quadVAO);
    //    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //    glBindVertexArray(0);
    //    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //    glEnable(GL_DEPTH_TEST);

    //    // Optional: Render debug information
    //    if (!player.key_toggles[(unsigned)'r']) {
    //        // debug_renderer.render(debug_shader, projection, view);
    //        debug_visualize_gbuffer(player);
    //    }
    //}

    //void debug_visualize_gbuffer(Player& player) {
    //    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //
    //    debug_gbuffer_shader.use();
    //
    //    // Bind G-buffer textures
    //    glActiveTexture(GL_TEXTURE0);
    //    glBindTexture(GL_TEXTURE_2D, g_position);
    //    glActiveTexture(GL_TEXTURE1);
    //    glBindTexture(GL_TEXTURE_2D, g_normal);
    //    glActiveTexture(GL_TEXTURE2);
    //    glBindTexture(GL_TEXTURE_2D, g_albedo_specular);
    //
    //    const char* mode_names[] = {
    //        "Position", "Normal", "Albedo", "Specular", "Depth"
    //    };
    //    static int current_mode = 0;
    //
    //    for (int i = 0; i < 4; ++i) {
    //        debug_gbuffer_shader.set_int("debug_mode", i);
    //
    //        int x = (i % 2) * (scr_width / 2);
    //        int y = (i / 2) * (scr_height / 2);
    //        glViewport(x, y, scr_width / 2, scr_height / 2);
    //
    //        // render_quad();
    //        glBindVertexArray(quadVAO);
    //        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //        glBindVertexArray(0);    
    //    }
    //
    //    // Reset viewport
    //    glViewport(0, 0, scr_width, scr_height);
    //}
    
    //void draw_player_stuff(Player& player, glm::vec3& clr, glm::vec3& emis_clr, glm::vec3& fres_clr, float expon, const Skybox& skybox) {
    //    // glDisable(GL_DEPTH_TEST);
    //    weapon_shader2.use();
    //    
    //    glm::mat4 projection = glm::perspective(glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
    //    weapon_shader2.set_mat4("projection", projection);
    //    
    //    glm::mat4 fullView = player.camera.get_view_matrix();
    //    glm::mat4 rotationOnlyView = glm::mat4(glm::mat3(fullView));
    //    weapon_shader2.set_mat4("view", rotationOnlyView);
    //    

    //    // REPLACE FROM HERE -------------------------------------------------------------
    //    glm::mat4 model = glm::mat4(1.0f);
    //    
    //    // FIX player.camera.yaw pitch roll maybe geeruc
    //    model = glm::rotate(model, -glm::radians(player.camera.yaw + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    //    model = glm::rotate(model, glm::radians(player.camera.pitch), glm::vec3(1.0f, 0.0f, 0.0f));
    //    
    //    model = glm::translate(model, player.controller->get_weapon_position());
    //    weapon_shader2.set_mat4("model", model);
    //    weapon_shader2.set_vec3("viewPos", player.camera.position);

    //    // HERE ------------------------------------
    //    // USE WEAPON + HANDS + QUATS + ANIMATION + OH GOD 

    //}

    void render_skybox(const Skybox& skybox, const glm::mat4& view, const glm::mat4& projection) {
        glDepthFunc(GL_LEQUAL);
        Shader* shader = Shader_Manager::get_shader(skybox_shader);
        shader->use();

        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

        shader->set_mat4("view", viewNoTranslation);
        shader->set_mat4("projection", projection);

        skybox.draw();

        glDepthFunc(GL_LESS);
    }

    void draw_model_at(Model& model, glm::vec3 pos) {

    }

    void render_crosshair(const Crosshair& crosshair) {
        Shader* s = Shader_Manager::get_shader(crosshair_shader);
        s->use();
        crosshair.draw(s, scr_width, scr_height);
    }

    void render_hud_text(const Text& text) {
        glm::mat4 projection = glm::ortho(0.0f, (float)scr_width, 0.0f, (float)scr_height);
        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        Shader* shader = Shader_Manager::get_shader(hud_text_shader);
        text.draw(shader, projection);
     
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    void debug_sphere_at(float x, float y, float z) {
        debug_renderer.add_sphere(glm::vec3(x, y, z), 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
    }

    void debug_sphere_at(glm::vec3 pos) {
        debug_renderer.add_sphere(pos, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
    }


    void shutdown() {
        glDeleteFramebuffers(1, &g_buffer);
        glDeleteTextures(1, &g_position);
        glDeleteTextures(1, &g_normal);
        glDeleteTextures(1, &g_albedo_specular);
        
        glDeleteVertexArrays(1, &quadVAO);
        
    }

    ortho_view_data* get_viewport_at_mouse(double xpos, double ypos) {
        if (!editor_mode) return nullptr;

        double half_width = scr_width / 2.0;
        double half_height = scr_height / 2.0;

        if (xpos < half_width && ypos < half_height) {
            return &editor_viewports.scene; // Top-Left
        }
        else if (xpos >= half_width && ypos < half_height) {
            return &editor_viewports.top; // Top-Right
        }
        else if (xpos < half_width && ypos >= half_height) {
            return &editor_viewports.side; // Bottom-Left
        }
        else {
            return &editor_viewports.front; // Bottom-Right
        }
    }


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

    editor_viewports_struct editor_viewports;
    shader_handle editor_shader;
    bool editor_mode = false;
    std::vector<size_t> selected_entites = { 0, 1, 2, 3, 4 };
    float outline_scale = 0.1f;

    float penis = 25.0f;
    float ambient_light = 0.05f;

    bool use_alpha_clipping = true;
    float alpha_cutoff = 0.5f;

    // deferred pipeline
    Shader deferred_shader, deferred_lighting_shader, debug_gbuffer_shader;
    GLuint g_buffer, g_position, g_normal, g_albedo_specular;
    GLuint quadVAO;
};
