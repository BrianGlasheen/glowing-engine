#version 460
layout (location = 0) in vec3 aPos;

struct Particle {
    vec4 position;
    vec4 velocity;
};

layout(std430, binding = 0) buffer Particles {
    Particle particles[];
};

uniform mat4 view;
uniform mat4 projection;

out float lifetime;

void main() {
    uint instance = gl_InstanceID;
    vec3 pos = particles[instance].position.xyz;
    float size = 0.2;

    vec3 right = vec3(view[0][0], view[1][0], view[2][0]) * size;
    vec3 up = vec3(view[0][1], view[1][1], view[2][1]) * size;

    vec3 world_pos = pos + (aPos.x * right) + (aPos.y * up);
    gl_Position = projection * view * vec4(world_pos, 1.0);
    lifetime = particles[instance].position.w;
}