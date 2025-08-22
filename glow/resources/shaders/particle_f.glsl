#version 460

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in float lifetime;

//in vec2 tex_coord;
in vec4 particle_color;
in float life_ratio;
in float alpha;

void main() {
    if (life_ratio > 1.0 || life_ratio < 0.0)
        return ;
    //FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    FragColor = vec4(particle_color.xyz, life_ratio);
    BrightColor = vec4(25.0 * particle_color.xyz, life_ratio);
}
