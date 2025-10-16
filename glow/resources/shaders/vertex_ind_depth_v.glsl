#version 460 core

#define BINDLESS 0

#extension GL_ARB_gpu_shader_int64: enable

layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 Tangent;
layout (location = 2) in vec4 Bitangent;
layout (location = 3) in vec2 aTexCoord;

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

    float alpha_cutoff;
    float metallic_factor; // 4
    float roughness_factor; // 4
    uint id;
};

layout(std430, binding = 0) readonly buffer per_object_ssbo {
    Per_Object_Data per_object_data[];
};

// out vec3 FragPos;  // position in world space
// maybe output normal for AO?

uniform mat4 vp;
#if !BINDLESS
    uniform uint draw_id;
#endif

void main() {
    #if BINDLESS
        gl_Position = vp * per_object_data[gl_BaseInstance].model_matrix * vec4(aPos.xyz, 1.0);
    #else
        gl_Position = vp * per_object_data[draw_id].model_matrix * vec4(aPos.xyz, 1.0);
    #endif
}
