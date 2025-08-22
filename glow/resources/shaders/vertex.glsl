#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 Tangent;
layout (location = 4) in vec3 Bitangent;

uniform mat4 model;
uniform mat4 mvp;
uniform mat3 normal_matrix;
uniform mat4 light_view;
uniform mat4 light_projection;
uniform mat4 dir_light_view;
uniform mat4 dir_light_projection;

out vec3 FragPos;  // position in world space
out vec4 FragPosLight;  // position in world space
out vec4 FragPosLightDirectional;  // position in world space
out vec3 Normal;   // normal in world space
out vec2 TexCoord;
out vec3 Tangentout;
out vec3 Bitangentout;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    FragPosLight = light_projection * light_view * vec4(FragPos, 1.0);
    FragPosLightDirectional = dir_light_projection * dir_light_view * vec4(FragPos, 1.0);

    Normal = normalize(normal_matrix * aNor);

    TexCoord = aTexCoord;

    Tangentout = normalize(normal_matrix * Tangent);
    Bitangentout = normalize(normal_matrix * Bitangent);

    gl_Position = mvp * vec4(aPos, 1.0);
}
