#version 460 core
#define BINDLESS 1

#extension GL_ARB_gpu_shader_int64: enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 Tangent;
layout (location = 4) in vec3 Bitangent;
layout (location = 5) in uvec4 BoneIds;
layout (location = 6) in vec4 BoneWeights;

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
    uint bone_offset; // todo remove
};

struct GPU_Bone_Skinned {
    mat4 transform;
};

layout(std430, binding = 0) readonly buffer per_object_ssbo {
    Per_Object_Data per_object_data[];
};

layout(std430, binding = 1) readonly buffer bones_ssbo {
    GPU_Bone_Skinned bones[];
};

out vec3 FragPos;  // position in world space
out vec3 Normal;   // normal in world space
out vec2 TexCoord;
out vec3 Tangentout;
out vec3 Bitangentout;
out uvec4 BonesOut;
out vec4 BoneWeightsOut;

out flat vec4 base_color_factor;
out flat uint64_t albedo_handle;
out flat uint64_t normal_handle;
out flat uint64_t met_rough_handle;
out flat uint64_t emissive_handle;
out flat uint64_t amb_occ_handle;
out flat vec4 emissive;
out flat float metallic_factor;
out flat float roughness_factor;

uniform mat4 vp;
#if !BINDLESS
    uniform uint draw_id;
#endif

void main() {
    #if BINDLESS
        Per_Object_Data obj_data = per_object_data[gl_DrawID];
    #else
        Per_Object_Data obj_data = per_object_data[draw_id];
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

    mat4 model = obj_data.model_matrix;

    //FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalize(mat3(obj_data.normal_matrix) * aNor);
    TexCoord = aTexCoord;
    Tangentout = normalize(mat3(obj_data.normal_matrix) * Tangent);
    Bitangentout = normalize(mat3(obj_data.normal_matrix) * Bitangent);
    BonesOut = BoneIds;
    BoneWeightsOut = BoneWeights;

    uint bone_offset = obj_data.bone_offset;
    mat4 bone_transform = bones[BoneIds[0] + bone_offset].transform * BoneWeights[0]; // add offset
    bone_transform += bones[BoneIds[1] + bone_offset].transform * BoneWeights[1];
    bone_transform += bones[BoneIds[2] + bone_offset].transform * BoneWeights[2];
    bone_transform += bones[BoneIds[3] + bone_offset].transform * BoneWeights[3];
    vec4 skinned_pos = bone_transform * vec4(aPos, 1.0);
    
    //FragPos = vec3(model * vec4(aPos, 1.0)); // todo change this to use bones
    FragPos = vec3(model * skinned_pos);

    gl_Position = vp * model * skinned_pos;
}
