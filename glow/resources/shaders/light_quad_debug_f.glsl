#version 460
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 color;
in vec2 out_pos;

void main() {
    if (length(out_pos) > .5)
        discard;

    FragColor = vec4(color, 1.0);
    BrightColor = vec4(0.0f);
}
