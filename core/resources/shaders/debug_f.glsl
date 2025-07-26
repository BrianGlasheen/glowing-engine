#version 460 core

out vec4 FragColor;

in vec3 line_color;

void main() {
	FragColor = vec4(line_color, 1.0);
}
