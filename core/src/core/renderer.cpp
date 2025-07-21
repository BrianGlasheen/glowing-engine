#include "renderer.h"

#include <cstddef>
#include <ctime>
#include <cfloat>
#include <iostream>
#include <algorithm>
#include <random>

// #include <glad/glad.h>
// #include <GLFW/glfw3.h>
#include "core/opengl.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "asset/material_manager.h"

#include "util/frustum.h"
#include "util/colors.h"
#include "util/profiler.h"

const float FAR_PLANE = 240.0f;
const uint32_t MAX_DRAW_COMMANDS = 4000;


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
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    glClearDepth(0.0f);

    glEnable(GL_CULL_FACE);

    //glEnable(GL_STENCIL_TEST);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // LIGHTs
    spotlight = Light::create_spot(glm::vec3(0.0f, 5.0f, -5.0f), glm::vec3(0.0f, -1.0f, -0.5f), glm::vec3(1.0f), 15.0f, 25.0f, 45.0f, 1024, 1024);
    directional_light = Light::create_directional(glm::vec3(0.0f, -0.25f, 0.25f), glm::vec3(1.0f), 0.1f);
    point_light = Light::create_point(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f), 1.0f, 1024);

    lights.reserve(1000);
    for (int i = 0; i < 1000; i++) {
        float x = ((float)rand() / RAND_MAX * 500.0f - 250);
        float y = ((float)rand() / RAND_MAX * 50 - 25);
        float z = ((float)rand() / RAND_MAX * 500.0f - 250);
        float r = ((float)rand() / RAND_MAX);
        float g = ((float)rand() / RAND_MAX);
        float b = ((float)rand() / RAND_MAX);

        GPU_Light point_light2 = {
            glm::vec4(x, y, z, 105.0f),          // position + radius (attenuation range)
            glm::vec4(r, g, b, 305.0f),          // color (white) + intensity
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),             // direction unused + type (0 = point light)
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)           // unused params for point light
        };
        lights.emplace_back(point_light2);

    }

    light_ssbo.init();
    light_ssbo.set_data(sizeof(GPU_Light) * 1000, lights.data(), GL_DYNAMIC_DRAW);

    cluster_ssbo.init();
    cluster_ssbo.set_data(sizeof(Cluster) * 16 * 9 * 24, nullptr, GL_STATIC_COPY);

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
    indirect_depth_prepass_shader = Shader_Manager::load_from_paths("MDI_depth_prepass", "vertex_ind_depth_v.glsl", "depth_prepass_f.glsl");

    cluster_build.init("../resources/shaders/compute/cluster.comp");
    cluster_cull.init("../resources/shaders/compute/cluster_cull.comp");
    slice_vis = Shader_Manager::load_from_name("cluster_vis");

    //toon.init("../resources/shaders/vertex.glsl", "../resources/shaders/toon.glsl");

    debug_renderer.init();

    setup_ssao();

    return 0;
}

void Renderer::setup_indirect() {
    indirect_shader = Shader_Manager::load_from_paths("indirect", "vertex_ind_v.glsl", "fragment_ind_f.glsl");
    // indirect draw cmds buffer
    //Model_Indirect mind = Model_Manager::get_model_ind(0);
    //draw_commands.resize(mind.m_meshes.size());
    //for (uint32_t i = 0; i < mind.m_meshes.size(); i++) {
    //    Draw_Elements_Indirect_Command draw_command;
    //    draw_command.count = mind.m_meshes[i].index_count;
    //    draw_command.instance_count = 1;
    //    draw_command.first_index = mind.m_meshes[i].base_index;
    //    draw_command.base_vertex = mind.m_meshes[i].base_vertex;
    //    draw_command.base_instance = 0;
    //    draw_commands[i] = draw_command;
    //}

    //glCreateBuffers(1, &draw_command_buffer);
    //glNamedBufferStorage(draw_command_buffer, sizeof(Draw_Elements_Indirect_Command) * draw_commands.size(), (const void*)draw_commands.data(), 0);

    draw_commands.reserve(MAX_DRAW_COMMANDS);
    per_object_data.reserve(MAX_DRAW_COMMANDS);

    glCreateBuffers(1, &draw_command_buffer);
    glNamedBufferStorage(draw_command_buffer, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &per_object_ssbo);
    glNamedBufferStorage(per_object_ssbo, sizeof(Per_Object_Data) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void Renderer::setup_ssao() {
    std::vector<glm::vec3> samples;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    int max_ssao_samples = 64;
    for (int i = 0; i < max_ssao_samples; i++) {
        glm::vec3 sample(
            dis(gen) * 2.0f - 1.0f,
            dis(gen) * 2.0f - 1.0f,
            dis(gen) // pos z, hemisphere (reverse z)
        );
        sample = glm::normalize(sample);
        sample *= dis(gen);

        // bias to center
        float scale = float(i) / float(max_ssao_samples);
        scale = 0.1f + scale * scale * 0.9f;
        sample *= scale;

        samples.push_back(sample);
    }
    // bind shader upload samples
    ssao.init("../resources/shaders/compute/ssao.comp");
    ssao.use();
    for (uint32_t i = 0; i < samples.size(); i++)
        ssao.set_vec3("samples[" + std::to_string(i) + "]", samples[i]);


    std::vector<float> noise;
    for (int i = 0; i < 16; ++i) {
        glm::vec3 vals = glm::normalize(glm::vec3(dis(gen) * 2.0f - 1.0f, dis(gen) * 2.0f - 1.0f, 0.0f));
        noise.push_back(vals.x);
        noise.push_back(vals.y);
        noise.push_back(vals.z);
    }

    ssao_noise_texture = Texture_Manager::create_noise_texture(noise, 4, 4);
    Texture_Manager::bind(ssao_noise_texture, 2);
}

bool Renderer::setup_buffers() {
    glGenFramebuffers(1, &render_target);
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);

    //// Create two color textures - one for scene, one for bright areas
    scene_texture = Texture_Manager::create_render_texture(scr_width, scr_height, true);
    bright_texture = Texture_Manager::create_bloom_texture(scr_width, scr_height);
    ssao_texture = Texture_Manager::create_ssao_texture(scr_width, scr_height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(scene_texture), 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(bright_texture), 0);

    // Create depth renderbuffer
    /*glGenRenderbuffers(1, &render_depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, render_depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, scr_width, scr_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_depth_buffer);*/

    depth_texture = Texture_Manager::create_depth_texture(scr_width, scr_height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(depth_texture), 0);

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
    cluster_build.use();

    cluster_ssbo.bind(1);
    
    cluster_build.set_float("zNear", 1.0f);
    cluster_build.set_float("zFar", FAR_PLANE);
    glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height);
    glm::mat4 inv_proj = glm::inverse(projection);
    cluster_build.set_mat4("inverseProjection", inv_proj);
    cluster_build.set_uvec3("gridSize", glm::uvec3(16, 9, 24));
    cluster_build.set_uvec2("screenDimensions", glm::uvec2(1600, 900));

    cluster_build.dispatch_and_wait(16, 9, 24);
}

void Renderer::cull_cluster_pass(Player& player) {
    cluster_cull.use();

    cluster_ssbo.bind(1);
    light_ssbo.bind(2);

    cluster_cull.set_mat4("viewMatrix", player.get_view_matrix());
    cluster_cull.set_int("num_lights", num_lights);

    cluster_cull.dispatch_and_wait(27, 1, 1);
}

void Renderer::build_command_buffer(Player& player, Scene& scene, float delta_time) {
    Util::Frustum frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);

    draw_commands.clear();
    per_object_data.clear();
    uint32_t current_draw_count = 0;

    for (Entity& entity : scene.entities) {

        Model_Indirect mind = Model_Manager::get_model_ind(entity.model_id);

        Util::AABB model_aabb = Util::transform_aabb(mind.m_aabb, entity.get_model_matrix());
        debug_renderer.add_bbox(model_aabb.min, model_aabb.max, glm::vec3(1.0f, 1.0f, 1.0f));

        // if culled
        if (!frustum.intersectsAABB(model_aabb, true))
            continue;

        for (uint32_t i = 0; i < mind.m_meshes.size(); i++) {
            if (current_draw_count >= MAX_DRAW_COMMANDS) {
                printf("Warning: Exceeded max draw commands!\n");
                break;
            }
            Per_Object_Data obj_data;

            obj_data.model_matrix = entity.get_model_matrix() * mind.m_meshes[i].transform;
            Util::AABB obj_aabb = Util::transform_aabb(mind.m_meshes[i].aabb, entity.get_model_matrix());

            if (!frustum.intersectsAABB(obj_aabb, true)) {
                debug_renderer.add_bbox(obj_aabb.min, obj_aabb.max, glm::vec3(1.0f, 0.0f, 1.0f));
                continue;
            }
            debug_renderer.add_bbox(obj_aabb.min, obj_aabb.max, glm::vec3(0.0f, 1.0f, 0.0f));

            // if culled
            // continue

            Draw_Elements_Indirect_Command draw_command;
            draw_command.count = mind.m_meshes[i].index_count;
            draw_command.instance_count = 1;
            draw_command.first_index = mind.m_meshes[i].base_index;
            draw_command.base_vertex = mind.m_meshes[i].base_vertex;
            draw_command.base_instance = current_draw_count;

            draw_commands.push_back(draw_command);

            obj_data.normal_matrix = glm::transpose(glm::inverse(obj_data.model_matrix));
            //obj_data.color = glm::vec4(0.0, 0.25, 0.5, 0.75);
            const Material_Indirect& mater = Material_Manager::get_material(mind.m_meshes[i].material_index);
            obj_data.albedo = mater.albedo;
            obj_data.normal = mater.normal;
            obj_data.met_rough = mater.met_rough;
            obj_data.emissive = mater.emissive;
            obj_data.amb_occ = mater.amb_occ;
            obj_data.emissive_factor = mater.emissive_factor;
            obj_data.metallic_factor = mater.metallic_factor; // 4
            obj_data.roughness_factor = mater.roughness_factor; // 4
            obj_data.base_color = mater.base_color;

            per_object_data.push_back(obj_data);

            current_draw_count++;
        }
    }

    if (current_draw_count > 0) {
        glNamedBufferSubData(draw_command_buffer, 0, sizeof(Draw_Elements_Indirect_Command) * current_draw_count, draw_commands.data());

        glNamedBufferSubData(per_object_ssbo, 0, sizeof(Per_Object_Data) * current_draw_count, per_object_data.data());
    }

    if (player.out_of_body) {
        debug_renderer.add_sphere(player.camera.position, 2.0f, glm::vec3(1.0f));
        debug_renderer.draw_frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 1.0f, 1000, Util::red);
        printf("out\n");
    }
}

void Renderer::indirect_depth_prepass(Player& player) {
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);
    glEnable(GL_DEPTH_TEST); // should be on already todo remove maybe

    // pre pass state
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = player.camera.get_projection((float)scr_width / (float)scr_height, player.get_camera_zoom());
    glm::mat4 view = player.get_view_matrix();
    glm::mat4 viewproj = projection * view;

    Shader* shader = Shader_Manager::get_shader(indirect_depth_prepass_shader);
    shader->use();
    shader->set_mat4("vp", viewproj);

    uint32_t vao = Model_Manager::get_big_vao();
    glBindVertexArray(vao);

    // draw commands and transform
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, per_object_ssbo);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_command_buffer);

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, draw_commands.size(), 0);

    glBindVertexArray(0);
}

void Renderer::render_indirect(Player& player) {
    // DRAW
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);
    glEnable(GL_DEPTH_TEST); // should be on already todo remove maybe

    glClearColor(0.1f, 0.2f, 0.1f, 1.0f);
    if (use_depth_prepass) {
        // after pre pass set this state
        glClear(GL_COLOR_BUFFER_BIT);
        glDepthFunc(GL_GEQUAL); // fragments at same depth pass
        glDepthMask(GL_FALSE); // dont write depth, unnecessary
    }
    else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_GREATER);
        glDepthMask(GL_TRUE);
    }

    glm::mat4 projection = player.camera.get_projection((float)scr_width / (float)scr_height, player.get_camera_zoom());
    glm::mat4 view = player.get_view_matrix();
    glm::mat4 viewproj = projection * view;

    Shader* shader = Shader_Manager::get_shader(indirect_shader);
    shader->use();
    //glStencilMask(0x00);

    shader->set_mat4("vp", viewproj);

    shader->set_vec3("view_pos", player.camera.position);
    shader->set_int("num_lights", num_lights);
    shader->set_bool("forward_plus", forward_plus);
    shader->set_float("zNear", 1.0f);
    shader->set_float("zFar", FAR_PLANE);
    shader->set_mat4("viewMatrix", player.get_view_matrix());
    shader->set_uvec3("gridSize", glm::uvec3(16, 9, 24));
    shader->set_uvec2("screenDimensions", glm::uvec2(1600, 900));

    shader->set_bool("ssao_enabled", ssao_enabled);


    //static float time = 0.0f;
    //time += delta_time;

    // main pass

    uint32_t vao = Model_Manager::get_big_vao();
    glBindVertexArray(vao);

    // draw commands and transform
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_command_buffer);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, per_object_ssbo);
    cluster_ssbo.bind(1);
    light_ssbo.bind(2);
    Texture_Manager::bind(ssao_texture, 0);

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, draw_commands.size(), 0);

    glBindVertexArray(0);
    // unbind
    //glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    // unbind ssbos?
}

void Renderer::render(Player& player, Scene& scene, float delta_time, SSBO& particles) {
    // begin frame
    {
        PROFILE_SCOPE_COLOR("build commands", legit::Colors::wisteria);
        build_command_buffer(player, scene, delta_time);
    }
    {
        PROFILE_SCOPE_COLOR("depth pre-pass", legit::Colors::belizeHole);
        if (use_depth_prepass) {
            if (indirect_rendering)
                indirect_depth_prepass(player);
            else
                depth_prepass(player, scene);
        }
    }
    {
        PROFILE_SCOPE_COLOR("build clusters", legit::Colors::emerald);
        if (forward_plus)
            build_cluster_pass(player);
    }
    {
        PROFILE_SCOPE_COLOR("cull lights", legit::Colors::greenSea);
        if (forward_plus)
            cull_cluster_pass(player);
    }
    {
        PROFILE_SCOPE_COLOR("SSAO", legit::Colors::sunFlower);
        if (ssao_enabled)
            ssao_pass(player);
    }
    {
        PROFILE_SCOPE_COLOR("shadows", legit::Colors::pomegranate);
        if (shadows_enabled)
            shadow_pass(scene, player);
    }
    {
        PROFILE_SCOPE_COLOR("submit render commands", legit::Colors::clouds);
        if (indirect_rendering) {
            render_indirect(player);
        }
        else
            render_scene(player, scene, delta_time);
    }
    {
        PROFILE_SCOPE_COLOR("particles", legit::Colors::wisteria);
        particle_pass(delta_time, particles, player);
    }
    // post process pass theoretically
    {
        PROFILE_SCOPE_COLOR("bloom", legit::Colors::nephritis);
        if (bloom_enabled)
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
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    //glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height);

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
        glDepthFunc(GL_GREATER);
        glDepthMask(GL_TRUE);
    }

    glEnable(GL_DEPTH_TEST);

    //glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height);
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

    cluster_ssbo.bind(1);
    light_ssbo.bind(2);

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

    shader->set_int("num_lights", num_lights);
    shader->set_bool("forward_plus", forward_plus);
    shader->set_float("zNear", 1.0f);
    shader->set_float("zFar", FAR_PLANE);
    shader->set_mat4("viewMatrix", player.get_view_matrix());
    shader->set_uvec3("gridSize", glm::uvec3(16, 9, 24));
    shader->set_uvec2("screenDimensions", glm::uvec2(1600, 900));

    //glm::projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE * (player.out_of_body ? 1.5f : 1.0f));
    shader->set_vec3("view_position", player.get_view_position());

    if (player.out_of_body) {
        debug_renderer.add_sphere(player.camera.position, 1.0f, glm::vec3(1.0f));
        debug_renderer.draw_frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE, Util::red);
    }

    Util::Frustum frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    int count = 0;
    for (Entity& entity : scene.entities) {

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
}


void Renderer::render_debug(Player& player) {
    Shader* shader = Shader_Manager::get_shader(debug_shader);
    //glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    //glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height);
    glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height, player.get_camera_zoom());

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
    
    //glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    //glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height);
    glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height, player.get_camera_zoom());

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

void Renderer::ssao_pass(Player& player) {
    ssao.use();

    Texture_Manager::bind(depth_texture, 0); // todo maybe dont need to
    Texture_Manager::bind(ssao_noise_texture, 1);
    //Texture_Manager::bind(ssao_texture, 2);
    //glBindImageTexture(2, Texture_Manager::get_ogl_id(ssao_texture), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
    glBindImageTexture(2, Texture_Manager::get_ogl_id(ssao_texture), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    glm::mat4 projection = player.camera.get_projection((float)scr_width / (float)scr_height, player.get_camera_zoom());

    //float fov = glm::radians(player.get_camera_zoom());
    //float aspect = (float)scr_width / (float)scr_height;
    //float near_plane = NEAR_PLANE;
    //float far_plane = 1000.0f;

    //glm::mat4 finite_projection(0.0f);
    //float f = 1.0f / std::tan(fov * 0.5f);
    //finite_projection[0][0] = f / aspect;
    //finite_projection[1][1] = f;
    //finite_projection[2][2] = near_plane / (far_plane - near_plane);     // Reverse-Z
    //finite_projection[2][3] = -1.0f;
    //finite_projection[3][2] = (near_plane * far_plane) / (far_plane - near_plane);

    ssao.set_mat4("projection", projection);
    ssao.set_mat4("inv_projection", glm::inverse(projection));
    ssao.set_vec2("screen_size", glm::vec2(scr_width, scr_height));
    ssao.set_float("radius", ssao_radius);
    ssao.set_float("bias", ssao_bias);
    ssao.set_int("sample_count", ssao_samples);
    ssao.set_float("min_depth", min_depth);

    ssao.dispatch_and_wait((scr_width + 15) / 16, (scr_height + 15) / 16, 1);
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
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

    glDepthFunc(GL_GREATER);
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
    ImGui::SliderInt("num_lights", &num_lights, 0, 1000);
    ImGui::Checkbox("forward+", &forward_plus);
    ImGui::Checkbox("indirect_rendering", &indirect_rendering);
    ImGui::Checkbox("bloom_enabled", &bloom_enabled);
    ImGui::Checkbox("ssao_enabled", &ssao_enabled);
    ImGui::SliderFloat("ssao_radius", &ssao_radius, 0, 5.0);
    ImGui::SliderFloat("ssao_bias", &ssao_bias, 0, 1.0f);
    ImGui::SliderInt("ssao_samples", &ssao_samples, 0, 64);
    ImGui::SliderFloat("min_depth", &min_depth, -0.01, 0.2f);

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
