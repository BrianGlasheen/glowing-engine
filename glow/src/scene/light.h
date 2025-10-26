#pragma once

#include <cstdint>

#include "glm/glm.hpp"

enum Light_Type {
    DIRECTIONAL = 0,
    SPOT,
    POINT
};

struct GPU_Light {
    glm::vec4 position_radius; // x, y ,z, radius
    glm::vec4 color_strength; // r g b intensity
    glm::vec4 direction_type; // x y z type
    glm::vec4 params; // inner cone, outer cone, shadow map idx, unused 
};

//class Light {
//public:
//    Light() = default;
//    Light(Light_Type lt, glm::vec3 pos, glm::vec3 dir, glm::vec3 col, float intens, uint32_t w, uint32_t h, float fov_in = 25.0f, float fov_out = 45.0f);
//
//    // TODO CLEAN TF UP HOLY
//    static Light create_directional(glm::vec3 dir, glm::vec3 col, float intens, uint32_t w = 1024, uint32_t h = 1024);
//    static Light create_point(glm::vec3 pos, glm::vec3 col, float intens, uint32_t w = 1024, uint32_t h = 1024);
//    static Light create_spot(glm::vec3 pos, glm::vec3 dir, glm::vec3 col, float intens, float fov_in, float fov_out, uint32_t w = 1024, uint32_t h = 1024);
//
//    void generate_fbo(uint32_t width, uint32_t height);
//    void generate_cubemap(uint32_t width);
//
//    void bind_fbo_write();
//    void bind_cubemap_face_write(uint32_t face);
//    void bind_fbo_read(uint32_t location);
//
////private:
//    Light_Type type;
//    glm::vec3 position;
//    glm::vec3 direction;
//    glm::vec3 color;
//    float intensity;
//    float inner_fov;
//    float outer_fov;
//
//    uint32_t width, height;
//    uint32_t fbo, shadow_map;
//    uint32_t cube_depth;
//};