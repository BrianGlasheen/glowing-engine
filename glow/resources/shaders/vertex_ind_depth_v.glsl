#version 460 core

#define BINDLESS 0

#extension GL_ARB_gpu_shader_int64: enable

layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 Tangent;
layout (location = 2) in vec4 Bitangent;
layout (location = 3) in vec2 aTexCoord;

/*
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
*/

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

// out vec3 FragPos;  // position in world space
// maybe output normal for AO?

uniform mat4 vp;
#if !BINDLESS
    uniform uint instance_id;
#endif

out vec2 TexCoord;

out flat vec4 base_color_factor;
out flat uint64_t albedo_handle;
out flat float alpha_cutoff;

void main() {
    #if BINDLESS
        Per_Object_Data obj_data = per_object_data[gl_BaseInstance];
    #else
        Per_Object_Data obj_data = per_object_data[instance_id];
    #endif

    TexCoord = aTexCoord;

    gl_Position = vp * obj_data.model_matrix * vec4(aPos.xyz, 1.0);
    base_color_factor = obj_data.base_color;
    albedo_handle = obj_data.albedo;
    alpha_cutoff = obj_data.alpha_cutoff;
}
