#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 0) in vec3 aNor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float scale;

void main() {
    gl_Position = projection * view * model * vec4(aPos + aNor * scale, 1.0);
}
