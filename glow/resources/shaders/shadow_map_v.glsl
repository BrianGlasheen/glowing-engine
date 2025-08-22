#version 330

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view; // light view
uniform mat4 projection; // light proj

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}