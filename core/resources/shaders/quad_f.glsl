#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D scene_color;
uniform sampler2D bright_color;

void main() {
    FragColor = texture(scene_color, TexCoord) + texture(bright_color, TexCoord);
    //FragColor = texture(bright_color, TexCoord);
}