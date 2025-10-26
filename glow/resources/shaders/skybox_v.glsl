#version 460 core

layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 Tangent;
layout (location = 2) in vec4 Bitangent;
layout (location = 3) in vec2 aTexCoord;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aPos.xyz;
    vec4 pos = projection * view * vec4(aPos.xyz, 1.0);
    //gl_Position = pos.xyww;
    gl_Position = vec4(pos.xy, 0.0, pos.w);
}  