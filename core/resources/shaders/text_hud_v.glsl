#version 460
layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 tex;

out vec2 TexCoords;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = tex;
}
