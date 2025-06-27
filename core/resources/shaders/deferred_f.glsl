#version 460 core
layout (location = 0) out vec3 g_position;
layout (location = 1) out vec3 g_normal;
layout (location = 2) out vec4 g_albedo_specular;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main() {    
    // store the fragment position vector in the first gbuffer texture
    g_position = FragPos;
    // also store the per-fragment normals into the gbuffer
    g_normal = normalize(Normal);
    // and the diffuse per-fragment color
    g_albedo_specular.rgb = texture(texture_diffuse1, TexCoords).rgb;
    // store specular intensity in gAlbedoSpec's alpha component
    g_albedo_specular.a = texture(texture_specular1, TexCoords).r;
}  