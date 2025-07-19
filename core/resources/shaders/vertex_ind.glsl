#version 460 core
#extension GL_ARB_gpu_shader_int64: enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 Tangent;
layout (location = 4) in vec3 Bitangent;

uniform mat4 vp;

struct Per_Object_Data {
    mat4 model_matrix;
    mat4 normal_matrix;
    vec4 color;
    uint64_t albedo;
    uint64_t normal;
};

layout(std430, binding = 0) readonly buffer per_object_ssbo {
    Per_Object_Data per_object_data[];
};

//uniform mat3 normal_matrix;
//uniform mat4 light_view;
//uniform mat4 light_projection;
//uniform mat4 dir_light_view;
//uniform mat4 dir_light_projection;

out vec3 FragPos;  // position in world space
out vec4 FragPosLight;  // position in world space
out vec4 FragPosLightDirectional;  // position in world space
out vec3 Normal;   // normal in world space
out vec2 TexCoord;
out vec3 Tangentout;
out vec3 Bitangentout;

out flat uint64_t albedo_handle;
out flat uint64_t normal_handle;

out vec4 color;

void main() {
    Per_Object_Data obj_data = per_object_data[gl_DrawID];
    
    mat4 model = obj_data.model_matrix;
    //model = mat4(0.01);
    albedo_handle = obj_data.albedo;
    normal_handle = obj_data.normal;
    color = obj_data.color;

    FragPos = vec3(model * vec4(aPos, 1.0));
    //FragPosLight = light_projection * light_view * vec4(FragPos, 1.0);
    //FragPosLightDirectional = dir_light_projection * dir_light_view * vec4(FragPos, 1.0);

    //Normal = normalize(normal_matrix * aNor);

    TexCoord = aTexCoord;

    //Tangentout = normalize(normal_matrix * Tangent);
    //Bitangentout = normalize(normal_matrix * Bitangent);

    gl_Position = vp * model * vec4(aPos, 1.0);
}
