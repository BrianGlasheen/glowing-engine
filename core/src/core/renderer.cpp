#include "renderer.h"

#include <cstddef>
#include <ctime>
#include <cfloat>
#include <iostream>
#include <algorithm>

// #include <glad/glad.h>
// #include <GLFW/glfw3.h>
#include "core/opengl.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "util/frustum.h"
#include "util/colors.h"
#include "util/profiler.h"

const float FAR_PLANE = 240.0f;

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


int Renderer::init() {
    // todo maybe move? into renderer or something?
    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return 1;
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    //glEnable(GL_STENCIL_TEST);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // LIGHTs
    spotlight = Light::create_spot(glm::vec3(0.0f, 5.0f, -5.0f), glm::vec3(0.0f, -1.0f, -0.5f), glm::vec3(1.0f), 15.0f, 25.0f, 45.0f, 1024, 1024);
    directional_light = Light::create_directional(glm::vec3(0.0f, -0.25f, 0.25f), glm::vec3(1.0f), 0.1f);
    point_light = Light::create_point(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f), 1.0f, 1024);

    for (int i = 0; i < 200; i++) {
        float x = ((float)rand() / RAND_MAX * 100.0f - 50);
        float y = ((float)rand() / RAND_MAX * 100.0f - 50);
        float r = ((float)rand() / RAND_MAX);
        float g = ((float)rand() / RAND_MAX);
        float b = ((float)rand() / RAND_MAX);

        GPU_Light point_light2 = {
            glm::vec4(x, 1.0f, y, 1.0f),          // position + radius (attenuation range)
            glm::vec4(r, g, b, 15.0f),          // color (white) + intensity
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),             // direction unused + type (0 = point light)
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)           // unused params for point light
        };
        lights.emplace_back(point_light2);

    }

    light_ssbo.init();
    light_ssbo.set_data(sizeof(Light) * 200, lights.data(), GL_DYNAMIC_DRAW);

    void* penis2 = calloc(1, sizeof(Cluster) * 16 * 9 * 24);
    cluster_ssbo.init();
    cluster_ssbo.set_data(sizeof(Cluster) * 16 * 9 * 24, penis2, GL_DYNAMIC_DRAW);

    // SHADERS
    Shader_Manager::init("../resources/shaders/");

    pbr_shader = Shader_Manager::load_from_paths("pbr", "vertex.glsl", "fragment.glsl");
    skybox_shader = Shader_Manager::load_from_name("skybox");
    debug_shader = Shader_Manager::load_from_name("debug");
    shadow_map_shader = Shader_Manager::load_from_name("shadow_map");
    point_shadow_map_shader = Shader_Manager::load_from_name("shadow_map_point");
    hud_text_shader = Shader_Manager::load_from_name("text_hud");
    //debug_shader.init("../resources/shaders/debug_v.glsl", "../resources/shaders/debug_f.glsl");
    outline_shader = Shader_Manager::load_from_name("outline");
    quad_shader = Shader_Manager::load_from_name("quad");

    Texture_Manager::init();
    setup_buffers(); // defferd g buffer setup
    //deferred_shader.init("../resources/shaders/deferred_v.glsl", "../resources/shaders/deferred_f.glsl");
    //deferred_lighting_shader.init("../resources/shaders/deferred_light_v.glsl", "../resources/shaders/deferred_light_f.glsl");
    //debug_gbuffer_shader.init("../resources/shaders/deferred_light_v.glsl", "../resources/shaders/deferred_lighting_debug_f.glsl");

    crosshair_shader = Shader_Manager::load_from_name("crosshair");

    bloom_down.init("../resources/shaders/compute/bloomdown.comp");
    bloom_up.init("../resources/shaders/compute/bloomup.comp");
    particle.init("../resources/shaders/compute/particle2.comp");
    particle_shader = Shader_Manager::load_from_name("particle");

    depth_prepass_shader = Shader_Manager::load_from_name("depth_prepass");

    cluster_build.init("../resources/shaders/compute/cluster.comp");
    slice_vis = Shader_Manager::load_from_name("cluster_vis");

    //toon.init("../resources/shaders/vertex.glsl", "../resources/shaders/toon.glsl");

    debug_renderer.init();

    return 0;
}



bool Renderer::setup_buffers() {
    glGenFramebuffers(1, &render_target);
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);

    //// Create two color textures - one for scene, one for bright areas
    scene_texture = Texture_Manager::create_render_texture(scr_width, scr_height, true);
    bright_texture = Texture_Manager::create_bloom_texture(scr_width, scr_height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(scene_texture), 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(bright_texture), 0);

    // Create depth renderbuffer
    glGenRenderbuffers(1, &render_depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, render_depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scr_width, scr_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, render_depth_buffer);

    // Specify which color attachments to use
    uint32_t attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("[RENDERER] MAIN RENDER BUFFER FAILLLLLED TF OUT\n");
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    uint32_t quadVBO;
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
    glBindVertexArray(0);

    return true;
}

void Renderer::shadow_pass(Scene& scene, const Player& player) {

    bool spotlight_shadowmap_dirty = false;
    // todo add check for LIGHT dirty

    Util::Frustum spotlight_frustum(spotlight.position, spotlight.direction, glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(spotlight.outer_fov * 2.0f), (float)spotlight.width / (float)spotlight.height, 0.1f, 50.0f);
    
    // if shadow map not dirty check if should be
    std::vector<size_t> spotlight_entities(16);
    size_t idx = 0;
    for (Entity& entity : scene.entities) {
        if (entity.physics_enabled) {
            Util::AABB box = Physics::get_world_AABB(entity.physics_id);
            if (spotlight_frustum.intersectsAABB(box.min, box.max)) {
                // mark as in shadow map
                spotlight_entities.push_back(idx);
                // check if dirty -> shadow map re-render
                if (entity.is_dirty) {
                    spotlight_shadowmap_dirty = true;
                    break;
                }
            }
        }
        idx++;
    }

    if (player.key_toggles['l'])
        debug_renderer.draw_frustum(spotlight.position, spotlight.direction, glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(spotlight.outer_fov * 2.0f), (float)spotlight.width / (float)spotlight.height, 1.0f, 50.0f, spotlight_shadowmap_dirty ? Util::magenta : Util::red);

    if (spotlight_shadowmap_dirty) {
        // todo totally change
        spotlight.bind_fbo_write();
        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);

        // use shadow shader
        Shader* shader = Shader_Manager::get_shader(shadow_map_shader);
        shader->use();
        glm::mat4 projection = glm::perspective(glm::radians(spotlight.outer_fov * 2.0f), (float)spotlight.width / (float)spotlight.height, 0.1f, 50.0f);

        shader->set_mat4("projection", projection);
        glm::mat4 view = glm::lookAt(spotlight.position, spotlight.position + spotlight.direction, glm::vec3(0.0f, 1.0f, 0.0f));
        shader->set_mat4("view", view);

        // frusutm cull objects + check move?
        for (size_t idx : spotlight_entities) {
            Entity& entity = scene.entities[idx];
            glm::mat4 model = entity.get_model_matrix();
            shader->set_mat4("model", model);
            bool shadow_pass = true;
            entity.draw(shader, shadow_pass);
        }
        printf("did spotlight shadowpass\n");
    }

    // dir light, maybe scene BB + csm
    directional_light.bind_fbo_write();
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    Shader* shader = Shader_Manager::get_shader(shadow_map_shader);
    shader->use();
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

    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, close_plane, penis);
    shader->set_mat4("projection", projection);

    shader->set_vec3("point_light_position", point_light.position);
    //shader->set_float("point_light_far_plane", 25.0f);

    for (size_t i = 0; i < 6; i++) {
        point_light.bind_cubemap_face_write(camera_directions[i].face);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        glm::vec3 target = point_light.position + camera_directions[i].direction;
        glm::mat4 view = glm::lookAt(point_light.position, target, camera_directions[i].up);
        shader->set_mat4("view", view);

        if (player.key_toggles['l'])
            debug_renderer.draw_frustum(point_light.position, camera_directions[i].direction, camera_directions[i].up, glm::radians(90.0f), 1.0f, close_plane, penis, Util::red);

        Util::Frustum frustum2(point_light.position, camera_directions[i].direction, camera_directions[i].up, glm::radians(90.0f), 1.0f, close_plane, penis);

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

void Renderer::build_cluster_pass(Player& player) {
    cluster_ssbo.bind(1);
    
    // compute pass
    cluster_build.set_float("zNear", 1.0f);
    cluster_build.set_float("zFar", FAR_PLANE);
    glm::mat4 inv_proj = glm::inverse(glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE));

    cluster_build.set_mat4 ("inverseProjection", inv_proj);
    cluster_build.set_vec3 ("gridSize", glm::vec3(100.0f));
    cluster_build.set_vec2 ("screenDimensions", glm::vec2(1600, 900));

    cluster_build.dispatch_and_wait(16, 9, 24);
}

void Renderer::render(Player& player, Scene& scene, float delta_time, SSBO& particles) {
    // begin frame
    {
        PROFILE_SCOPE_COLOR("depth pre-pass", legit::Colors::belizeHole);
        if (use_depth_prepass)
            depth_prepass(player, scene);
    }
    {
        PROFILE_SCOPE_COLOR("create cluster", legit::Colors::emerald);
        //build_cluster_pass(player);
    }
    {
        PROFILE_SCOPE_COLOR("shadows", legit::Colors::pomegranate);
        if (shadows_enabled)
            shadow_pass(scene, player);
    }
    {
        PROFILE_SCOPE_COLOR("scene", legit::Colors::clouds);
        render_scene(player, scene, delta_time);
    }
    {
        PROFILE_SCOPE_COLOR("particles", legit::Colors::wisteria);
        particle_pass(delta_time, particles, player);
    }
    // post process pass theoretically
    {
        PROFILE_SCOPE_COLOR("bloom", legit::Colors::nephritis);
        bloom_pass();
    }
    {
        PROFILE_SCOPE_COLOR("composite", legit::Colors::turqoise);
        composite();
    }
    // end frame
}

void Renderer::depth_prepass(Player& player, Scene& scene) {
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    glm::mat4 view = player.get_view_matrix();
    glm::mat4 viewproj = projection * view;

    Shader* shader = Shader_Manager::get_shader(depth_prepass_shader);
    shader->use();

    Util::Frustum frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    for (Entity& entity : scene.entities) {
        Util::AABB box;
        if (entity.physics_enabled) {
            box = Physics::get_world_AABB(entity.physics_id);
            if (frustum.intersectsAABB(box.min, box.max)) {
                glm::mat4 model = entity.get_model_matrix();
                shader->set_mat4("mvp", viewproj * model);
                bool no_materials = true;
                entity.draw(shader, no_materials);
            }
        }
        else {
            glm::mat4 model = entity.get_model_matrix();
            shader->set_mat4("mvp", viewproj * model);
            bool no_materials = true;
            entity.draw(shader, no_materials);
        }
    }
}

void Renderer::render_scene(Player& player, Scene& scene, float delta_time) {
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    if (use_depth_prepass) {
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDepthFunc(GL_LEQUAL); // fragments at same depth pass
        glDepthMask(GL_FALSE); // dont write depth
    }
    else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    }

    glEnable(GL_DEPTH_TEST);

    glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    glm::mat4 view = player.get_view_matrix();
    glm::mat4 viewproj = projection * view;

    //Shader used_shader = toon;
    Shader* shader = Shader_Manager::get_shader(pbr_shader);
    //Shader* shader = Shader_Manager::get_shader(slice_vis);
    shader->use();

    //shader->set_float("zNear", 0.1f);
    //shader->set_float("zFar", FAR_PLANE);
    //glm::mat4 inv_proj = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
    //shader->set_mat4("projection", inv_proj);
    //shader->set_vec3("gridSize", glm::vec3(16.0f, 9.0f, 24.0f));
    //shader->set_vec2("screenDimensions", glm::vec2(1600, 900));

    glStencilMask(0x00);

    light_ssbo.bind(0);

    shader->set_bool("use_alpha_clipping", use_alpha_clipping);
    shader->set_float("alpha_cutoff", alpha_cutoff);

    shader->set_bool("shadows_enabled", shadows_enabled);
    if (shadows_enabled) {

        // spotlight
        spotlight.bind_fbo_read(3);
        shader->set_int("shadow_map", 3);
        glm::mat4 lprojection = glm::perspective(glm::radians(spotlight.outer_fov * 2.0f), (float)spotlight.width / (float)spotlight.height, 0.1f, 50.0f);
        shader->set_mat4("light_projection", lprojection);
        glm::mat4 lview = glm::lookAt(spotlight.position, spotlight.position + spotlight.direction, glm::vec3(0.0f, 1.0f, 0.0f));
        shader->set_mat4("light_view", lview);
    
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

        point_light.bind_fbo_read(5);
        shader->set_int("point_shadow_map", 5);
        shader->set_float("point_light_far_plane", 50.0f);
    }

    shader->set_vec3("spot_light_position", spotlight.position);
    shader->set_vec3("spot_light_direction", spotlight.direction);
    shader->set_vec3("spot_light_color", spotlight.color);
    shader->set_float("spot_light_intensity", spotlight.intensity);
    shader->set_float("spot_light_inner_cone", glm::cos(glm::radians(spotlight.inner_fov)));
    shader->set_float("spot_light_outer_cone", glm::cos(glm::radians(spotlight.outer_fov)));
    debug_renderer.add_sphere(spotlight.position, 0.1f, spotlight.color);
    debug_renderer.add_line(spotlight.position, spotlight.position + spotlight.direction, spotlight.color);

    shader->set_vec3("directional_light_direction", directional_light.direction);
    shader->set_vec3("directional_light_color", directional_light.color);
    shader->set_float("directional_light_intensity", directional_light.intensity);
    debug_renderer.add_line(glm::vec3(0.0f, 10.f, 0.0f), glm::vec3(0.0f, 10.f, 0.0f) + directional_light.direction, spotlight.color);

    shader->set_vec3("point_light_position", point_light.position);
    shader->set_vec3("point_light_color", point_light.color);
    shader->set_float("point_light_intensity", point_light.intensity);
    debug_renderer.add_sphere(point_light.position, 0.1f, glm::vec3(1.0f));

    shader->set_float("ambient_light", ambient_light);


    //glm::projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE * (player.out_of_body ? 1.5f : 1.0f));
    shader->set_vec3("view_position", player.get_view_position());

    if (player.out_of_body) {
        debug_renderer.add_sphere(player.camera.position, 1.0f, glm::vec3(1.0f));
        debug_renderer.draw_frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE, Util::red);
    }

    Util::Frustum frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
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
                shader->set_mat4("mvp", viewproj * model);

                glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
                shader->set_mat3("normal_matrix", normal_matrix);
                entity.draw(shader);
            }
        }
        else {
            count++;

            glm::mat4 model = entity.get_model_matrix();
            shader->set_mat4("model", model);
            shader->set_mat4("mvp", viewproj * model);

            glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
            shader->set_mat3("normal_matrix", normal_matrix);

            entity.draw(shader);
        }

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

    glStencilMask(0xFF); // todo figure out where this goes?
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

    // flush(); !!
}


void Renderer::render_debug(Player& player) {
    Shader* shader = Shader_Manager::get_shader(debug_shader);
    glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    shader->set_mat4("projection", projection);
    glm::mat4 view = player.get_view_matrix();
    shader->set_mat4("view", view);

    debug_renderer.render(shader, projection, view);
}

void Renderer::particle_pass(float delta_time, SSBO& particle_ssbo, Player& player) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_DEPTH_TEST);
    const int MAX_PARTICLES = 10000;
    
    particle.use();
    particle.set_float("dt", delta_time);

    particle.set_vec3("emitter_position", emitter_position);
    particle.set_vec3("acceleration_direction", acceleration_direction);
    particle.set_float("acceleration_force", acceleration_force);

    particle.set_vec2("life_range", life_range);
    particle.set_vec4("color_start_base", color_start_base);
    particle.set_vec4("color_end_base", color_end_base);
    particle.set_vec3("velocity_base", velocity_base);
    particle.set_vec3("velocity_random_bias", velocity_random_bias);
    particle.set_float("velocity_mag", velocity_mag);

    particle.set_float("emission_rate", emission_rate);
    particle.set_int("max_particle", MAX_PARTICLES);

    particle_ssbo.bind(0);
    glDispatchCompute((MAX_PARTICLES + 127) / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    Shader* shader = Shader_Manager::get_shader(particle_shader);
    shader->use();
    shader->set_mat4("view", player.get_view_matrix());
    shader->set_mat4("projection", projection);
    particle_ssbo.bind(0);
    glBindVertexArray(quadVAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, MAX_PARTICLES);
    
    //glEnable(GL_DEPTH_TEST);
}

void Renderer::bloom_pass() {
    uint32_t texture_id = Texture_Manager::get_ogl_id(bright_texture);

    int base_width = 1600;
    int base_height = 900;
    const int MIP_LEVELS = 6;

    for (int i = 1; i < MIP_LEVELS; i++) {
        int mip_width = std::max(1, base_width >> i);
        int mip_height = std::max(1, base_height >> i);

        // prev mip input
        glBindImageTexture(0, texture_id, i - 1, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        // current mip output
        glBindImageTexture(1, texture_id, i, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        bloom_down.use();
        int groups_x = (mip_width + 7) / 8;
        int groups_y = (mip_height + 7) / 8;
        bloom_down.dispatch_and_wait(groups_x, groups_y, 1);
    }

    for (int i = MIP_LEVELS - 2; i >= 0; i--) {
        int mip_width = std::max(1, base_width >> i);
        int mip_height = std::max(1, base_height >> i);

        // bind current mip
        glBindImageTexture(0, texture_id, i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        // bind higher res mip
        glBindImageTexture(1, texture_id, i + 1, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

        bloom_up.use();
        int groups_x = (mip_width + 7) / 8;
        int groups_y = (mip_height + 7) / 8;
        bloom_up.dispatch_and_wait(groups_x, groups_y, 1);
    }
}

void Renderer::composite() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to default framebuffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    Shader* shader = Shader_Manager::get_shader(quad_shader);
    shader->use();

    shader->set_int("scene_color", 0);
    Texture_Manager::bind(scene_texture, 0);
    shader->set_int("bright_color", 1);
    Texture_Manager::bind(bright_texture, 1);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0); // unbind quad
}

void Renderer::render_skybox(const Skybox& skybox, const glm::mat4& view, const glm::mat4& projection) {
    glDepthFunc(GL_LEQUAL);
    Shader* shader = Shader_Manager::get_shader(skybox_shader);
    shader->use();

    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

    shader->set_mat4("view", viewNoTranslation);
    shader->set_mat4("projection", projection);

    skybox.draw();

    glDepthFunc(GL_LESS);
}

void Renderer::render_crosshair(const Crosshair& crosshair) {
    Shader* s = Shader_Manager::get_shader(crosshair_shader);
    s->use();
    crosshair.draw(s, scr_width, scr_height);
}

void Renderer::render_hud_text(const Text& text) {
    glm::mat4 projection = glm::ortho(0.0f, (float)scr_width, 0.0f, (float)scr_height);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader* shader = Shader_Manager::get_shader(hud_text_shader);
    text.draw(shader, projection);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Renderer::debug_sphere_at(float x, float y, float z) {
    debug_renderer.add_sphere(glm::vec3(x, y, z), 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
}

void Renderer::debug_sphere_at(glm::vec3 pos) {
    debug_renderer.add_sphere(pos, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
}

void Renderer::imgui_pass() {
    ImGui::Begin("Renderer Settings");
    ImGui::Checkbox("depth pre-pass", &use_depth_prepass);
    ImGui::Checkbox("shadows enabled", &shadows_enabled);

    ImGui::End();
}

void Renderer::shutdown() {
    //glDeleteFramebuffers(1, &g_buffer);
    //glDeleteTextures(1, &g_position);
    //glDeleteTextures(1, &g_normal);
    //glDeleteTextures(1, &g_albedo_specular);

    glDeleteFramebuffers(1, &render_target);

    glDeleteVertexArrays(1, &quadVAO);
}
