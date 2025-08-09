#version 460 core

layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec3 aNor;
//layout (location = 2) in vec2 aTexCoord;
//layout (location = 3) in vec3 Tangent;
//layout (location = 4) in vec3 Bitangent;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(aPos, 1.0);
}
