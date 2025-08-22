#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

layout(binding = 0) uniform sampler2DArray CSM;

uniform float cascade_layer;

void main() {
    float depth = texture(CSM, vec3(TexCoord, cascade_layer)).r;
    FragColor = vec4(vec3(depth), 1.0);
}