#version 460 core

in float Height;
in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

layout(binding = 1) uniform sampler2D heightmap_color;

uniform bool lines;

void main() {
	float h = (Height + 16)/64.0f;
	if (lines)
		FragColor = vec4(1.0, 0, 0, 1.0);
	else {
		vec3 color = texture(heightmap_color, TexCoord).rgb;
	    color = color / (color + vec3(1.0));
		color = pow(color, vec3(2.2));
		FragColor = vec4(color, 1.0);
	}
	//FragColor = vec4(1.0, 0, 0, 1.0);
	//BrightColor = vec4(1.0, 0, 0, 1.0);
}