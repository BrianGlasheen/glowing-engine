#version 460 core

out vec4 FragColor;

uniform bool uniform_color;
uniform vec3 color;

in vec3 line_color;

void main() {
	if (uniform_color)
		FragColor = vec4(color, 1.0);
	else
		FragColor = vec4(line_color, 1.0);
}
