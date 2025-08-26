#pragma once

 #include <glad/glad.h>
 #include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include <cstdint>

// todo prob move to types or something
struct Per_Object_Data {
    glm::mat4 model_matrix; // 64
    glm::mat4 normal_matrix; // 64
    glm::vec4 base_color;
    glm::vec4 emissive_factor; // 16

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