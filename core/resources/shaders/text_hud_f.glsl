#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D msdfTexture;
uniform vec3 textColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture(msdfTexture, TexCoords).rgb;
    float sd = median(msd.r, msd.g, msd.b);

    float screenPxDistance = (sd - 0.5) / fwidth(sd);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    
    color = vec4(opacity * textColor, opacity);
}
