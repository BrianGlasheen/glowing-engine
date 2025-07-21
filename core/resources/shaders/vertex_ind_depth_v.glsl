#version 460 core
#extension GL_ARB_gpu_shader_int64: enable

layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec3 aNor;
//layout (location = 2) in vec2 aTexCoord;
//layout (location = 3) in vec3 Tangent;
//layout (location = 4) in vec3 Bitangent;

struct Per_Object_Data {
    mat4 model_matrix;
    mat4 normal_matrix;
    vec4 color; // todo remove
    uint64_t albedo;
    uint64_t normal;
    uint64_t met_rough;
    uint64_t emissive;
    vec4 emissive_factor;
    float metallic_factor; // 4
    float roughness_factor; // 4
    uint64_t amb_occ;
    vec4 base_color;
};

layout(std430, binding = 0) readonly buffer per_object_ssbo {
    Per_Object_Data per_object_data[];
};

out vec3 FragPos;  // position in world space
// maybe output normal for AO?

uniform mat4 vp;

void main() {
    gl_Position = vp * per_object_data[gl_DrawID].model_matrix * vec4(aPos, 1.0);
}
