/*#version 460
layout (location = 0) in vec3 aPos;

struct Particle {
    vec3 position;
    float ttl;
    vec3 velocity;
    float max_ttl;
    vec4 color_start;
    vec4 color_end;
    float size_start;
    float size_end;
    vec2 padding;
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
    float size = particles[instance].position.w / 10.0;

    vec3 right = vec3(view[0][0], view[1][0], view[2][0]) * size;
    vec3 up = vec3(view[0][1], view[1][1], view[2][1]) * size;

    vec3 world_pos = pos + (aPos.x * right) + (aPos.y * up);
    gl_Position = projection * view * vec4(world_pos, 1.0);
    lifetime = particles[instance].position.w;
}
*/

#version 460
layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec2 aTexCoord;

struct Particle {
    vec3 position;
    float ttl;
    vec3 velocity;
    float max_ttl;
    vec4 color_start;
    vec4 color_end;
    float size_start;
    float size_end;
    vec2 padding;
};

layout(std430, binding = 0) buffer Particles {
    Particle particles[];
};

uniform mat4 view;
uniform mat4 projection;

out vec2 tex_coord;
out vec4 particle_color;
out float life_ratio;
out float alpha;

void main() {
    uint instance = gl_InstanceID;
    
    if (particles[instance].ttl > particles[instance].max_ttl) {
        gl_Position = vec4(0.0, 99999999.0, 0.0, 0.0);
        return;
    }
    
    float life_t = particles[instance].ttl /  particles[instance].max_ttl;
    
    // Interpolate size over lifetime
    float current_size = mix(particles[instance].size_start, particles[instance].size_end, life_t);
    
    // Billboard calculation
    vec3 right = vec3(view[0][0], view[1][0], view[2][0]) * current_size;
    vec3 up = vec3(view[0][1], view[1][1], view[2][1]) * current_size;
    
    vec3 pos = particles[instance].position;
    vec3 world_pos = pos + (aPos.x * right) + (aPos.y * up);
    gl_Position = projection * view * vec4(world_pos, 1.0);
    
    // Pass data to fragment shader
    //tex_coord = aTexCoord;
    particle_color = mix(particles[instance].color_start, particles[instance].color_end, life_t);
    life_ratio = life_t;
    alpha = particle_color.w;
}