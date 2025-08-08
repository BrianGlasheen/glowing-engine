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
#include "asset/model_manager.h"

#include "util/frustum.h"
#include "util/colors.h"
#include "util/profiler.h"

const float FAR_PLANE = 1000.0f;
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

static glm::mat4 cascade_mats[4] = { 0 };

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
    //glDisable(GL_STENCIL_TEST);
    //glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    //glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // LIGHTs
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

    //Shader_Manager::load_from_paths("pbr", "vertex.glsl", "fragment.glsl");
    Shader_Manager::load_from_name("skybox");
    //Shader_Manager::load_from_name("shadow_map");
    //Shader_Manager::load_from_name("shadow_map_point");
    Shader_Manager::load_from_name("quad");

    Texture_Manager::init();
    setup_buffers();

    Shader_Manager::load_from_name("particle");
    Shader_Manager::load_from_name("depth_prepass");
    Shader_Manager::load_from_paths("indirect_depth_prepass", "vertex_ind_depth_v.glsl", "depth_prepass_f.glsl");

    // 
    Shader_Manager::load_from_name("outline");
    Shader_Manager::load_from_name("debug");

    // hud
    Shader_Manager::load_from_name("crosshair");
    Shader_Manager::load_from_name("text_hud");

    //
    Shader_Manager::load_from_name("cluster_vis");
    Shader_Manager::load_from_name("light_quad_debug");

    // core rendering compute shaders
    Shader_Manager::load_compute("cluster");
    Shader_Manager::load_compute("cluster_cull");

    // post processing compute shaders
    Shader_Manager::load_compute("bloomdown");
    Shader_Manager::load_compute("bloomup");
    Shader_Manager::load_compute("particle2");

    debug_renderer.init();

    setup_ssao();

    glGenFramebuffers(1, &csm_fbo);
    csm_texture = Texture_Manager::create_3d_texture(2048, 2048, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, csm_fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, Texture_Manager::get_ogl_id(csm_texture), 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::FRAMEBUFFER:: CSM FBO is not complete!";
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return 0;
}

void Renderer::resize(const int width, const int height) {
    printf("[RENDERER] got resize event w: %d, h: %d\n", width, height);

    scr_width = width;
    scr_height = height;

    Texture_Manager::resize(scene_texture, scr_width, scr_height);
    Texture_Manager::resize(bright_texture, scr_width, scr_height, 6);
    Texture_Manager::resize(ssao_texture, scr_width, scr_height);
    Texture_Manager::resize(depth_texture, scr_width, scr_height);
}

void Renderer::setup_indirect() {
    Shader_Manager::load_from_paths("indirect", "vertex_ind_v.glsl", "fragment_ind_f.glsl");

    draw_commands.reserve(MAX_DRAW_COMMANDS); // todo change draw command per entity prob
    per_object_data.reserve(MAX_DRAW_COMMANDS);

    glCreateBuffers(1, &draw_command_buffer);
    glNamedBufferStorage(draw_command_buffer, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &per_object_ssbo);
    glNamedBufferStorage(per_object_ssbo, sizeof(Per_Object_Data) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);


    Shader_Manager::load_from_name("skinned");
    draw_commands_skinned.reserve(MAX_DRAW_COMMANDS);
    per_object_data_skinned.reserve(MAX_DRAW_COMMANDS);

    glCreateBuffers(1, &draw_command_buffer_skinned);
    glNamedBufferStorage(draw_command_buffer_skinned, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &per_object_ssbo_skinned);
    glNamedBufferStorage(per_object_ssbo_skinned, sizeof(Per_Object_Data) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);
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
    Shader_Manager::load_compute("ssao");
    Compute_Shader* ssao = Shader_Manager::get_compute("ssao");
    ssao->use();
    for (uint32_t i = 0; i < samples.size(); i++)
        ssao->set_vec3("samples[" + std::to_string(i) + "]", samples[i]);

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

    scene_texture = Texture_Manager::create_render_texture(scr_width, scr_height, true);
    bright_texture = Texture_Manager::create_bloom_texture(scr_width, scr_height);
    ssao_texture = Texture_Manager::create_ssao_texture(scr_width, scr_height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(scene_texture), 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(bright_texture), 0);

    depth_texture = Texture_Manager::create_depth_texture(scr_width, scr_height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(depth_texture), 0);

    uint32_t attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

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

    printf("Quad VAO created: %u\n", quadVAO);
    glBindVertexArray(0);

    return true;
}

void Renderer::build_cluster_pass(Player& player) {
    Compute_Shader* cluster_build = Shader_Manager::get_compute("cluster");
    cluster_build->use();
    cluster_ssbo.bind(1); // todo fix once
    
    cluster_build->set_float("zNear", 1.0f);
    cluster_build->set_float("zFar", FAR_PLANE);
    glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height);
    glm::mat4 inv_proj = glm::inverse(projection);
    cluster_build->set_mat4("inverseProjection", inv_proj);
    cluster_build->set_uvec3("gridSize", glm::uvec3(16, 9, 24));
    cluster_build->set_uvec2("screenDimensions", glm::uvec2(scr_width, scr_height));

    uint32_t groups_x = (16 + 7) / 8;
    uint32_t groups_y = (9 + 7) / 8;
    cluster_build->dispatch_and_wait(16, 9, 24, GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::cull_cluster_pass(Player& player) {
    Compute_Shader* cluster_cull = Shader_Manager::get_compute("cluster_cull");
    cluster_cull->use();

    cluster_ssbo.bind(1); // todo fix to do once
    light_ssbo.bind(2);

    cluster_cull->set_mat4("viewMatrix", player.get_view_matrix());
    cluster_cull->set_int("num_lights", num_lights);

    cluster_cull->dispatch_and_wait(27, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::shadow_pass(Scene& scene, const Player& player) {
    // cascades





    // atlas
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
        debug_renderer.draw_frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE, Util::red);
        printf("out\n");
    }

    //
    // skinned commands
    //


    draw_commands_skinned.clear();
    per_object_data_skinned.clear();
    current_draw_count = 0;
    
    for (uint32_t m = 0; m < Model_Manager::get_num_animated_models(); m++){
        Animated_Model mind = Model_Manager::get_skinned_model(m);
        //debug_renderer.add_bbox(mind.m_aabb.min, mind.m_aabb.max, glm::vec3(1.0f, 0.0f, 0.0f));

        for (uint32_t i = 0; i < mind.m_meshes.size(); i++) {
            Per_Object_Data obj_data = { 0 };

            int col = m % 50;
            int row = m / 50;
            glm::vec3 pos(50 * col, 0, row);

            obj_data.model_matrix = glm::scale(glm::translate(mind.m_meshes[i].transform, pos), glm::vec3(1.0f));

            Draw_Elements_Indirect_Command draw_command;
            draw_command.count = mind.m_meshes[i].index_count;
            draw_command.instance_count = 1;
            draw_command.first_index = mind.m_meshes[i].base_index;
            draw_command.base_vertex = mind.m_meshes[i].base_vertex;
            draw_command.base_instance = current_draw_count;

            draw_commands_skinned.push_back(draw_command);

            //obj_data.normal_matrix = glm::transpose(glm::inverse(obj_data.model_matrix));
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

            obj_data.bone_offset = mind.bone_offset;

            per_object_data_skinned.push_back(obj_data);

            current_draw_count++;
        }
    }

    if (current_draw_count > 0) {
        glNamedBufferSubData(draw_command_buffer_skinned, 0, sizeof(Draw_Elements_Indirect_Command) * current_draw_count, draw_commands_skinned.data());

        glNamedBufferSubData(per_object_ssbo_skinned, 0, sizeof(Per_Object_Data) * current_draw_count, per_object_data_skinned.data());
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

    Shader* shader = Shader_Manager::get_shader("indirect_depth_prepass");
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

    Shader* shader = Shader_Manager::get_shader("indirect");
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
    shader->set_uvec2("screenDimensions", glm::uvec2(scr_width, scr_height));

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

    //
    // skinned
    //
    shader = Shader_Manager::get_shader("skinned");
    shader->use();
    shader->set_mat4("vp", viewproj);
    shader->set_uint("bone", bone);
    //printf("bone: %d\n", bone);

    vao = Model_Manager::get_rigged_vao();
    glBindVertexArray(vao);

    // draw commands and transform
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, draw_command_buffer_skinned);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, per_object_ssbo_skinned);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, Model_Manager::get_skinned_bone_ssbo());

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, draw_commands_skinned.size(), 0);

    glBindVertexArray(0);
}

void Renderer::render(Player& player, Scene& scene, float delta_time, SSBO& particles) {
    // begin frame
    {
        PROFILE_SCOPE_COLOR("build commands", legit::Colors::wisteria);
        build_command_buffer(player, scene, delta_time);
    }
    {
        PROFILE_SCOPE_COLOR("shadows", legit::Colors::pomegranate);
        if (shadows_enabled)
            shadow_pass(scene, player);
    }
    {
        PROFILE_SCOPE_COLOR("depth pre-pass", legit::Colors::belizeHole);
        if (use_depth_prepass)
            indirect_depth_prepass(player);
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
        PROFILE_SCOPE_COLOR("submit render commands", legit::Colors::clouds);
        render_indirect(player);
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
    if (do_draw_light_quads)
        draw_light_quads(player);
    {
        PROFILE_SCOPE_COLOR("composite", legit::Colors::turqoise);
        composite();
    }
    // end frame
}

void Renderer::render_debug(Player& player) {
    Shader* shader = Shader_Manager::get_shader("debug");
    //glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    //glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height);
    glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height, player.get_camera_zoom());

    shader->set_mat4("projection", projection);
    glm::mat4 view = player.get_view_matrix();
    shader->set_mat4("view", view);

    debug_renderer.render(shader, projection, view, num_lights);
}

void Renderer::particle_pass(float delta_time, SSBO& particle_ssbo, Player& player) {
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_DEPTH_TEST);
    const int MAX_PARTICLES = 10000;
    
    Compute_Shader* particle = Shader_Manager::get_compute("particle2");

    particle->use();
    particle->set_float("dt", delta_time);
    particle->set_vec3("emitter_position", emitter_position);
    particle->set_vec3("acceleration_direction", acceleration_direction);
    particle->set_float("acceleration_force", acceleration_force);
    particle->set_vec2("life_range", life_range);
    particle->set_vec4("color_start_base", color_start_base);
    particle->set_vec4("color_end_base", color_end_base);
    particle->set_vec3("velocity_base", velocity_base);
    particle->set_vec3("velocity_random_bias", velocity_random_bias);
    particle->set_float("velocity_mag", velocity_mag);
    particle->set_float("emission_rate", emission_rate);
    particle->set_int("max_particle", MAX_PARTICLES);

    particle_ssbo.bind(0);
    glDispatchCompute((MAX_PARTICLES + 127) / 128, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height, player.get_camera_zoom());

    Shader* shader = Shader_Manager::get_shader("particle");
    shader->use();
    shader->set_mat4("view", player.get_view_matrix());
    shader->set_mat4("projection", projection);
    particle_ssbo.bind(0);
    glBindVertexArray(quadVAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, MAX_PARTICLES);
    
    //glEnable(GL_DEPTH_TEST);
}

void Renderer::draw_light_quads(Player& player) {
    //glDisable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);

    glm::mat4 projection = player.camera.get_projection((float)scr_width / (float)scr_height, player.get_camera_zoom());

    Shader* shader = Shader_Manager::get_shader("light_quad_debug");
        
    shader->use();
    shader->set_mat4("view", player.get_view_matrix());
    shader->set_mat4("projection", projection);

    light_ssbo.bind(0);

    glBindVertexArray(quadVAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, num_lights);
    glBindVertexArray(0);
 
    //glEnable(GL_DEPTH_TEST);
}

void Renderer::bloom_pass() {
    uint32_t texture_id = Texture_Manager::get_ogl_id(bright_texture);

    Compute_Shader* bloom_down = Shader_Manager::get_compute("bloomdown");
    Compute_Shader* bloom_up = Shader_Manager::get_compute("bloomup");

    const int MIP_LEVELS = 6;

    for (int i = 1; i < MIP_LEVELS; i++) {
        int mip_width = std::max(1, scr_width >> i);
        int mip_height = std::max(1, scr_height >> i);

        // prev mip input
        glBindImageTexture(0, texture_id, i - 1, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        // current mip output
        glBindImageTexture(1, texture_id, i, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        bloom_down->use();
        int groups_x = (mip_width + 7) / 8;
        int groups_y = (mip_height + 7) / 8;
        bloom_down->dispatch_and_wait(groups_x, groups_y, 1, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    for (int i = MIP_LEVELS - 2; i >= 0; i--) {
        int mip_width = std::max(1, scr_width >> i);
        int mip_height = std::max(1, scr_height >> i);

        // bind current mip
        glBindImageTexture(0, texture_id, i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        // bind higher res mip
        glBindImageTexture(1, texture_id, i + 1, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

        bloom_up->use();
        int groups_x = (mip_width + 7) / 8;
        int groups_y = (mip_height + 7) / 8;
        bloom_up->dispatch_and_wait(groups_x, groups_y, 1, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}

void Renderer::ssao_pass(Player& player) {
    Compute_Shader* ssao = Shader_Manager::get_compute("ssao");

    ssao->use();

    Texture_Manager::bind(depth_texture, 0); // todo maybe dont need to
    Texture_Manager::bind(ssao_noise_texture, 1);
    //Texture_Manager::bind(ssao_texture, 2);
    //glBindImageTexture(2, Texture_Manager::get_ogl_id(ssao_texture), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
    glBindImageTexture(2, Texture_Manager::get_ogl_id(ssao_texture), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    glm::mat4 projection = player.camera.get_projection((float)scr_width / (float)scr_height, player.get_camera_zoom());

    ssao->set_mat4("projection", projection);
    ssao->set_mat4("inverse_projection", glm::inverse(projection));
    ssao->set_vec2("screen_size", glm::vec2(scr_width, scr_height));
    ssao->set_float("radius", ssao_radius);
    ssao->set_float("bias", ssao_bias);
    ssao->set_int("sample_count", ssao_samples);
    ssao->set_float("min_depth", min_depth);
    ssao->set_float("power", power);

    ssao->dispatch_and_wait((scr_width + 15) / 16, (scr_height + 15) / 16, 1, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}


void Renderer::composite() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to default framebuffer
    glViewport(0, 0, scr_width, scr_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    Shader* shader = Shader_Manager::get_shader("quad");
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
    Shader* shader = Shader_Manager::get_shader("skybox");
    shader->use();

    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

    shader->set_mat4("view", viewNoTranslation);
    shader->set_mat4("projection", projection);

    skybox.draw();
}

void Renderer::render_crosshair(const Crosshair& crosshair) {
    Shader* s = Shader_Manager::get_shader("crosshair");
    s->use();
    crosshair.draw(s, scr_width, scr_height);
}

void Renderer::render_hud_text(const Text& text) {
    glm::mat4 projection = glm::ortho(0.0f, (float)scr_width, 0.0f, (float)scr_height);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader* shader = Shader_Manager::get_shader("hud_text");
    text.draw(shader, projection);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Renderer::imgui_pass() {
    ImGui::Begin("Renderer Settings");

    ImGui::Checkbox("depth pre-pass", &use_depth_prepass);
    ImGui::Checkbox("shadows enabled", &shadows_enabled);
    ImGui::SliderInt("num_lights", &num_lights, 0, 1000);
    ImGui::Checkbox("forward+", &forward_plus);
    ImGui::Checkbox("bloom_enabled", &bloom_enabled);
    ImGui::Checkbox("ssao_enabled", &ssao_enabled);
    ImGui::SliderFloat("ssao_radius", &ssao_radius, 0, 5.0);
    ImGui::SliderFloat("ssao_bias", &ssao_bias, 0, 1.0f);
    ImGui::SliderInt("ssao_samples", &ssao_samples, 0, 64);
    ImGui::SliderFloat("min_depth", &min_depth, -0.01, 0.2f);
    ImGui::SliderFloat("power", &power, -2, 4);
    ImGui::Checkbox("light quads", &do_draw_light_quads);

    ImGui::End();
}

void Renderer::shutdown() {
    glDeleteFramebuffers(1, &render_target);
    glDeleteVertexArrays(1, &quadVAO);
}
