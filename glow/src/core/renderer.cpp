#include "renderer.h"

#include "glow_config.h"

#include "core/opengl.h"

#include "asset/material_manager.h"
#include "asset/model_manager.h"

#include "util/frustum.h"
#include "util/colors.h"
#include "util/profiler.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <cstdint>
#include <cstddef>
#include <ctime>
#include <cfloat>
#include <iostream>
#include <algorithm>
#include <random>

const float FAR_PLANE = 1000.0f;
const uint32_t MAX_DRAW_COMMANDS = 8000;

const uint32_t NUM_CASCADE = 4;
const float CASCADE_SIZE = 50.0f;
const glm::vec3 SUN_DIR = glm::vec3(0.0, -1.0f, -1.0f);
static glm::mat4 cascade_mats[NUM_CASCADE] = { 0 };
const float CASCADE_END[NUM_CASCADE + 1] = { 1.0f, 25.0f, 50.0f, 100.0f, 200.0f };

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

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glEnable(GL_CULL_FACE);

    //glEnable(GL_STENCIL_TEST);
    //glDisable(GL_STENCIL_TEST);
    //glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    //glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);


    // TODO MOVE TO SCENE
    // PULL LIGHTS FROM MODELS AS WELL
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

    // MOVE TO SCENE
    light_ssbo.init();
    light_ssbo.set_data(sizeof(GPU_Light) * 1000, lights.data(), GL_DYNAMIC_DRAW);

    // clusters prob stay here
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
    setup_buffers(); // todo maybe idk organize

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

    Shader_Manager::load_compute("cull_mesh");
    Shader_Manager::load_compute("clear_dirty");

    debug_renderer.init();

    setup_ssao(); // organize

    //glGenFramebuffers(1, &csm_fbo);
    //csm_texture = Texture_Manager::create_2d_array_texture(2048, 2048, 4);
    //glBindFramebuffer(GL_FRAMEBUFFER, csm_fbo);
    //glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, Texture_Manager::get_ogl_id(csm_texture), 0);
    //glDrawBuffer(GL_NONE);
    //glReadBuffer(GL_NONE);
    //int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    //if (status != GL_FRAMEBUFFER_COMPLETE) {
    //    std::cout << "ERROR::FRAMEBUFFER:: CSM FBO is not complete!";
    //    assert(false);
    //}
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // organize, maybe scene should control params for CSM
    glGenFramebuffers(1, &csm_fbo);
    csm_texture = Texture_Manager::create_2d_array_texture(2048, 2048, NUM_CASCADE);
    glBindFramebuffer(GL_FRAMEBUFFER, csm_fbo);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, Texture_Manager::get_ogl_id(csm_texture), 0);
    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::FRAMEBUFFER:: CSM FBO is not complete!";
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    csm_draw_commands.resize(NUM_CASCADE);
    csm_per_object_data.resize(NUM_CASCADE);
    // todo maybe move to scene


    Shader_Manager::load_from_paths("fullscreen_texture", "quad_v.glsl", "quad_texture.glsl");

    Shader_Manager::load_tesselation("terrain");

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

    // update per shader "constant-ish" uniforms (screen size, etc)
}

void Renderer::setup_indirect() {
    Shader_Manager::load_from_paths("indirect", "vertex_ind_v.glsl", "fragment_ind_f.glsl");

    opaque_draw_commands.reserve(MAX_DRAW_COMMANDS); // todo change draw command per entity prob
    opaque_object_data.reserve(MAX_DRAW_COMMANDS);

    glCreateBuffers(1, &opaque_draw_command_ssbo);
    glNamedBufferStorage(opaque_draw_command_ssbo, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glCreateBuffers(1, &opaque_object_ssbo);
    glNamedBufferStorage(opaque_object_ssbo, sizeof(Per_Object_Data) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);

    // // // 
    glCreateBuffers(1, &compute_culled_commands);
    glNamedBufferStorage(compute_culled_commands, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);    
    
    glCreateBuffers(1, &csm_commands);
    glNamedBufferStorage(csm_commands, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &num_commands);
    glNamedBufferStorage(num_commands, sizeof(uint32_t) * 2, 0, GL_DYNAMIC_STORAGE_BIT);
    // // //

    blended_draw_commands.reserve(MAX_DRAW_COMMANDS);
    blended_object_data.reserve(MAX_DRAW_COMMANDS);
    blended_draw_command_indices.resize(MAX_DRAW_COMMANDS);
    blended_draw_command_distances.resize(MAX_DRAW_COMMANDS);
    
    glCreateBuffers(1, &blended_draw_command_ssbo);
    glNamedBufferStorage(blended_draw_command_ssbo, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glCreateBuffers(1, &blended_object_ssbo);
    glNamedBufferStorage(blended_object_ssbo, sizeof(Per_Object_Data) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);


    // #if COMPUTE_SKINNG load skinned compute shader (does boen transformation)
    Shader_Manager::load_from_name("skinned");
    skinned_draw_commands.reserve(MAX_DRAW_COMMANDS);
    skinned_object_data.reserve(MAX_DRAW_COMMANDS);

    // todo add actual skinning compute shader

    glCreateBuffers(1, &skinned_draw_commands_ssbo);
    glNamedBufferStorage(skinned_draw_commands_ssbo, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glCreateBuffers(1, &skinned_object_ssbo);
    glNamedBufferStorage(skinned_object_ssbo, sizeof(Per_Object_Data) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void Renderer::setup_ssao() {
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

void Renderer::build_cluster_pass(const glm::mat4& inv_proj) {
    Compute_Shader* cluster_build = Shader_Manager::get_compute("cluster");
    cluster_build->use();
    cluster_ssbo.bind(1); // todo fix once

    cluster_build->set_float("zNear", 1.0f); // once
    cluster_build->set_float("zFar", FAR_PLANE); // once (would have to change if changed)
    cluster_build->set_mat4("inverseProjection", inv_proj);
    cluster_build->set_uvec3("gridSize", glm::uvec3(16, 9, 24)); // thnk about how to do x y
    cluster_build->set_uvec2("screenDimensions", glm::uvec2(scr_width, scr_height)); // once + on change

    uint32_t groups_x = (16 + 7) / 8;
    uint32_t groups_y = (9 + 7) / 8;
    uint32_t groups_z = (24 + 7) / 8;

    cluster_build->dispatch_and_wait(1, 1, 24, GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::cull_cluster_pass(const glm::mat4& view) {
    Compute_Shader* cluster_cull = Shader_Manager::get_compute("cluster_cull");
    cluster_cull->use();

    cluster_ssbo.bind(1); // bind once
    light_ssbo.bind(2); // bind once

    cluster_cull->set_mat4("viewMatrix", view);
    cluster_cull->set_int("num_lights", num_lights); // maybe frequently changing?

    cluster_cull->dispatch_and_wait(27, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::shadow_setup(const glm::mat4& view, const glm::mat4& inv_view, const float& aspect_ratio, const float& zoom) {
    glm::mat4 sun_mat = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::normalize(SUN_DIR), glm::vec3(0.0f, 1.0f, 0.0f));

    //float tanHalfVFOV = tanf(glm::radians(player.camera.zoom / 2.0f));
    //float tanHalfHFOV = tanHalfVFOV * aspect_ratio;

    float tanHalfHFOV = tanf(glm::radians(zoom / 2.0f));
    float tanHalfVFOV = tanf(glm::radians((zoom * aspect_ratio) / 2.0f));

    //printf("ar %f tanHalfHFOV %f tanHalfVFOV %f\n", ar, tanHalfHFOV, tanHalfVFOV);

    for (uint32_t i = 0; i < NUM_CASCADE; i++) {
        float xn = CASCADE_END[i] * tanHalfHFOV;
        float xf = CASCADE_END[i + 1] * tanHalfHFOV;
        float yn = CASCADE_END[i] * tanHalfVFOV;
        float yf = CASCADE_END[i + 1] * tanHalfVFOV;

        //printf("xn %f xf %f\n", xn, xf);
        //printf("yn %f yf %f\n", yn, yf);

        glm::vec4 frustumCorners[8] = {
            // near face
            glm::vec4(xn,   yn, -CASCADE_END[i], 1.0),
            glm::vec4(-xn,  yn, -CASCADE_END[i], 1.0),
            glm::vec4(xn,  -yn, -CASCADE_END[i], 1.0),
            glm::vec4(-xn, -yn, -CASCADE_END[i], 1.0),

            // far face
            glm::vec4(xf,   yf, -CASCADE_END[i + 1], 1.0),
            glm::vec4(-xf,  yf, -CASCADE_END[i + 1], 1.0),
            glm::vec4(xf,  -yf, -CASCADE_END[i + 1], 1.0),
            glm::vec4(-xf, -yf, -CASCADE_END[i + 1], 1.0)
        };

        //glm::vec4 frustumCornersL[8];

        glm::vec4 min = glm::vec4(FLT_MAX);
        glm::vec4 max = glm::vec4(-FLT_MAX);

        for (uint32_t j = 0; j < 8; j++) {
            //printf("Frustum: ");
            glm::vec4 vW = inv_view * frustumCorners[j];
            //printf("Light space: ");
            //frustumCornersL[j] = sun_mat * vW;
            //frustumCornersL[j].Print();
            //printf("\n");
            glm::vec4 corner = sun_mat * vW;

            min = glm::min(min, corner);
            max = glm::max(max, corner);
        }

        //glm::vec3 box_size = glm::vec3(max) - glm::vec3(min);

        //float texel_size_x = box_size.x / 2048;
        //float texel_size_y = box_size.y / 2048;
        //min.x = floor(min.x / texel_size_x) * texel_size_x;
        //min.y = floor(min.y / texel_size_y) * texel_size_y;
        //
        //max.x = min.x + box_size.x;
        //max.y = min.y + box_size.y;

        max += 5.0f;
        min -= 5.0f;

        //min *= 1.5;
        //max *= 1.5;
        //printf("BB: %f %f %f %f %f %f\n", min.x, max.x, min.y, max.y, min.z, max.z);
        // draw aabb?
        //cascade_mats[i] = glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z) * sun_mat;
        cascade_mats[i] = glm::ortho(min.x, max.x, min.y, max.y, max.z, min.z) * sun_mat;

        glm::mat4 inv_sun_mat = glm::inverse(sun_mat);
        glm::vec3 light_corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z}
        };
        glm::vec3 minW(FLT_MAX), maxW(-FLT_MAX);
        for (auto& c : light_corners) {
            glm::vec4 w = inv_sun_mat * glm::vec4(c, 1.0f);
            minW = glm::min(minW, glm::vec3(w));
            maxW = glm::max(maxW, glm::vec3(w));
        }
        debug_renderer.add_bbox(minW, maxW, Util::cyan);
    }
}

void Renderer::shadow_pass(Scene& scene) {
    //p.SetCamera(Vector3f(0.0f, 0.0f, 0.0f), m_dirLight.Direction, Vector3f(0.0f, 1.0f, 0.0f));
    //GLuint csm_count;
    //glGetBufferSubData(GL_PARAMETER_BUFFER, 4, sizeof(GLuint), &csm_count);
    //printf("CSM Draw count: %u\n", csm_count);

    Shader* shader = Shader_Manager::get_shader("indirect_depth_prepass");
    shader->use();

    glBindFramebuffer(GL_FRAMEBUFFER, csm_fbo);
    glViewport(0, 0, 2048, 2048);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_TRUE);
    uint32_t vao = Model_Manager::get_big_vao();
    glBindVertexArray(vao);

    glBindBuffer(GL_PARAMETER_BUFFER, num_commands);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, csm_commands);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scene.per_mesh_ssbo);

    glDisable(GL_BLEND);

    for (uint32_t i = 0; i < NUM_CASCADE; i++) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            Texture_Manager::get_ogl_id(csm_texture),
            0,        // mip level
            i);       // texture array layer

        glClear(GL_DEPTH_BUFFER_BIT);

        /*glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);*/

        shader->set_mat4("vp", cascade_mats[i]);

    #if BINDLESS
        //glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, opaque_draw_count, 0);

        glMultiDrawElementsIndirectCount(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            (void*)0,                             // indirect offset
            (GLintptr)4,                          // offset in the count buffer
            MAX_DRAW_COMMANDS,                                 // maximum draws
            sizeof(Draw_Elements_Indirect_Command)  // stride
        );

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    #else
        //for (size_t i = 0; i < draw_commands.size(); i++) {
        //    Draw_Elements_Indirect_Command cmd = draw_commands[i];
        //    Per_Object_Data pod = per_object_data[i];

        //    //Texture_Manager::bind(pod.albedo, 0);
        //    //Texture_Manager::bind(pod.normal, 1);
        //    //Texture_Manager::bind(pod.met_rough, 2);
        //    //Texture_Manager::bind(pod.emissive, 3);
        //    //Texture_Manager::bind(pod.amb_occ, 4);
        //    shader->set_uint("draw_id", i);

        //    glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES,
        //        cmd.count,
        //        GL_UNSIGNED_INT,
        //        (void*)(cmd.first_index * sizeof(uint32_t)),
        //        cmd.instance_count,
        //        cmd.base_vertex,
        //        cmd.base_instance);
        //}
    #endif
    }

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // atlas
}

//void Renderer::build_command_buffer(Player& player, Scene& scene, float delta_time) {
//    Util::Frustum frustum(player.camera.position, player.camera.front, player.camera.right, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
//
//    draw_commands.clear();
//    per_object_data.clear();
//    uint32_t current_draw_count = 0;
//    draw_commands_blended.clear();
//    per_object_data_blended.clear();
//    blended_draw_command_indices.clear();
//    blended_draw_command_distances.clear();
//    uint32_t blended_draw_count = 0;
//
//    for (Entity& entity : scene.entities) {
//
//        Model mind = Model_Manager::get_model_ind(entity.model_id);
//
//        Util::AABB model_aabb = Util::transform_aabb(mind.m_aabb, entity.get_model_matrix());
//        debug_renderer.add_bbox(model_aabb.min, model_aabb.max, glm::vec3(1.0f, 1.0f, 1.0f));
//
//        // if culled
//        if (!frustum.intersectsAABB(model_aabb, true))
//            continue;
//
//        for (uint32_t i = 0; i < mind.m_meshes.size(); i++) {
//            if (current_draw_count >= MAX_DRAW_COMMANDS) {
//                printf("Warning: Exceeded max draw commands!\n");
//                break;
//            }
//            Per_Object_Data obj_data;
//
//            obj_data.model_matrix = entity.get_model_matrix() * mind.m_meshes[i].transform;
//            Util::AABB obj_aabb = Util::transform_aabb(mind.m_meshes[i].aabb, entity.get_model_matrix());
//
//            if (!frustum.intersectsAABB(obj_aabb, true)) {
//                debug_renderer.add_bbox(obj_aabb.min, obj_aabb.max, glm::vec3(1.0f, 0.0f, 1.0f));
//                continue;
//            }
//            debug_renderer.add_bbox(obj_aabb.min, obj_aabb.max, glm::vec3(0.0f, 1.0f, 0.0f));
//
//            // if culled
//            // continue
//
//            Draw_Elements_Indirect_Command draw_command;
//            draw_command.count = mind.m_meshes[i].index_count;
//            draw_command.instance_count = 1;
//            draw_command.first_index = mind.m_meshes[i].base_index;
//            draw_command.base_vertex = mind.m_meshes[i].base_vertex;
//            draw_command.base_instance = current_draw_count;
//
//            obj_data.normal_matrix = glm::transpose(glm::inverse(obj_data.model_matrix));
//            //obj_data.color = glm::vec4(0.0, 0.25, 0.5, 0.75);
//            const Material& mater = mind.m_meshes[i].material;
//            obj_data.albedo = mater.albedo;
//            obj_data.normal = mater.normal;
//            obj_data.met_rough = mater.met_rough;
//            obj_data.emissive = mater.emissive;
//            obj_data.amb_occ = mater.amb_occ;
//            obj_data.emissive_factor = mater.emissive_factor;
//            obj_data.metallic_factor = mater.metallic_factor; // 4
//            obj_data.roughness_factor = mater.roughness_factor; // 4
//            obj_data.base_color = mater.base_color;
//            obj_data.alpha_cutoff = mater.alpha_cutoff;
//
//            // if opaque / alpha mask
//            if (mater.blend_mode == Blend_Mode::disabled) {
//                draw_commands.push_back(draw_command);
//                per_object_data.push_back(obj_data);
//                opaque_draw_count++;
//            }
//            else { // assume non additive blending for now
//                draw_commands_blended.push_back(draw_command);
//                per_object_data_blended.push_back(obj_data);
//                blended_draw_command_indices.push_back(blended_draw_count);
//
//                glm::vec3 aabb_center = (mind.m_meshes[i].aabb.max + mind.m_meshes[i].aabb.min) * 0.5f;
//                glm::vec3 world_center = glm::vec3(obj_data.model_matrix * glm::vec4(aabb_center, 1.0f));
//                blended_draw_command_distances.push_back(glm::distance(player.camera.position, world_center));
//                blended_draw_count++;
//            }
//
//        }
//
//        // check if entity interescts each cascade
//        // add to cascade command buffer + per obj data
//    }
//
//    if (current_draw_count > 0) {
//        glNamedBufferSubData(draw_command_buffer, 0, sizeof(Draw_Elements_Indirect_Command) * current_draw_count, draw_commands.data());
//        glNamedBufferSubData(per_object_ssbo, 0, sizeof(Per_Object_Data) * current_draw_count, per_object_data.data());
//    }
//
//    if (player.out_of_body) {
//        debug_renderer.draw_frustum(player.camera.position, player.camera.front, player.camera.up, glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE, Util::red);
//        // printf("out\n");
//    }
//
//    //
//    // skinned commands
//    //
//
//
//    draw_commands_skinned.clear();
//    per_object_data_skinned.clear();
//    current_draw_count = 0;
//    
//    for (uint32_t m = 0; m < Model_Manager::get_num_animated_models(); m++){
//        Animated_Model mind = Model_Manager::get_animated_model(m);
//        //debug_renderer.add_bbox(mind.m_aabb.min, mind.m_aabb.max, glm::vec3(1.0f, 0.0f, 0.0f));
//
//        for (uint32_t i = 0; i < mind.m_meshes.size(); i++) {
//            Per_Object_Data obj_data = { 0 };
//
//            obj_data.model_matrix = glm::translate(mind.m_meshes[i].transform, glm::vec3(5.0, 0.0, 0.0));
//
//            Draw_Elements_Indirect_Command draw_command;
//            draw_command.count = mind.m_meshes[i].index_count;
//            draw_command.instance_count = 1;
//            draw_command.first_index = mind.m_meshes[i].base_index;
//            draw_command.base_vertex = mind.m_meshes[i].base_vertex;
//            draw_command.base_instance = current_draw_count;
//
//            draw_commands_skinned.push_back(draw_command);
//
//            //obj_data.normal_matrix = glm::transpose(glm::inverse(obj_data.model_matrix));
//            //obj_data.color = glm::vec4(0.0, 0.25, 0.5, 0.75);
//            const Material& mater = mind.m_meshes[i].material;
//            obj_data.albedo = mater.albedo;
//            obj_data.normal = mater.normal;
//            obj_data.met_rough = mater.met_rough;
//            obj_data.emissive = mater.emissive;
//            obj_data.amb_occ = mater.amb_occ;
//            obj_data.emissive_factor = mater.emissive_factor;
//            obj_data.metallic_factor = mater.metallic_factor; // 4
//            obj_data.roughness_factor = mater.roughness_factor; // 4
//            obj_data.base_color = mater.base_color;
//            ////obj_data.alpha_cutoff = mater.alpha_cutoff;
//            //obj_data.alpha_cutoff = 0.5;
//
//            obj_data.bone_offset = mind.bone_offset;
//
//            per_object_data_skinned.push_back(obj_data);
//
//            current_draw_count++;
//        }
//    }
//
//    if (current_draw_count > 0) {
//        glNamedBufferSubData(draw_command_buffer_skinned, 0, sizeof(Draw_Elements_Indirect_Command) * opaque_draw_count, draw_commands_skinned.data());
//        glNamedBufferSubData(per_object_ssbo_skinned, 0, sizeof(Per_Object_Data) * blended_draw_count, per_object_data_skinned.data());
//    }
//
//}

void Renderer::indirect_depth_prepass(const glm::mat4& viewproj) {
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);
    glEnable(GL_DEPTH_TEST); // should be on already todo remove maybe

    // pre pass state
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    Shader* shader = Shader_Manager::get_shader("indirect_depth_prepass");
    shader->use();
    shader->set_mat4("vp", viewproj);

    uint32_t vao = Model_Manager::get_big_vao();
    glBindVertexArray(vao);

    // draw commands and transform
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, opaque_object_ssbo);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, opaque_draw_command_ssbo);

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, opaque_draw_count, 0);

    glBindVertexArray(0);
}

void Renderer::compute_cull_draw(Scene& scene, const glm::vec3& view_pos, const glm::mat4& view, const glm::mat4& viewproj, const glm::mat4& cull_view, const glm::mat4& cull_proj) {
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

    Shader* shader = Shader_Manager::get_shader("indirect");
    shader->use();
    //glStencilMask(0x00);
    Texture_Manager::bind(ssao_texture, 0); // todo once
    Texture_Manager::bind_array(csm_texture, 1); // todo once

    scene.skybox.bind(9);
    shader->set_uint("num_skybox_mips", scene.skybox.num_mips);

    shader->set_mat4("vp", viewproj);

    shader->set_bool("blend", false);

    shader->set_vec3("view_pos", view_pos);
    shader->set_int("num_lights", num_lights);
    shader->set_bool("forward_plus", forward_plus);
    shader->set_float("zNear", 1.0f);
    shader->set_float("zFar", FAR_PLANE);
    shader->set_mat4("viewMatrix", view);
    shader->set_uvec3("gridSize", glm::uvec3(16, 9, 24));
    shader->set_uvec2("screenDimensions", glm::uvec2(scr_width, scr_height));

    shader->set_bool("ssao_enabled", ssao_enabled);

    shader->set_mat4_array("cascade_matrices", cascade_mats, NUM_CASCADE);
    shader->set_float_array("cascade_distances", CASCADE_END, NUM_CASCADE + 1);
    shader->set_int("num_cascades", NUM_CASCADE);
    shader->set_vec3("directional_light_direction", SUN_DIR);
    shader->set_vec3("directional_light_color", glm::vec3(1.0f));
    shader->set_float("directional_light_intensity", sun_strength);

    Texture_Manager::bind(ssao_texture, 0); // todo once
    Texture_Manager::bind_array(csm_texture, 1); // todo once

    glDisable(GL_BLEND);

    uint32_t vao = Model_Manager::get_big_vao();
    glBindVertexArray(vao);

    GLuint count;
    glGetBufferSubData(GL_PARAMETER_BUFFER, 0, sizeof(GLuint), &count);
    printf("Draw count: %u \n", count);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scene.per_mesh_ssbo);
    cluster_ssbo.bind(1);
    light_ssbo.bind(2);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, compute_culled_commands);
    glBindBuffer(GL_PARAMETER_BUFFER, num_commands);

    glMultiDrawElementsIndirectCount(
        GL_TRIANGLES,
        GL_UNSIGNED_INT,
        (void*)0,                             // indirect offset
        (GLintptr)0,                          // offset in the count buffer
        MAX_DRAW_COMMANDS,                                 // maximum draws
        sizeof(Draw_Elements_Indirect_Command)  // stride
    );

    glBindVertexArray(0);

    shader = Shader_Manager::get_shader("terrain");
    shader->use();
    shader->set_mat4("vp", viewproj);
    shader->set_mat4("view", view);
    Texture_Manager::bind(scene.terrain.heightmap, 0);
    Texture_Manager::bind(scene.terrain.heightmap_texture, 1);
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glBindVertexArray(scene.terrain.vao);

    if (terrain_draw_type == 0 || terrain_draw_type == 2) {
        shader->set_bool("lines", false);
        glDrawArrays(GL_PATCHES, 0, scene.terrain.vertex_count);
    }
    if (terrain_draw_type == 1) {
        shader->set_bool("lines", true);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_PATCHES, 0, scene.terrain.vertex_count);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    if (terrain_draw_type == 2) {
        shader->set_bool("lines", true);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_PATCHES, 0, scene.terrain.vertex_count);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBindVertexArray(0);
}

void Renderer::draw(Scene& scene, const glm::mat4& view, const glm::mat4& viewproj, const glm::vec3& view_pos, const glm::mat4& proj) {
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

    Shader* shader = Shader_Manager::get_shader("indirect");
    shader->use();
    //glStencilMask(0x00);
    
    scene.skybox.bind(9);
    shader->set_uint("num_skybox_mips", scene.skybox.num_mips);

    shader->set_mat4("vp", viewproj);

    shader->set_bool("blend", false);

    shader->set_vec3("view_pos", view_pos);
    shader->set_int("num_lights", num_lights);
    shader->set_bool("forward_plus", forward_plus);
    shader->set_float("zNear", 1.0f);
    shader->set_float("zFar", FAR_PLANE);
    shader->set_mat4("viewMatrix", view);
    shader->set_uvec3("gridSize", glm::uvec3(16, 9, 24));
    shader->set_uvec2("screenDimensions", glm::uvec2(scr_width, scr_height));

    shader->set_bool("ssao_enabled", ssao_enabled);

    shader->set_mat4_array("cascade_matrices", cascade_mats, NUM_CASCADE);
    shader->set_float_array("cascade_distances", CASCADE_END, NUM_CASCADE + 1);
    shader->set_int("num_cascades", NUM_CASCADE);
    shader->set_vec3("directional_light_direction", SUN_DIR);
    shader->set_vec3("directional_light_color", glm::vec3(1.0f));
    shader->set_float("directional_light_intensity", sun_strength);

    Texture_Manager::bind(ssao_texture, 0); // todo once
    Texture_Manager::bind_array(csm_texture, 1); // todo once


    //static float time = 0.0f;
    //time += delta_time;

    // main pass
#if BINDLESS
    glDisable(GL_BLEND);

    uint32_t vao = Model_Manager::get_big_vao();
    glBindVertexArray(vao);

    // draw commands and transform
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, opaque_draw_command_ssbo);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, opaque_object_ssbo);
    cluster_ssbo.bind(1);
    light_ssbo.bind(2);

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, opaque_draw_count, 0);

    glBindVertexArray(0);
    //
    // skinned
    //
    shader = Shader_Manager::get_shader("skinned");
    shader->use();
    shader->set_mat4("vp", proj);
    //shader->set_uint("bone", bone);
    //printf("bone: %d\n", bone);

    vao = Model_Manager::get_rigged_vao();
    glBindVertexArray(vao);

    // draw commands and transform
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, skinned_draw_commands_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, skinned_object_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, Model_Manager::get_skinned_bone_ssbo());

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, skinned_draw_commands.size(), 0);

    glBindVertexArray(0);

    // blended stuff
    shader = Shader_Manager::get_shader("indirect");
    shader->use();
    shader->set_bool("blend", true);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    vao = Model_Manager::get_big_vao();
    glBindVertexArray(vao);

    // draw commands and transform
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, blended_draw_command_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, blended_object_ssbo);

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, blended_draw_count, 0);

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);

#else
    //uint32_t vao = Model_Manager::get_big_vao();
    //glBindVertexArray(vao);

    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, per_object_ssbo);
    //cluster_ssbo.bind(1);
    //light_ssbo.bind(2);
    //Texture_Manager::bind(ssao_texture, 0);

    //for (size_t i = 0; i < draw_commands.size(); i++) {
    //    Draw_Elements_Indirect_Command cmd = draw_commands[i];
    //    Per_Object_Data pod = per_object_data[i];

    //    Texture_Manager::bind(pod.albedo, 0);
    //    Texture_Manager::bind(pod.normal, 1);
    //    Texture_Manager::bind(pod.met_rough, 2);
    //    Texture_Manager::bind(pod.emissive, 3);
    //    Texture_Manager::bind(pod.amb_occ, 4);
    //    shader->set_uint("draw_id", i);

    //    glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES,
    //                                        cmd.count,
    //                                        GL_UNSIGNED_INT,
    //                                        (void*)(cmd.first_index * sizeof(uint32_t)),
    //                                        cmd.instance_count,
    //                                        cmd.base_vertex,
    //                                        cmd.base_instance);
    //}
    //glBindVertexArray(0);
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //shader = Shader_Manager::get_shader("skinned");
    //shader->use();
    //shader->set_mat4("vp", viewproj);
    //shader->set_uint("bone", bone);
    ////printf("bone: %d\n", bone);

    //vao = Model_Manager::get_rigged_vao();
    //glBindVertexArray(vao);

    //// draw commands and transform
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, per_object_ssbo_skinned);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, Model_Manager::get_skinned_bone_ssbo());

    //for (size_t i = 0; i < draw_commands_skinned.size(); i++) {
    //    Draw_Elements_Indirect_Command cmd = draw_commands_skinned[i];
    //    Per_Object_Data pod = per_object_data_skinned[i];

    //    Texture_Manager::bind(pod.albedo, 0);
    //    Texture_Manager::bind(pod.normal, 1);
    //    Texture_Manager::bind(pod.met_rough, 2);
    //    Texture_Manager::bind(pod.emissive, 3);
    //    Texture_Manager::bind(pod.amb_occ, 4);
    //    shader->set_uint("draw_id", i);

    //    glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES,
    //                                        cmd.count,
    //                                        GL_UNSIGNED_INT,
    //                                        (void*)(cmd.first_index * sizeof(uint32_t)),
    //                                        cmd.instance_count,
    //                                        cmd.base_vertex,
    //                                        cmd.base_instance);
    //}

    //glBindVertexArray(0);
#endif

}

//void Renderer::sort_blended_draws() { // todo move to gpu?
// 
//      barrier command buffer
//      dispatch sort shader
// 
// 
//    // sort draw commands by center of aabb
//    std::sort(blended_draw_command_indices.begin(), blended_draw_command_indices.end(),
//        [&distances = blended_draw_command_distances](size_t d1, size_t d2) {
//            return distances[d1] > distances[d2];
//        });
//
//    // order draw command per obj
//    uint32_t count = blended_draw_commands.size();
//    // TODO I HATE THIS FIND BETTER WAY
//    auto sorted_draw_commands = blended_draw_commands;
//    auto sorted_per_object_data = blended_object_data;
//    for (uint32_t i = 0; i < count; ++i) {
//        blended_draw_commands[i] = sorted_draw_commands[blended_draw_command_indices[i]];
//        blended_object_data[i] = sorted_per_object_data[blended_draw_command_indices[i]];
//    }
//
//    if (count) {
//        glNamedBufferSubData(blended_draw_command_ssbo, 0, sizeof(Draw_Elements_Indirect_Command) * count, blended_draw_commands.data());
//        glNamedBufferSubData(blended_object_ssbo, 0, sizeof(Per_Object_Data) * count, blended_object_data.data());
//    }
//}

void Renderer::debug_cascades(Scene& scene) {

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to default framebuffer
        Shader* shader = Shader_Manager::get_shader("fullscreen_texture");
        shader->use();

        glDisable(GL_DEPTH_TEST);
        Texture_Manager::bind_array(csm_texture, 0);

        shader->set_int("mode", 0);
        for (uint32_t i = 0; i < NUM_CASCADE; i++) {
            int quad_size = scr_width / (float)NUM_CASCADE - (NUM_CASCADE * 10.0f);
            int x = i * (quad_size + 10);
            int y = scr_height - quad_size - 10;

            glViewport(x, y, quad_size, quad_size);

            shader->set_float("cascade_layer", (float)i);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        shader->set_int("mode", 1);
        glViewport(10, 10, 400, 400);
        Texture_Manager::bind(scene.terrain.heightmap, 1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindVertexArray(0); // unbind quad

}

void Renderer::render_debug(const glm::mat4& view, const glm::mat4& proj) {
    Shader* shader = Shader_Manager::get_shader("debug");
    //glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    //glm::mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height);

    shader->set_mat4("projection", proj);
    shader->set_mat4("view", view);

    debug_renderer.render(shader, proj, view, num_lights);
}

void Renderer::particle_pass(float delta_time, SSBO& particle_ssbo, const glm::mat4& proj, const glm::mat4& view) {
    // todo holy hell
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
    
    Shader* shader = Shader_Manager::get_shader("particle");
    shader->use();
    shader->set_mat4("view", view);
    shader->set_mat4("projection", proj);
    particle_ssbo.bind(0);
    glBindVertexArray(quadVAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, MAX_PARTICLES);
    
    //glEnable(GL_DEPTH_TEST);
}

void Renderer::draw_light_quads(const glm::mat4& proj, const glm::mat4& view) {
    //glDisable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);

    Shader* shader = Shader_Manager::get_shader("light_quad_debug");
        
    shader->use();
    shader->set_mat4("view", view);
    shader->set_mat4("projection", proj);

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

void Renderer::ssao_pass(const glm::mat4& proj, const glm::mat4& inv_proj) {
    Compute_Shader* ssao = Shader_Manager::get_compute("ssao");
    ssao->use();

    Texture_Manager::bind(depth_texture, 0); // todo maybe dont need to
    Texture_Manager::bind(ssao_noise_texture, 1);
    //Texture_Manager::bind(ssao_texture, 2);
    //glBindImageTexture(2, Texture_Manager::get_ogl_id(ssao_texture), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
    glBindImageTexture(2, Texture_Manager::get_ogl_id(ssao_texture), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    ssao->set_mat4("projection", proj);
    ssao->set_mat4("inverse_projection", inv_proj);
    ssao->set_vec2("screen_size", glm::vec2(scr_width, scr_height));
    ssao->set_float("radius", ssao_radius);
    ssao->set_float("bias", ssao_bias);
    ssao->set_int("sample_count", ssao_samples);
    ssao->set_float("min_depth", min_depth);
    ssao->set_float("power", power);
    for (uint32_t i = 0; i < samples.size(); i++)
        ssao->set_vec3("samples[" + std::to_string(i) + "]", samples[i]);

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
    glDepthFunc(GL_GEQUAL);

    Shader* shader = Shader_Manager::get_shader("skybox");
    shader->use();

    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

    // todo combine
    shader->set_mat4("view", viewNoTranslation);
    shader->set_mat4("projection", projection);

    skybox.bind(0);
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
    ImGui::Checkbox("light quads", &do_draw_light_quads);
    ImGui::SliderFloat("sun strength", &sun_strength, 0, 500.0);
    ImGui::Checkbox("forward+", &forward_plus);
    ImGui::Checkbox("bloom_enabled", &bloom_enabled);
    ImGui::Checkbox("ssao_enabled", &ssao_enabled);
    ImGui::SliderFloat("ssao_radius", &ssao_radius, 0, 5.0);
    ImGui::SliderFloat("ssao_bias", &ssao_bias, 0, 1.0f);
    ImGui::SliderInt("ssao_samples", &ssao_samples, 0, 64);
    ImGui::SliderFloat("min_depth", &min_depth, -0.01, 0.2f);
    ImGui::SliderFloat("power", &power, -2, 4);

    ImGui::End();
}

void Renderer::shutdown() {
    glDeleteFramebuffers(1, &render_target);
    glDeleteVertexArrays(1, &quadVAO);
}

void Renderer::begin_frame(Scene& scene, const glm::mat4& cull_view, const glm::mat4& cull_proj) {
    opaque_draw_commands.clear(); // todo will be all gpu
    opaque_object_data.clear();
    opaque_draw_count = 0;

    blended_draw_commands.clear(); // todo will be all gpu
    blended_object_data.clear();
    blended_draw_command_indices.clear();
    blended_draw_command_distances.clear();
    blended_draw_count = 0;

    skinned_draw_commands.clear(); // todo will be all gpu
    skinned_object_data.clear();
    skinned_draw_count = 0;

    // dispatch main update + culling shader
    // fills all render commands for meshes in scene
    Compute_Shader* c_shader = Shader_Manager::get_compute("cull_mesh");
    c_shader->use();
    // bind buffers

    GLuint zero = 0;
    glNamedBufferSubData(num_commands, 0, sizeof(GLuint), &zero); // clear draw cmds
    glNamedBufferSubData(num_commands, 4, sizeof(GLuint), &zero); // clear num csm commands

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // barrier for entity updates
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, compute_culled_commands);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, num_commands);
    // entities buffer 2
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, scene.gpu_entity_ssbo);
    // meshes buffer 3
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, scene.gpu_mesh_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, scene.per_mesh_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, csm_commands);

    // set cull uinforms
    glm::mat4 projectionT = glm::transpose(cull_proj);
    glm::vec4 frustumX = normalize_plane(projectionT[3] + projectionT[0]); // x + w < 0
    glm::vec4 frustumY = normalize_plane(projectionT[3] + projectionT[1]); // y + w < 0

    float frustum[4];
    frustum[0] = frustumX.x;
    frustum[1] = frustumX.z;
    frustum[2] = frustumY.y;
    frustum[3] = frustumY.z;
    //printf("Projection[0][0]: %f, [1][1]: %f\n", cull_proj[0][0], cull_proj[1][1]);
    //printf("Frustum values: [%f, %f, %f, %f]\n", frustum[0], frustum[1], frustum[2], frustum[3]);
    uint32_t num_meshes = (uint32_t)scene.gpu_meshes.size();
    c_shader->set_uint("num_meshes", num_meshes);
    c_shader->set_mat4("view", cull_view);
    c_shader->set_float_array("frustum", frustum, 4);
    //c_shader->set_float("znear", 1.0f);
    //c_shader->set_float("zfar", 10000.0f);
    //uniform bool infinite_far;
    c_shader->dispatch_and_wait((num_meshes + 63) / 64, 1, 1, GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    // todo move somewhere else idk
    c_shader = Shader_Manager::get_compute("clear_dirty");
    c_shader->use();

    uint32_t num_entities = scene.gpu_entities.size();
    c_shader->set_uint("num_entities", num_entities);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, scene.gpu_entity_ssbo);

    c_shader->dispatch_and_wait((num_entities + 63) / 64, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
}

//void Renderer::submit_render_command(Draw_Elements_Indirect_Command draw_command, const Per_Object_Data object_data, const Blend_Mode blend_mode, const glm::vec3 view_pos, const Util::AABB aabb) {
//    // if opaque / alpha mask
//    // todo probably sort by shader for special effects?!
//    if (blend_mode == Blend_Mode::disabled) {
//        draw_command.base_instance = opaque_draw_count;
//        opaque_draw_commands.push_back(draw_command);
//        opaque_object_data.push_back(object_data);
//        opaque_draw_count++;
//    }
//    else { // assume non additive blending for now
//        draw_command.base_instance = blended_draw_count;
//        blended_draw_commands.push_back(draw_command);
//        blended_object_data.push_back(object_data);
//        blended_draw_command_indices.push_back(blended_draw_count);
//
//        glm::vec3 aabb_center = (aabb.max + aabb.min) * 0.5f;
//        glm::vec3 world_center = glm::vec3(object_data.model_matrix * glm::vec4(aabb_center, 1.0f));
//        blended_draw_command_distances.push_back(glm::distance(view_pos, world_center));
//        blended_draw_count++;
//    }
//}

//void Renderer::submit_animated_render_command(Draw_Elements_Indirect_Command draw_command, const Per_Object_Data object_data) {
//    draw_command.base_instance = skinned_draw_count;
//    skinned_draw_commands.push_back(draw_command);
//    skinned_object_data.push_back(object_data);
//    skinned_draw_count++;
//}

//void Renderer::upload_render_commands() {
//    if (opaque_draw_count > 0) {
//        glNamedBufferSubData(opaque_draw_command_ssbo, 0, sizeof(Draw_Elements_Indirect_Command) * opaque_draw_count, opaque_draw_commands.data());
//        glNamedBufferSubData(opaque_object_ssbo, 0, sizeof(Per_Object_Data) * opaque_draw_count, opaque_object_data.data());
//    }
//
//    if (blended_draw_count > 0) {
//        glNamedBufferSubData(blended_draw_command_ssbo, 0, sizeof(Draw_Elements_Indirect_Command) * blended_draw_count, blended_draw_commands.data());
//        glNamedBufferSubData(blended_object_ssbo, 0, sizeof(Per_Object_Data) * blended_draw_count, blended_object_data.data());
//    }
//
//    if (skinned_draw_count > 0) {
//        glNamedBufferSubData(skinned_draw_commands_ssbo, 0, sizeof(Draw_Elements_Indirect_Command) * skinned_draw_count, skinned_draw_commands.data());
//        glNamedBufferSubData(skinned_object_ssbo, 0, sizeof(Per_Object_Data) * skinned_draw_count, skinned_object_data.data());
//    }
//
//    // skinned draw count todo prob not will just be in normal buffers
//    //glNamedBufferSubData(per_object_ssbo_skinned, 0, sizeof(Per_Object_Data) * blended_draw_count, per_object_data_skinned.data());
//
//}

//void submit_shadow_command(Draw_Elements_Indirect_Command draw_command, Per_Object_Data object_data, Blend_Mode blend_mode);
//// todo probably move CSM to some kind of light system along with other lights
//int get_cascade_level(const Entity& entity, glm::mat4 view); // returns which cascade an object belongs to, -1 if no cascade

glm::vec4 Renderer::normalize_plane(glm::vec4 p) {
    return p / glm::length(glm::vec3(p));
}
