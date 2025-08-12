#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

layout(binding = 0) uniform sampler2DArray CSM;

uniform float cascade_layer;

void main() {
    vec2 gridCoord = TexCoord * 2.0;
    int cascadeX = int(gridCoord.x);
    int cascadeY = int(gridCoord.y);
    int cascadeIndex = cascadeY * 2 + cascadeX;
    
    vec2 localUV = fract(gridCoord);
    float depth = texture(CSM, vec3(localUV, cascade_layer)).r;
    
    FragColor = vec4(vec3(depth), 1.0);
}