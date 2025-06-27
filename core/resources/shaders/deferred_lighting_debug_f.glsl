#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedo_specular;
uniform int debug_mode;

void main() {
    vec4 result = vec4(0.0);
    
    switch(debug_mode) {
        case 0: // Position (RGB)
            result = vec4(texture(g_position, TexCoords).rgb, 1.0);
            break;
        case 1: // Normal (RGB)
            result = vec4(texture(g_normal, TexCoords).rgb, 1.0);
            break;
        case 2: // Albedo (RGB)
            result = vec4(texture(g_albedo_specular, TexCoords).rgb, 1.0);
            break;
        case 3: // Specular (Alpha of Albedo Texture)
            float specular = texture(g_albedo_specular, TexCoords).a;
            result = vec4(specular, specular, specular, 1.0);
            break;
        case 4: // Depth Visualization
            float depth = gl_FragCoord.z;
            result = vec4(vec3(depth), 1.0);
            break;
        default:
            result = vec4(1.0, 0.0, 1.0, 1.0); // Magenta error color
    }
    
    FragColor = result;
}