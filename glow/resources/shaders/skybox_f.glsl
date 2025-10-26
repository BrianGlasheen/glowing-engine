#version 460 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out uint PickingId; // todo put behind #def prob
layout(location = 3) out vec4 moment0; // b0, b1, b2, b3
layout(location = 4) out vec4 moment1; // b4, b5, b6, transmittance

in vec3 TexCoords;

layout(binding = 30) uniform samplerCube skybox;

// uniform vec3 view_dir;
// uniform vec3 sun_dir;
// uniform vec3 cam_pos;

void main() {
    bool mode = true;
    if (mode) {
        FragColor = texture(skybox, TexCoords);
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
    else {
        // yes
    }
}