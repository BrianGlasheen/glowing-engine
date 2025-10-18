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
    uint64_t padding;

    float alpha_cutoff;
    float metallic_factor; // 4
    float roughness_factor; // 4
    uint id;
};

layout(std430, binding = 0) readonly buffer per_object_ssbo {
    Per_Object_Data per_object_data[];
};

out vec3 FragPos;  // position in world space
out vec3 Normal;   // normal in world space
out vec2 TexCoord;
out vec3 Tangentout;
out vec3 Bitangentout;

out flat vec4 base_color_factor;
out flat uint64_t albedo_handle;
out flat uint64_t normal_handle;
out flat uint64_t met_rough_handle;
out flat uint64_t emissive_handle;
out flat uint64_t amb_occ_handle;
out flat vec4 emissive;
out flat float metallic_factor;
out flat float roughness_factor;
out flat float alpha_cutoff;
out flat uint id;

const int NUM_CASCADES = 4;
uniform mat4 cascade_matrices[NUM_CASCADES];

out vec4 light_space_pos[NUM_CASCADES];
out float view_space_z;

uniform mat4 vp;
uniform mat4 playerViewMatrix;
#if !BINDLESS
    uniform uint instance_id;
#endif

void main() {
    #if BINDLESS
        Per_Object_Data obj_data = per_object_data[gl_BaseInstance];
    #else
        Per_Object_Data obj_data = per_object_data[instance_id];
    #endif

    base_color_factor = obj_data.base_color;
    albedo_handle = obj_data.albedo;
    normal_handle = obj_data.normal;
    met_rough_handle = obj_data.met_rough;
    emissive_handle = obj_data.emissive;
    emissive = obj_data.emissive_factor;
    metallic_factor = obj_data.metallic_factor;
    roughness_factor = obj_data.roughness_factor;
    amb_occ_handle = obj_data.amb_occ;
    alpha_cutoff = obj_data.alpha_cutoff;
    id = obj_data.id;
    
    mat4 model = obj_data.model_matrix;

    vec3 aNor = vec3(aPos.w, Tangent.w, Bitangent.w);

    FragPos = vec3(model * vec4(aPos.xyz, 1.0));
    Normal = normalize(mat3(obj_data.normal_matrix) * aNor);
    TexCoord = aTexCoord;
    Tangentout = normalize(mat3(obj_data.normal_matrix) * Tangent.xyz);
    Bitangentout = normalize(mat3(obj_data.normal_matrix) * Bitangent.xyz);

    gl_Position = vp * model * vec4(aPos.xyz, 1.0);

    for (int i = 0 ; i < NUM_CASCADES ; i++)
        light_space_pos[i] = cascade_matrices[i] * model * vec4(aPos.xyz, 1.0);

    view_space_z = -(playerViewMatrix * model * vec4(aPos.xyz, 1.0)).z;
    // clip_space_z = gl_Position.z;
}
