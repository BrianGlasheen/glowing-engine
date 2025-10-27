#version 460
layout (location = 0) in vec3 aPos;

struct GPU_Light {
    vec4 position_radius; // x, y ,z, radius
    vec4 color_strength; // r g b intensity
    vec4 direction_type; // x y z type
    vec4 params; // inner cone, outer cone, shadow map idx, unused 
};

layout(std430, binding = 0) restrict buffer lightSSBO {
    GPU_Light lights[];
};

uniform mat4 view;
uniform mat4 projection;

out vec3 color;
out vec2 out_pos;

void main() {
    vec3 right = vec3(view[0][0], view[1][0], view[2][0]); // todo maybe size based on radius / strength
    vec3 up = vec3(view[0][1], view[1][1], view[2][1]);
    
    vec3 pos = lights[gl_InstanceID].position_radius.xyz;
    vec3 world_pos = pos + (aPos.x * right) + (aPos.y * up);

    color = lights[gl_InstanceID].color_strength.rgb;
    
    gl_Position = projection * view * vec4(world_pos, 1.0);
    out_pos = aPos.xy;
}