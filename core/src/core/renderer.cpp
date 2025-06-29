#include "renderer.h"

#include <filesystem>
#include <ctime>
#include <cfloat>
#include <iostream>

// #include <glad/glad.h>
// #include <GLFW/glfw3.h>
#include "core/opengl.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>


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

    return 0;
}



bool Renderer::setup_buffers() {
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
    uint32_t attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    // Create and attach depth buffer
    uint32_t rboDepth;
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

void Renderer::shadow_pass(Scene& scene, const Player& player) {
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

void Renderer::render(Player& player, Scene& scene, float delta_time) {

    shadow_pass(scene, player);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, scr_width, scr_height);

    render_scene(player, scene, delta_time);
}

void Renderer::render_scene(Player& player, Scene& scene, float delta_time) {
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

    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    // flush(); !!
}


void Renderer::render_debug(Player& player) {
    Shader* shader = Shader_Manager::get_shader(debug_shader);
    glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
    shader->set_mat4("projection", projection);
    glm::mat4 view = player.get_view_matrix();
    shader->set_mat4("view", view);

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


void Renderer::shutdown() {
    glDeleteFramebuffers(1, &g_buffer);
    glDeleteTextures(1, &g_position);
    glDeleteTextures(1, &g_normal);
    glDeleteTextures(1, &g_albedo_specular);

    glDeleteVertexArrays(1, &quadVAO);
}
