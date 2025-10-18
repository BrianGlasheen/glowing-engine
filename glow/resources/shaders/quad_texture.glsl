#version 460 core

in vec2 TexCoord;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2DArray CSM;
uniform float cascade_layer;

layout(binding = 1) uniform sampler2D tex;

uniform int mode = 0;

void main() {
    if (mode == 0) {
        float depth = texture(CSM, vec3(TexCoord, cascade_layer)).r;
        FragColor = vec4(vec3(depth), 1.0);
    }
    else if (mode == 1) {
        vec3 color = texture(tex, TexCoord).rgb;
        FragColor = vec4(color, 1.0);
    }
}