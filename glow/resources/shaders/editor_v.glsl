#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 Tangent;
layout (location = 4) in vec3 Bitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
// uniform mat3 normal_matrix;

void main() {
    /*FragPos = vec3(model * vec4(aPos, 1.0));  // Transform to world space

    Normal = normalize(normal_matrix * aNor);

    TexCoord = aTexCoord;

    Tangentout = normalize(normal_matrix * Tangent);
    Bitangentout = normalize(normal_matrix * Bitangent);*/

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
