#version 460 core

out vec3 WorldPos;
// out vec4 LightSpacePos;

uniform mat4 vp = mat4(1.0);
// uniform mat4 gLightVP;
uniform float gGridSize = 100.0;
uniform vec3 gCameraWorldPos;

const vec3 Pos[4] = vec3[4](
    vec3(-1.0, 0.0, -1.0),      // bottom left
    vec3( 1.0, 0.0, -1.0),      // bottom right
    vec3( 1.0, 0.0,  1.0),      // top right
    vec3(-1.0, 0.0,  1.0)       // top left
);

const int Indices[6] = int[6](0, 2, 1, 2, 0, 3);

void main() {
    int Index = Indices[gl_VertexID];
    vec3 vPos3 = Pos[Index] * gGridSize;

    vPos3.x += gCameraWorldPos.x;
    vPos3.z += gCameraWorldPos.z;

    vec4 vPos4 = vec4(vPos3, 1.0);

    gl_Position = vp * vPos4;
    // LightSpacePos = gLightVP * vPos4;

    WorldPos = vPos3;
}