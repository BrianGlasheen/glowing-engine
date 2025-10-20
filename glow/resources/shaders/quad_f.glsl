#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D scene_color;
uniform sampler2D bright_color;
uniform sampler2D moment0;
uniform sampler2D moment1;
uniform sampler2D depth_texture;

void main() {
    //FragColor = texture(scene_color, TexCoord) + texture(bright_color, TexCoord);
    //FragColor = texture(bright_color, TexCoord);

    vec4 color = texture(scene_color, TexCoord);
    vec4 bloom = texture(bright_color, TexCoord);
    vec4 m0 = texture(moment0, TexCoord);
    vec4 m1 = texture(moment1, TexCoord);
    
    float b0 = m0.x;
    float b1 = m0.y;
    float b2 = m0.z;
    float b3 = m0.w;
    float b4 = m1.x;
    float b5 = m1.y;
    float b6 = m1.z;
    
    // no transparency at fragment
    if (b0 < 0.001) {
        vec4 col = color + bloom;
    
        vec3 outcol = col.rgb / (col.rgb + vec3(1.0));
        FragColor = vec4(pow(outcol, vec3(1.0/2.2)), 1.0);

        return;
    }
    
    // todo implement actual formula
    float transmittance = exp(-b0 * 4.0);
    
    vec3 resolved = color.rgb + color.rgb * transmittance;
    
    resolved = resolved / (resolved + vec3(1.0));
    resolved = pow(resolved, vec3(1.0/2.2));
    
    FragColor = vec4(resolved, 1.0) + bloom;
}
