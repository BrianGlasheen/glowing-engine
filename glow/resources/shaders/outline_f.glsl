#version 460 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out uint PickingId; // todo put behind #def prob

uniform vec3 color;

void main() {
    FragColor = vec4(color, 1.0);
}