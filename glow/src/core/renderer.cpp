#include "renderer.h"

#include "glm/ext/matrix_clip_space.hpp"
#include "glow_config.h"

#include "core/opengl.h"
#include "core/scene.h"

#include "asset/material_manager.h"
#include "asset/model_manager.h"
#include "asset/particle_manager.h"

#include "util/frustum.h"
#include "util/colors.h"
#include "util/profiler.h"

#include <imgui.h>

#include <cstdint>
#include <cstddef>
#include <ctime>
#include <cfloat>
#include <iostream>
#include <algorithm>
#include <random>

const float FAR_PLANE = 1000.0f; // todo gross
const uint32_t MAX_DRAW_COMMANDS = 8000;

// num cascade from scene
static mat4 cascade_mats[NUM_CASCADE] = { 0 }; // todo figure out where this goes. prob here maybe
const int csm_res = 4096;

// point light shadow mapping
//struct camera_dir {
//    GLenum face;
//    vec3 direction;
//    vec3 up;
//};

//static camera_dir camera_directions[] = {
//    { GL_TEXTURE_CUBE_MAP_POSITIVE_X, vec3(1.0f, 0.0f, 0.0f),  vec3(0.0f, 1.0f, 0.0f) },
//    { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f) },
//    { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, vec3(0.0f, 1.0f, 0.0f),  vec3(0.0f, 0.0f, -1.0f) },
//    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f) },
//    { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, vec3(0.0f, 0.0f, 1.0f),  vec3(0.0f, 1.0f, 0.0f) },
//    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f) }
//};
// point light shadow mapping

int Renderer::init() {
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

    debug_renderer.init();

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
    Texture_Manager::resize(output_texture, scr_width, scr_height);
    Texture_Manager::resize(picking_texture, scr_width, scr_height);
    Texture_Manager::resize(moment0_texture, scr_width, scr_height);
    Texture_Manager::resize(moment1_texture, scr_width, scr_height);

    // update per shader "constant-ish" uniforms (screen size, etc)
}

void Renderer::shutdown() {
    // todo wtf is this Lol
    glDeleteFramebuffers(1, &render_target);
    glDeleteVertexArrays(1, &quadVAO);

    debug_renderer.shutdown();
}

void Renderer::setup() {
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
            vec4(x, y, z, 105.0f),          // position + radius (attenuation range)
            vec4(r, g, b, 100.0f),          // color (white) + intensity
            vec4(0.0f, 0.0f, 0.0f, 0.0f),             // direction unused + type (0 = point light)
            vec4(0.0f, 0.0f, 0.0f, 0.0f)           // unused params for point light
        };
        lights.emplace_back(point_light2);
    }
    // MOVE TO SCENE
    light_ssbo.init();
    light_ssbo.set_data(sizeof(GPU_Light) * 1000, lights.data(), GL_DYNAMIC_DRAW);

    setup_shaders();
    setup_ssao();
    setup_buffers();
}

void Renderer::setup_shaders() {
    Shader_Manager::init("../resources/shaders/");

    //Shader_Manager::load_from_paths("pbr", "vertex.glsl", "fragment.glsl");
    Shader_Manager::load_from_paths("indirect", "vertex_ind_v.glsl", "fragment_ind_f.glsl");

    Shader_Manager::load_from_name("skybox");
    //Shader_Manager::load_from_name("shadow_map");
    //Shader_Manager::load_from_name("shadow_map_point");
    Shader_Manager::load_from_name("quad");


    Shader_Manager::load_from_name("particle");
    //Shader_Manager::load_from_name("depth_prepass");
    Shader_Manager::load_from_paths("indirect_depth_prepass", "vertex_ind_depth_v.glsl", "depth_prepass_f.glsl");

    Shader_Manager::load_from_paths("skeleton_debug", "skeleton_debug_v.glsl", "outline_f.glsl");

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

    Shader_Manager::load_compute("cull_mesh");
    Shader_Manager::load_compute("clear_dirty");

    Shader_Manager::load_from_paths("fullscreen_texture", "quad_v.glsl", "quad_texture.glsl");

    Shader_Manager::load_tesselation("terrain");
}

void Renderer::setup_buffers() {
    glGenFramebuffers(1, &render_target);
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    
    depth_texture = Texture_Manager::create_depth_texture(scr_width, scr_height);
    scene_texture = Texture_Manager::create_render_texture(scr_width, scr_height, true);
    bright_texture = Texture_Manager::create_bloom_texture(scr_width, scr_height);
    ssao_texture = Texture_Manager::create_ssao_texture(scr_width, scr_height);
    picking_texture = Texture_Manager::create_picking_texture(scr_width, scr_height);
    moment0_texture = Texture_Manager::create_moment_texture(scr_width, scr_height); // b0,b1,b2,b3
    moment1_texture = Texture_Manager::create_moment_texture(scr_width, scr_height); // b4,b5,b6,transmittance

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(depth_texture), 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(scene_texture), 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(bright_texture), 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(picking_texture), 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(moment0_texture), 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(moment1_texture), 0);

    uint32_t attachments[5] = {
        GL_COLOR_ATTACHMENT0, // scene color
        GL_COLOR_ATTACHMENT1, // bright
        GL_COLOR_ATTACHMENT2, // picking
        GL_COLOR_ATTACHMENT3, // moment0
        GL_COLOR_ATTACHMENT4  // moment1
    };
    glDrawBuffers(5, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("[RENDERER] MAIN RENDER BUFFER FAILLLLLED TF OUT\n");
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // final output texture
    glGenFramebuffers(1, &output_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, output_framebuffer);

    output_texture = Texture_Manager::create_render_texture(scr_width, scr_height, true);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Texture_Manager::get_ogl_id(output_texture), 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("[RENDERER] OUTPUT BUFFER FAILLLLLED TF OUT\n");
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


    // setup draw command buffers for gpu culling
    glCreateBuffers(1, &opaque_draw_commands);
    glNamedBufferStorage(opaque_draw_commands, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &blended_draw_commands);
    glNamedBufferStorage(blended_draw_commands, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &csm_draw_commands);
    glNamedBufferStorage(csm_draw_commands, sizeof(Draw_Elements_Indirect_Command) * MAX_DRAW_COMMANDS, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &num_commands);
    glNamedBufferStorage(num_commands, sizeof(uint32_t) * 3, 0, GL_DYNAMIC_STORAGE_BIT);

    // clusters for forward+
    // todo ifdef forward vs deferred
    cluster_ssbo.init();
    cluster_ssbo.set_data(sizeof(Cluster) * 16 * 9 * 24, nullptr, GL_STATIC_COPY);

    // csm

    // organize, maybe scene should control params for CSM
    csm_texture = Texture_Manager::create_2d_array_texture(csm_res, csm_res, NUM_CASCADE);
    glGenFramebuffers(1, &csm_fbo);
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
}

void Renderer::setup_ssao() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    int max_ssao_samples = 64;
    for (int i = 0; i < max_ssao_samples; i++) {
        vec3 sample(
            dis(gen) * 2.0f - 1.0f,
            dis(gen) * 2.0f - 1.0f,
            dis(gen) // pos z, hemisphere (reverse z)
        );
        sample = normalize(sample);
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
        vec3 vals = normalize(vec3(dis(gen) * 2.0f - 1.0f, dis(gen) * 2.0f - 1.0f, 0.0f));
        noise.push_back(vals.x);
        noise.push_back(vals.y);
        noise.push_back(vals.z);
    }

    ssao_noise_texture = Texture_Manager::create_noise_texture(noise, 4, 4);
    Texture_Manager::bind(ssao_noise_texture, 2);
}

void Renderer::begin_frame(Scene& scene, const mat4& player_view, const mat4& player_inv_view, const mat4& player_proj, const mat4& active_view, const mat4& active_inv_proj, float aspect_ratio, float zoom) {
    cull_draw_commands(scene, player_view, player_proj); // build draw commands via main camera
    build_cluster_pass(active_inv_proj); // build clusters with current cam (editor, debug, etc)
    cull_cluster_pass(active_view);
    shadow_setup(scene, player_view, player_inv_view, aspect_ratio, zoom); // setup shadows with main player cam
}

void Renderer::cull_draw_commands(Scene& scene, const mat4& cull_view, const mat4& cull_proj) {
    PROFILE_SCOPE_COLOR("gpu cull", legit::Colors::pomegranate);
    // dispatch main update + culling shader
    // fills all render commands for meshes in scene
    Compute_Shader* c_shader = Shader_Manager::get_compute("cull_mesh");
    c_shader->use();
    // bind buffers

    GLuint zero = 0;
    glNamedBufferSubData(num_commands, 0, sizeof(GLuint), &zero); // clear draw cmds
    glNamedBufferSubData(num_commands, 4, sizeof(GLuint), &zero); // clear num csm commands
    glNamedBufferSubData(num_commands, 8, sizeof(GLuint), &zero);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // barrier for entity updates

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, num_commands);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, opaque_draw_commands);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, blended_draw_commands);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, csm_draw_commands);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, scene.gpu_entity_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, scene.gpu_mesh_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, scene.per_mesh_ssbo);

    // set cull uinforms
    mat4 projectionT = transpose(cull_proj);
    vec4 frustumX = normalize_plane(projectionT[3] + projectionT[0]); // x + w < 0
    vec4 frustumY = normalize_plane(projectionT[3] + projectionT[1]); // y + w < 0

    float frustum[4];
    frustum[0] = frustumX.x;
    frustum[1] = frustumX.z;
    frustum[2] = frustumY.y;
    frustum[3] = frustumY.z;

    uint32_t num_meshes = (uint32_t)scene.gpu_meshes.size();
    c_shader->set_uint("num_meshes", num_meshes);
    c_shader->set_mat4("view", cull_view);
    c_shader->set_float_array("frustum", frustum, 4);
    //c_shader->set_float("znear", 0.1f);
    //c_shader->set_float("zfar", 10000.0f);
    //uniform bool infinite_far;
    //c_shader->dispatch_and_wait((num_meshes + 63) / 64, 1, 1, GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    c_shader->dispatch((num_meshes + 63) / 64, 1, 1);

    // clear dirty flags from entities
    c_shader = Shader_Manager::get_compute("clear_dirty");
    c_shader->use();

    uint32_t num_entities = scene.gpu_entities.size();
    c_shader->set_uint("num_entities", num_entities);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, scene.gpu_entity_ssbo);

    c_shader->dispatch((num_entities + 63) / 64, 1, 1);
    // GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT
}

void Renderer::build_cluster_pass(const mat4& inv_proj) {
    PROFILE_SCOPE_COLOR("build clusters", legit::Colors::pumpkin);

    Compute_Shader* cluster_build = Shader_Manager::get_compute("cluster");
    cluster_build->use();
    cluster_ssbo.bind(1); // todo fix once

    cluster_build->set_float("zNear", 0.1f); // once
    cluster_build->set_float("zFar", FAR_PLANE); // once (would have to change if changed)
    cluster_build->set_mat4("inverseProjection", inv_proj);
    cluster_build->set_uvec3("gridSize", uvec3(16, 9, 24)); // thnk about how to do x y
    cluster_build->set_uvec2("screenDimensions", uvec2(scr_width, scr_height)); // once + on change

    // todo maybe change these based on res?
    uint32_t groups_x = (16 + 7) / 8;
    uint32_t groups_y = (9 + 7) / 8;
    uint32_t groups_z = (24 + 7) / 8;

    //cluster_build->dispatch_and_wait(1, 1, 24, GL_SHADER_STORAGE_BARRIER_BIT);
    cluster_build->dispatch(1, 1, 24);
}

void Renderer::cull_cluster_pass(const mat4& view) {
    PROFILE_SCOPE_COLOR("cull lights", legit::Colors::carrot);

    // clusters need to exist for us to cull lights against them
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    Compute_Shader* cluster_cull = Shader_Manager::get_compute("cluster_cull");
    cluster_cull->use();

    cluster_ssbo.bind(1); // bind once
    light_ssbo.bind(2); // bind once

    cluster_cull->set_mat4("viewMatrix", view);
    cluster_cull->set_int("num_lights", num_lights); // maybe frequently changing?

    //cluster_cull->dispatch_and_wait(27, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
    uint32_t num_clusters = 16 * 9 * 24; // todo maybe change
    uint32_t groups = (num_clusters + 127) / 128;
    cluster_cull->dispatch_and_wait(groups, 1, 1, GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::shadow_setup(const Scene& scene, const mat4& view, const mat4& inv_view, float aspect_ratio, float zoom) {
    PROFILE_SCOPE_COLOR("shadow setup", legit::Colors::alizarin);

    // todo use different up if sun pointing straight up or down
    mat4 sun_mat = lookAt(vec3(0.0f, 0.0f, 0.0f), -normalize(scene.sun_direction), vec3(0.0f, 1.0f, 0.0f));

    float tanHalfVFOV = tanf(radians(zoom / 2.0f));
    float tanHalfHFOV = tanHalfVFOV * aspect_ratio;

    // float tanHalfHFOV = tanf(radians(zoom / 2.0f));
    // float tanHalfVFOV = tanf(radians((zoom * aspect_ratio) / 2.0f));

    //printf("ar %f tanHalfHFOV %f tanHalfVFOV %f\n", ar, tanHalfHFOV, tanHalfVFOV);

    for (uint32_t i = 0; i < NUM_CASCADE; i++) {
        float xn = scene.cascade_ends[i] * tanHalfHFOV;
        float xf = scene.cascade_ends[i + 1] * tanHalfHFOV;
        float yn = scene.cascade_ends[i] * tanHalfVFOV;
        float yf = scene.cascade_ends[i + 1] * tanHalfVFOV;

        //printf("xn %f xf %f\n", xn, xf);
        //printf("yn %f yf %f\n", yn, yf);

        vec4 frustumCorners[8] = {
            // near face
            vec4(xn,   yn, -scene.cascade_ends[i], 1.0),
            vec4(-xn,  yn, -scene.cascade_ends[i], 1.0),
            vec4(xn,  -yn, -scene.cascade_ends[i], 1.0),
            vec4(-xn, -yn, -scene.cascade_ends[i], 1.0),

            // far face
            vec4(xf,   yf, -scene.cascade_ends[i + 1], 1.0),
            vec4(-xf,  yf, -scene.cascade_ends[i + 1], 1.0),
            vec4(xf,  -yf, -scene.cascade_ends[i + 1], 1.0),
            vec4(-xf, -yf, -scene.cascade_ends[i + 1], 1.0)
        };

        //vec4 frustumCornersL[8];

        vec4 min_c = vec4(FLT_MAX);
        vec4 max_c = vec4(-FLT_MAX);

        for (uint32_t j = 0; j < 8; j++) {
            //printf("Frustum: ");
            vec4 vW = inv_view * frustumCorners[j];
            //printf("Light space: ");
            //frustumCornersL[j] = sun_mat * vW;
            //frustumCornersL[j].Print();
            //printf("\n");
            vec4 corner = sun_mat * vW;

            min_c = min(min_c, corner);
            max_c = max(max_c, corner);
        }

        vec3 box_size = vec3(max_c) - vec3(min_c);
        vec3 center = (vec3(min_c) + vec3(max_c)) * 0.5f;

        float texel_size_x = box_size.x / (float)csm_res;
        float texel_size_y = box_size.y / (float)csm_res;

        box_size.x = ceil(box_size.x / texel_size_x) * texel_size_x;
        box_size.y = ceil(box_size.y / texel_size_y) * texel_size_y;

        texel_size_x = box_size.x / (float)csm_res;
        texel_size_y = box_size.y / (float)csm_res;

        center.x = floor(center.x / texel_size_x) * texel_size_x;
        center.y = floor(center.y / texel_size_y) * texel_size_y;

        vec3 half_size = box_size * 0.5f;
        min_c = vec4(center - half_size, 69.0f);
        max_c = vec4(center + half_size, 420.0f);

        cascade_mats[i] = glm::orthoRH_ZO(min_c.x, max_c.x, min_c.y, max_c.y, -max_c.z, -min_c.z) * sun_mat;
        // cascade_mats[i] = ortho(min_c.x, max_c.x, min_c.y, max_c.y, max_c.z, min_c.z) * sun_mat;

        mat4 inv_sun_mat = inverse(sun_mat);
        vec3 light_corners[8] = {
            {min_c.x, min_c.y, min_c.z}, {max_c.x, min_c.y, min_c.z},
            {min_c.x, max_c.y, min_c.z}, {max_c.x, max_c.y, min_c.z},
            {min_c.x, min_c.y, max_c.z}, {max_c.x, min_c.y, max_c.z},
            {min_c.x, max_c.y, max_c.z}, {max_c.x, max_c.y, max_c.z}
        };
        vec3 minW(FLT_MAX), maxW(-FLT_MAX);
        for (auto& c : light_corners) {
            vec4 w = inv_sun_mat * vec4(c, 1.0f);
            minW = min(minW, vec3(w));
            maxW = max(maxW, vec3(w));
        }
        vec3 color;
        if (i == 0) color = Util::red;
        if (i == 1) color = Util::green;
        if (i == 2) color = Util::blue;
        if (i == 3) color = Util::cyan;
        debug_renderer.add_bbox(minW, maxW, color);
    }
}

void Renderer::depth_prepass(const mat4& viewproj) {

    // TODO Fix me

    //glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    //glViewport(0, 0, scr_width, scr_height);
    //glEnable(GL_DEPTH_TEST); // should be on already todo remove maybe

    //// pre pass state
    //glDepthFunc(GL_GREATER);
    //glDepthMask(GL_TRUE);
    //glClear(GL_DEPTH_BUFFER_BIT);

    //Shader* shader = Shader_Manager::get_shader("indirect_depth_prepass");
    //shader->use();
    //shader->set_mat4("vp", viewproj);

    //uint32_t vao = Model_Manager::get_big_vao();
    //glBindVertexArray(vao);

    //// draw commands and transform
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, opaque_object_ssbo);
    //glBindBuffer(GL_DRAW_INDIRECT_BUFFER, opaque_draw_command_ssbo);

    //glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, opaque_draw_count, 0);

    //glBindVertexArray(0);
}

void Renderer::shadow_pass(Scene& scene) {
    PROFILE_SCOPE_COLOR("CSM shadow pass", legit::Colors::pomegranate);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

    //p.SetCamera(Vector3f(0.0f, 0.0f, 0.0f), m_dirLight.Direction, Vector3f(0.0f, 1.0f, 0.0f));
    //GLuint csm_count;
    //glGetBufferSubData(GL_PARAMETER_BUFFER, 4, sizeof(GLuint), &csm_count);
    //printf("CSM Draw count: %u\n", csm_count);

    Shader* shader = Shader_Manager::get_shader("indirect_depth_prepass");
    shader->use();

    glBindFramebuffer(GL_FRAMEBUFFER, csm_fbo);
    glViewport(0, 0, csm_res, csm_res);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_TRUE);
    uint32_t vao = Model_Manager::get_big_vao();
    glBindVertexArray(vao);

    glBindBuffer(GL_PARAMETER_BUFFER, num_commands);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, csm_draw_commands);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scene.per_mesh_ssbo);

    glDisable(GL_BLEND);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

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

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT); // todo why is this here
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
        // terrain casts shadows
        // shader = Shader_Manager::get_shader("terrain");
        // shader->use();
        // shader->set_mat4("vp", cascade_mats[i]);

        // Texture_Manager::bind(scene.terrain.heightmap, 0);
        // Texture_Manager::bind(scene.terrain.heightmap_texture, 1);
        // glPatchParameteri(GL_PATCH_VERTICES, 4);
        // glBindVertexArray(scene.terrain.vao);

        // shader->set_bool("lines", false);
        // glDrawArrays(GL_PATCHES, 0, scene.terrain.vertex_count);

    }

    glCullFace(GL_BACK);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // atlas
}

void Renderer::draw(Scene& scene, const vec3& view_pos, const mat4& view, const mat4& viewproj, const mat4& player_view, const mat4& cull_proj, bool wireframe) {
    PROFILE_SCOPE_COLOR("draw", legit::Colors::belizeHole);

    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);
    glEnable(GL_DEPTH_TEST); // should be on already todo remove maybe
    glDisable(GL_BLEND);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    if (use_depth_prepass) {
        glClear(GL_COLOR_BUFFER_BIT);
        glDepthFunc(GL_GEQUAL); // fragments at same depth pass
        glDepthMask(GL_FALSE); // dont write depth, unnecessary
    }
    else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDepthFunc(GL_GREATER);
        glDepthMask(GL_TRUE);
    }

    if (wireframe)
       glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
       glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    Shader* shader = Shader_Manager::get_shader("indirect");
    shader->use();
    //glStencilMask(0x00);

    Texture_Manager::bind(ssao_texture, 7); // todo once
    Texture_Manager::bind_array(csm_texture, 8); // todo once
    glActiveTexture(GL_TEXTURE0 + 9);
    glBindTexture(GL_TEXTURE_CUBE_MAP, scene.skybox.texture_id);

    shader->set_uint("num_skybox_mips", scene.skybox.num_mips);

    shader->set_mat4("vp", viewproj);

    shader->set_bool("blend", false);

    shader->set_vec3("view_pos", view_pos);
    shader->set_int("num_lights", num_lights);
    shader->set_float("zNear", 0.1f);
    shader->set_float("zFar", FAR_PLANE);
    shader->set_mat4("viewMatrix", view);
    shader->set_mat4("playerViewMatrix", player_view);
    shader->set_uvec3("gridSize", uvec3(16, 9, 24));
    shader->set_uvec2("screenDimensions", uvec2(scr_width, scr_height));

    shader->set_bool("ssao_enabled", ssao_enabled);
    shader->set_int("debug", debug_mode % 8); // num debug modes

    for (uint32_t i = 0 ; i < NUM_CASCADE ; i++) {
        vec4 vView(0.0f, 0.0f, scene.cascade_ends[i + 1], 1.0f);
        vec4 clip = cull_proj * vView;
        shader->set_float("cascade_distances[" + std::to_string(i) + "]", vView.z);
    } 
    shader->set_mat4_array("cascade_matrices", cascade_mats, NUM_CASCADE);
    // shader->set_float_array("cascade_distances", CASCADE_END, NUM_CASCADE + 1);
    shader->set_vec3("directional_light_direction", scene.sun_direction);
    shader->set_vec3("directional_light_color", scene.sun_color);
    shader->set_float("directional_light_intensity", scene.sun_strength);

    shader->set_bool("cascade_vis", cascade_vis);

    Texture_Manager::bind(ssao_texture, 0); // todo once
    Texture_Manager::bind_array(csm_texture, 1); // todo once

    uint32_t vao = Model_Manager::get_big_vao();
    glBindVertexArray(vao);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scene.per_mesh_ssbo);
    cluster_ssbo.bind(1);
    light_ssbo.bind(2);
    
    glBindBuffer(GL_PARAMETER_BUFFER, num_commands);

    // we need updated buffers
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

#if BINDLESS

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, opaque_draw_commands);

    // GLuint count;
    // glGetBufferSubData(GL_PARAMETER_BUFFER, 0, sizeof(GLuint), &count);
    // printf("solid Draw count: %u \n", count);

    glMultiDrawElementsIndirectCount(
        GL_TRIANGLES,
        GL_UNSIGNED_INT,
        (void*)0,                             // indirect offset
        (GLintptr)0,                          // offset in the count buffer
        MAX_DRAW_COMMANDS,                                 // maximum draws
        sizeof(Draw_Elements_Indirect_Command)  // stride
    );


    render_skybox(scene.skybox, view, cull_proj);

    // blended
    shader = Shader_Manager::get_shader("indirect");
    shader->use(); // already has correct uniforms for frame
    shader->set_bool("blend", true);

    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    //glEnable(GL_DEPTH_TEST); on already

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunci(0, GL_ONE, GL_ONE);
    glBlendFunci(1, GL_ONE, GL_ONE);
    glBlendFunci(2, GL_ONE, GL_ZERO);
    glBlendFunci(3, GL_ONE, GL_ONE);
    glBlendFunci(4, GL_ONE, GL_ONE);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, blended_draw_commands);

    // glGetBufferSubData(GL_PARAMETER_BUFFER, 8, sizeof(GLuint), &count);
    // printf("op Draw count: %u \n", count);
    
    glMultiDrawElementsIndirectCount(
        GL_TRIANGLES,
        GL_UNSIGNED_INT,
        (void*)0,                             // indirect offset
        (GLintptr)8,                          // offset in the count buffer
        MAX_DRAW_COMMANDS,                                 // maximum draws
        sizeof(Draw_Elements_Indirect_Command)  // stride
    );

#else
    GLuint count;
    glGetBufferSubData(GL_PARAMETER_BUFFER, 0, sizeof(GLuint), &count);
    // printf("Draw count: %u \n", count);

    std::vector<Draw_Elements_Indirect_Command> commands(count);
    glBindBuffer(GL_COPY_READ_BUFFER, opaque_draw_commands);
    glGetBufferSubData(GL_COPY_READ_BUFFER, 0,
        count * sizeof(Draw_Elements_Indirect_Command),
        commands.data());

    for (uint32_t i = 0; i < count; i++) {
        Draw_Elements_Indirect_Command cmd = commands[i];
        Per_Object_Data pod = scene.per_mesh_data[cmd.base_instance];

        Defaults def = Texture_Manager::get_defaults();
        Texture_Manager::bind(pod.albedo != 0 ? pod.albedo : def.albedo, 1, 0); // pink black
        //Texture_Manager::bind(pod.normal != 0 ? pod.normal : def.normal, 1);
        //Texture_Manager::bind(pod.met_rough != 0 ? pod.met_rough : def.met_rough, 2);
        //Texture_Manager::bind(pod.emissive != 0 ? pod.emissive : def.emissive, 3);
        //Texture_Manager::bind(pod.amb_occ != 0 ? pod.amb_occ : def.ao, 4);

        //Texture_Manager::bind(pod.albedo, 0);
        Texture_Manager::bind(pod.normal, 1);
        Texture_Manager::bind(pod.met_rough, 2);
        Texture_Manager::bind(pod.emissive, 3);
        Texture_Manager::bind(pod.amb_occ, 4);


        shader->set_uint("instance_id", cmd.base_instance);

        // glDrawElementsBaseVertex(GL_TRIANGLES, cmd.count, GL_UNSIGNED_INT, (void*)(cmd.first_index * sizeof(uint32_t)), cmd.base_vertex);

        glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, cmd.count, GL_UNSIGNED_INT, (void*)(cmd.first_index * sizeof(uint32_t)), cmd.instance_count, cmd.base_vertex, cmd.base_instance);
    }

    //render_skybox(scene.skybox, view, cull_proj);
    
    // render blendeds 

#endif

    glBindVertexArray(0);

    // shader = Shader_Manager::get_shader("terrain");
    // shader->use();
    // shader->set_mat4("vp", viewproj);
    // shader->set_mat4("view", view);
    // for (uint i = 0 ; i < NUM_CASCADE ; i++) {
    //     vec4 vView(0.0f, 0.0f, CASCADE_END[i + 1], 1.0f);
    //     vec4 clip = cull_proj * vView;
    //     shader->set_float("cascade_distances[" + std::to_string(i) + "]", vView.z);
    // } 
    // shader->set_mat4_array("cascade_matrices", cascade_mats, NUM_CASCADE);
    // Texture_Manager::bind_array(csm_texture, 8); // todo once

    // Texture_Manager::bind(scene.terrain.heightmap, 0);
    // Texture_Manager::bind(scene.terrain.heightmap_texture, 1);
    // glPatchParameteri(GL_PATCH_VERTICES, 4);
    // glBindVertexArray(scene.terrain.vao);

    // if (terrain_draw_type == 0 || terrain_draw_type == 2) {
    //     shader->set_bool("lines", false);
    //     glDrawArrays(GL_PATCHES, 0, scene.terrain.vertex_count);
    // }
    // if (terrain_draw_type == 1) {
    //     shader->set_bool("lines", true);
    //     glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //     glDrawArrays(GL_PATCHES, 0, scene.terrain.vertex_count);
    //     glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // }
    // if (terrain_draw_type == 2) {
    //     shader->set_bool("lines", true);
    //     glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //     glDrawArrays(GL_PATCHES, 0, scene.terrain.vertex_count);
    //     glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // }

    // glBindVertexArray(0);
}

void Renderer::particle_pass(float delta_time, const mat4& proj, const mat4& view) {
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_DEPTH_TEST);

    Shader* shader = Shader_Manager::get_shader("particle");
    shader->use();
    shader->set_mat4("view", view);
    shader->set_mat4("projection", proj);
    glBindVertexArray(quadVAO);

    Particle_Manager::draw();
}

void Renderer::render_skybox(const Skybox& skybox, const mat4& view, const mat4& projection) {
    // draw sky so blending works with it
    glDepthFunc(GL_GEQUAL);
    glDepthMask(GL_FALSE);
    //glDisable(GL_BLEND);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    Shader* shader = Shader_Manager::get_shader("skybox");
    shader->use();

    mat4 viewNoTranslation = mat4(mat3(view));
    shader->set_mat4("view", viewNoTranslation);
    shader->set_mat4("projection", projection);

    glActiveTexture(GL_TEXTURE30);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.texture_id);
    glDrawArrays(GL_TRIANGLES, skybox.base_vertex, 36);
}

void Renderer::render_crosshair(const Crosshair& crosshair) {
    Shader* s = Shader_Manager::get_shader("crosshair");
    s->use();
    crosshair.draw(s, scr_width, scr_height);
}

void Renderer::render_hud_text(const Text& text) {
    mat4 projection = ortho(0.0f, (float)scr_width, 0.0f, (float)scr_height);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader* shader = Shader_Manager::get_shader("hud_text");
    text.draw(shader, projection);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Renderer::bloom_pass() {
    PROFILE_SCOPE_COLOR("bloom", legit::Colors::turqoise);

    uint32_t texture_id = Texture_Manager::get_ogl_id(bright_texture);

    Compute_Shader* bloom_down = Shader_Manager::get_compute("bloomdown");
    Compute_Shader* bloom_up = Shader_Manager::get_compute("bloomup");

    const int MIP_LEVELS = 6;

    for (int i = 1; i < MIP_LEVELS; i++) {
        int mip_width = std::max(1, scr_width >> i);
        int mip_height = std::max(1, scr_height >> i);

        // prev mip input
        //glBindImageTexture(0, texture_id, i - 1, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        glBindTextureUnit(0, texture_id);
        // current mip output
        glBindImageTexture(1, texture_id, i, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        bloom_down->use();
        bloom_down->set_int("mipLevel", i - 1);
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
        bloom_down->set_int("mipLevel", i - 1);
        int groups_x = (mip_width + 7) / 8;
        int groups_y = (mip_height + 7) / 8;
        bloom_up->dispatch_and_wait(groups_x, groups_y, 1, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}

void Renderer::ssao_pass(const mat4& proj, const mat4& inv_proj) {
    PROFILE_SCOPE_COLOR("SSAO", legit::Colors::emerald);

    Compute_Shader* ssao = Shader_Manager::get_compute("ssao");
    ssao->use();

    Texture_Manager::bind(depth_texture, 0); // todo maybe dont need to
    Texture_Manager::bind(ssao_noise_texture, 1);
    //Texture_Manager::bind(ssao_texture, 2);
    //glBindImageTexture(2, Texture_Manager::get_ogl_id(ssao_texture), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8);
    glBindImageTexture(2, Texture_Manager::get_ogl_id(ssao_texture), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    ssao->set_mat4("projection", proj);
    ssao->set_mat4("inverse_projection", inv_proj);
    ssao->set_vec2("screen_size", vec2(scr_width, scr_height));
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
    PROFILE_SCOPE_COLOR("composite", legit::Colors::greenSea);

    glBindFramebuffer(GL_FRAMEBUFFER, output_framebuffer);
    glViewport(0, 0, scr_width, scr_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    Shader* shader = Shader_Manager::get_shader("quad");
    shader->use();

    shader->set_int("scene_color", 0);
    Texture_Manager::bind(scene_texture, 0);
    shader->set_int("bright_color", 1);
    Texture_Manager::bind(bright_texture, 1);
    shader->set_int("moment0", 2);
    Texture_Manager::bind(moment0_texture, 2);
    shader->set_int("moment1", 3);
    Texture_Manager::bind(moment1_texture, 3);
    shader->set_int("depth_texture", 4);
    Texture_Manager::bind(depth_texture, 4);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0); // unbind quad
}

void Renderer::blit_to_screen() {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, output_framebuffer);

    // Bind the destination (default framebuffer — the screen)
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // 0 is default framebuffer

    // Blit the color buffer from composite FBO to screen
    glBlitFramebuffer(
        0, 0, scr_width, scr_height,        // src rect
        0, 0, scr_width, scr_height,        // dst rect
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST // or GL_LINEAR for smooth scaling
    );
}

void Renderer::debug_cascades(Scene& scene) {
    glBindFramebuffer(GL_FRAMEBUFFER, output_framebuffer);
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

    // shader->set_int("mode", 1);
    // glViewport(10, 10, 400, 400);
    // Texture_Manager::bind(scene.terrain.heightmap, 1);
    // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0); // unbind quad
}

void Renderer::infinite_grid(const mat4& vp, const vec3& cam_pos) {
    Shader* shader = Shader_Manager::get_shader("grid");
    shader->use();
    shader->set_mat4("vp", vp);
    shader->set_vec3("gCameraWorldPos", cam_pos);
    
    static GLuint dummyvao = 0;
    if (dummyvao == 0) {
        glGenVertexArrays(1, &dummyvao);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // glDepthFunc(GL_ALWAYS);  // Always pass

    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, scr_width, scr_height);

    glBindVertexArray(dummyvao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::draw_light_quads(const mat4& proj, const mat4& view) {
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
}

void Renderer::render_debug(const mat4& view, const mat4& proj, Scene& scene) {
    Shader* shader = Shader_Manager::get_shader("debug");
    //mat4 projection = perspective(radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 1.0f, FAR_PLANE);
    //mat4 projection = player.camera.get_projection((float) scr_width / (float) scr_height);

    shader->set_bool("uniform_color", false);
    debug_renderer.render(shader, proj, view, num_lights);
    shader->set_bool("uniform_color", true);
    debug_renderer.draw_scene_bounding_spheres(shader, scene, proj * view);
}

void Renderer::debug_skeletons(Scene& scene, const mat4& vp) {
    // use skeleton debug shader
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);  // Always pass
    glDepthMask(GL_TRUE);    // Still write to depth buffer
    glEnable(GL_PROGRAM_POINT_SIZE);

    Shader* shader = Shader_Manager::get_shader("skeleton_debug");
    shader->use();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, Model_Manager::get_absolute_bones());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, Model_Manager::get_bone_ssbo());

    static GLuint dummyVAO = 0;
    if (dummyVAO == 0) {
        glGenVertexArrays(1, &dummyVAO);
    }
    glBindVertexArray(dummyVAO);

    bool blender_bones = true;

    // draw points
    for (Entity& e : scene.entities) {
        if (e.is_animated) {
            Animated_Model am = Model_Manager::get_animated_model(e.model_id);
            uint32_t base_bone = am.base_bone + am.bone_offset;
            uint32_t bone_count = am.bone_count;

            // printf("drawing skeelton for %s, base bone: %d, bone count: %d, offset(%d)\n", am.m_name.c_str(), base_bone, bone_count, am.bone_offset);

            shader->set_mat4("mvp", vp * e.get_model_matrix() * am.m_meshes[0].transform);
            shader->set_uint("base_bone", base_bone);
            shader->set_uint("max_bone", base_bone + bone_count);
            shader->set_uint("bone_offset", am.bone_offset);

            if (blender_bones) {
                glLineWidth(1.0f);
                shader->set_uint("draw_mode", 2);
                shader->set_vec3("color", vec3(0.0f, 1.0f, 1.0f));
                glDrawArrays(GL_LINES, 0, bone_count * 24);

            } else {
                glLineWidth(2.0f);
                shader->set_uint("draw_mode", 1);
                shader->set_vec3("color", vec3(0.0f, 1.0f, 1.0f));
                glDrawArrays(GL_LINES, 0, bone_count * 2);

                shader->set_uint("draw_mode", 0);
                shader->set_vec3("color", vec3(1.0f, 0.64f, 0.0f));
                glDrawArrays(GL_POINTS, 0, bone_count);
            }
        }
    }
}

void Renderer::imgui_pass() {
    ImGui::Begin("Renderer");

    // ImGui::Checkbox("depth pre-pass", &use_depth_prepass);
    // ImGui::Checkbox("shadows enabled", &shadows_enabled); // todo maybe use
    ImGui::SliderInt("num_lights", &num_lights, 0, 1000);
    ImGui::Checkbox("light quads", &do_draw_light_quads);
    ImGui::Checkbox("forward+", &forward_plus);
    ImGui::Checkbox("bloom_enabled", &bloom_enabled);

    // ssao settings
    ImGui::Checkbox("ssao_enabled", &ssao_enabled);
    ImGui::SliderFloat("ssao_radius", &ssao_radius, 0, 5.0);
    ImGui::SliderFloat("ssao_bias", &ssao_bias, 0, 1.0f);
    ImGui::SliderInt("ssao_samples", &ssao_samples, 0, 64);
    ImGui::SliderFloat("min_depth", &min_depth, -0.01, 0.2f);
    ImGui::SliderFloat("power", &power, -2, 4);

    ImGui::End();
}

vec4 Renderer::normalize_plane(vec4 p) {
    return p / length(vec3(p));
}
