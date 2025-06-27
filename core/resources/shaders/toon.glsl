#version 460 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Tangentout;
in vec3 Bitangentout;

// Light & camera uniforms
uniform vec3 light_position;
uniform vec3 viewPos;
uniform vec3 light_color;
uniform float light_intensity;

// Texture uniforms
uniform bool has_diffuse;
uniform bool has_normal;
uniform bool has_metallic_roughness;
uniform sampler2D diffuse;
uniform sampler2D normal;
uniform sampler2D metallic_roughness;

// Toon shading parameters
uniform float toon_steps = 4.0;          // Number of lighting steps
uniform float toon_specular_steps = 2.0; // Specular steps
uniform float rim_power = 2.0;           // Rim lighting falloff
uniform float rim_intensity = 1.0;       // Rim lighting strength
uniform vec3 rim_color = vec3(1.0, 0.9, 0.8); // Rim light color

// PBR constants
const float PI = 3.14159265359;

// Toon step function - quantizes values
float toonStep(float value, float steps) {
    return floor(value * steps) / steps;
}

// Smooth toon step for softer transitions
float smoothToonStep(float value, float steps) {
    float stepped = floor(value * steps) / steps;
    float nextStep = (floor(value * steps) + 1.0) / steps;
    
    // Create smooth transition between steps
    float blend = smoothstep(stepped + 0.05, stepped + 0.1, value);
    return mix(stepped, nextStep, blend);
}

// Normal Distribution Function (simplified for toon)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

// Simplified geometry function for toon
float GeometrySmithToon(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    
    // Simplified for toon style
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float g1 = NdotV / (NdotV * (1.0 - k) + k);
    float g2 = NdotL / (NdotL * (1.0 - k) + k);
    
    return g1 * g2;
}

// Fresnel (simplified)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // Get normal
    vec3 N = normalize(Normal);
    if (has_normal) {
        vec3 normalMap = texture(normal, TexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        
        vec3 T = normalize(Tangentout);
        vec3 B = normalize(Bitangentout);
        vec3 Norm = normalize(Normal);
        mat3 TBN = mat3(T, B, Norm);
        N = normalize(TBN * normalMap);
    }
    
    // Get material properties
    vec3 albedo = texture(diffuse, TexCoord).rgb;
    float metallic = 0.0;
    float roughness = 0.5;
    
    if (has_metallic_roughness) {
        vec3 mrSample = texture(metallic_roughness, TexCoord).rgb;
        metallic = mrSample.b;
        roughness = mrSample.g;
    }
    
    // Calculate lighting vectors
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(light_position - FragPos);
    vec3 H = normalize(V + L);
    
    // Basic lighting calculations
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    
    // Distance and attenuation
    float distance = length(light_position - FragPos);
    float attenuation = light_intensity / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // === TOON LIGHTING ===
    
    // 1. Quantized diffuse lighting
    float toonNdotL = smoothToonStep(NdotL, toon_steps);
    vec3 diffuse_toon = toonNdotL * light_color * attenuation;
    
    // 2. PBR-based specular but quantized
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmithToon(N, V, L, roughness);
    vec3 F = fresnelSchlick(VdotH, F0);
    
    // Calculate specular intensity
    float spec_intensity = (NDF * G) / (4.0 * NdotV * NdotL + 0.001);
    
    // Quantize specular
    float toon_spec = toonStep(spec_intensity, toon_specular_steps);
    vec3 specular_toon = F * toon_spec * light_color * attenuation;
    
    // 3. Rim lighting for that anime look
    float rim_factor = 1.0 - NdotV;
    rim_factor = pow(rim_factor, rim_power);
    vec3 rim_light = rim_factor * rim_intensity * rim_color;
    
    // 4. Combine everything
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    // Diffuse contribution
    vec3 diffuse_contrib = kD * albedo * diffuse_toon;
    
    // Specular contribution (already includes F)
    vec3 specular_contrib = specular_toon;
    
    // Ambient (also quantized)
    float ambient_strength = 0.15;
    vec3 ambient = toonStep(ambient_strength, 3.0) * albedo * light_color;
    
    // Final color
    vec3 color = ambient + diffuse_contrib + specular_contrib + rim_light;
    
    // Optional: Add slight color grading for anime look
    color = mix(color, color * vec3(1.05, 1.0, 0.95), 0.1); // Slightly warm
    
    // Tone mapping (less aggressive for toon style)
    color = color / (color + vec3(1.5));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}