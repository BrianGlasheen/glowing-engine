#pragma once

 #include <glad/glad.h>
 #include <GLFW/glfw3.h>

#include "util/math.h"

#include <cstdint>

// todo prob move to types or something
struct Per_Object_Data {
    mat4 model_matrix; // 64
    mat4 normal_matrix; // 64
    vec4 base_color;
    vec4 emissive_factor; // 16

    uint64_t albedo; // 8
    uint64_t normal; // 8    
    uint64_t met_rough; // 8
    uint64_t emissive; // 8
    uint64_t amb_occ;
    uint64_t padding;

    float alpha_cutoff;
    float metallic_factor; // 4
    float roughness_factor; // 4
    uint32_t bone_offset;
};