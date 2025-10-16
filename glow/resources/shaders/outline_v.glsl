#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 0) in vec3 aNor;

struct Per_Object_Data {
    mat4 model_matrix;
    mat4 normal_matrix;
    vec4 base_color;
    vec4 emissive_factor;
    
    uint64_t albedo;
    uint64_t normal;
    uint64_t met_rough;
    uint64_t emissive;
    uint64_t amb_occ;
    uint64_t padding;

    float alpha_cutoff;
    float metallic_factor; // 4
    float roughness_factor; // 4
    uint id;
};

layout(std430, binding = 0) readonly buffer per_object_ssbo {
    Per_Object_Data per_object_data[];
};

uniform mat4 vp;
uniform float scale;

void main() {
    gl_Position = projection * view * model * vec4(aPos + aNor * scale, 1.0);
}
