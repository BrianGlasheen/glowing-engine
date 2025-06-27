#version 460 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;

// Basic lighting inputs
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    // Sample the base texture color
    vec4 texColor = texture(texture_diffuse1, TexCoords);

    // Basic diffuse lambert shading
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);

    // Combine texture color with diffuse lighting
    vec3 result = texColor.rgb * diff;

    FragColor = vec4(result, texColor.a);
}
