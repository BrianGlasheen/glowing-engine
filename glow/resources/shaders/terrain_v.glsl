#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

const int NUM_CASCADES = 4;
uniform mat4 cascade_matrices[NUM_CASCADES];
uniform mat4 playerViewMatrix;
out vec4 vs_light_space_pos[NUM_CASCADES];
out float vs_view_space_z;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTex;

    for (int i = 0 ; i < NUM_CASCADES ; i++)
        vs_light_space_pos[i] = cascade_matrices[i] * vec4(aPos, 1.0);

    vs_view_space_z = -(playerViewMatrix * vec4(aPos, 1.0)).z;
}