#version 460
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in float lifetime;

void main() {
    float alpha = clamp(lifetime / 10.0, 0.0, 1.0);
    //FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    //FragColor = vec4(1.0, 0.5, 0.1, alpha);
    BrightColor = 3.0 * vec4(1.0, 0.5, 0.1, alpha);
}
