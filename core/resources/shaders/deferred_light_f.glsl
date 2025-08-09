#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedo_specular;

uniform int debug_mode;

struct Light {
    vec3 Position;
    vec3 Color;
    float Linear;
    float Quadratic;
    float Intensity;
};

uniform Light light;
uniform vec3 viewPos;

void main() {
    if (debug_mode == 999) {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Bright green
        return;
    }

    // Retrieve data from G-buffer
    vec3 FragPos = texture(g_position, TexCoords).rgb;
    vec3 Normal = texture(g_normal, TexCoords).rgb;
    vec3 Albedo = texture(g_albedo_specular, TexCoords).rgb;
    float Specular = texture(g_albedo_specular, TexCoords).a;

    // Blinn-Phong lighting calculations
    vec3 lighting = Albedo * 0.1; // ambient component
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(light.Position - FragPos);
    
    // Diffuse
    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = light.Color * diff * Albedo;
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
    vec3 specular = light.Color * spec * Specular;
    
    // Attenuation
    float distance = length(light.Position - FragPos);
    float attenuation = 1.0 / (1.0 + light.Linear * distance + 
                                light.Quadratic * (distance * distance));
    
    diffuse *= attenuation;
    specular *= attenuation;
    
    lighting += diffuse + specular;
    
    FragColor = vec4(lighting, 1.0);
}