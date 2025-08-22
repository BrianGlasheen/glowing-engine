#version 460 core

struct GPU_Light {
    vec4 position_radius; // x, y ,z, radius
    vec4 color_strength; // r g b intensity
    vec4 direction_type; // x y z type
    vec4 params; // inner cone, outer cone, shadow map idx, unused 
};

struct Cluster {
    vec4 minPoint;
    vec4 maxPoint;
    uint count;
    uint lightIndices[199];
};

layout(std430, binding = 1) restrict buffer clusterSSBO {
    Cluster clusters[];
};

layout(std430, binding = 2) restrict buffer lightSSBO {
    GPU_Light lights[];
};

uniform int num_lights;
uniform bool forward_plus;
uniform float zNear;
uniform float zFar;
uniform mat4 viewMatrix;
uniform uvec3 gridSize;
uniform uvec2 screenDimensions;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec4 FragPosLight;
in vec4 FragPosLightDirectional;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Tangentout;
in vec3 Bitangentout;

uniform bool use_alpha_clipping; 
uniform float alpha_cutoff;
uniform bool shadows_enabled;

uniform float ambient_light;

//uniform vec3 point_light_position;
//uniform vec3 point_light_color;
//uniform float point_light_intensity;
//uniform float point_light_far_plane;

uniform vec3 directional_light_direction;
uniform vec3 directional_light_color;
uniform float directional_light_intensity;

uniform vec3 spot_light_position;
uniform vec3 spot_light_direction;
uniform vec3 spot_light_color;
uniform float spot_light_intensity;
uniform float spot_light_inner_cone;
uniform float spot_light_outer_cone;

uniform vec3 viewPos;

uniform bool has_diffuse;
uniform bool has_normal;
uniform bool has_metallic_roughness;
// uniform sampler2D diffuse;
// uniform sampler2D normal;
// uniform sampler2D metallic_roughness;
layout(binding = 0) uniform sampler2D diffuse;
layout(binding = 1) uniform sampler2D normal;
layout(binding = 2) uniform sampler2D metallic_roughness;

uniform sampler2D shadow_map; // spotlight
uniform sampler2D directional_shadow_map;
uniform samplerCube point_shadow_map;

const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
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

// Geometry function (Smith's method)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel equation (Schlick's approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// todo point light shadow calc
float PointShadowCalculationPCF(vec3 fragPos, vec3 lightPos, float farPlane) {
    vec3 fragToLight = fragPos - lightPos;

    fragToLight.x *= -1;
    //fragToLight.y *= -1;
    fragToLight.z *= -1;
    
    float currentDepth = length(fragToLight);
    
    float bias = 0.05;
    float shadow = 0.0;
    float samples = 4.0;
    float offset = 0.01;
    
    for(float x = -offset; x < offset; x += offset / (samples * 0.5)) {
        for(float y = -offset; y < offset; y += offset / (samples * 0.5)) {
            for(float z = -offset; z < offset; z += offset / (samples * 0.5)) {
                float closestDepth = texture(point_shadow_map, fragToLight + vec3(x, y, z)).r;
                if(currentDepth - bias > closestDepth)
                    shadow += 1.0;
            }
        }
    }
    shadow /= (samples * samples * samples);
    
    return shadow;
}

float PointShadowCalculation(vec3 fragPos, vec3 lightPos, float farPlane) {
    vec3 fragToLight = fragPos - lightPos;
    
    fragToLight.x *= -1;
    //fragToLight.y *= -1;
    fragToLight.z *= -1;
    
    float currentDepth = length(fragToLight);
    float closestDepth = texture(point_shadow_map, normalize(fragToLight)).r;
    
    float bias = 0.1;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    return shadow;
}

float SpotShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = 0.005;
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadow_map, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadow_map, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

float DirectionalShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = 0.005;
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(directional_shadow_map, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(directional_shadow_map, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

vec3 CalculateLighting(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 H = normalize(V + L);
    
    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 CalculatePointLight(vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness, GPU_Light light) {
    vec3 point_light_position = light.position_radius.xyz;
    vec3 point_light_color = light.color_strength.xyz;
    float point_light_intensity = light.color_strength.w;

    vec3 L = normalize(point_light_position - FragPos);
    float distance = length(point_light_position - FragPos);
    if (distance > light.position_radius.w)
        return vec3(0);
    float attenuation = point_light_intensity / (distance * distance);
    vec3 radiance = point_light_color * attenuation;
    
    vec3 lighting = CalculateLighting(L, radiance, N, V, F0, albedo, metallic, roughness);
    
    //float shadow = PointShadowCalculationPCF(FragPos, point_light_position, point_light_far_plane);
    float shadow;
    //if (shadows_enabled)
        //PointShadowCalculation(FragPos, point_light_position, point_light_far_plane);
    //else
        shadow = 0.0;

    return lighting * (1.0 - shadow);
}
vec3 CalculateDirectionalLight(vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 L = normalize(-directional_light_direction); // Light direction points towards the light
    vec3 radiance = directional_light_color * directional_light_intensity;
    
    vec3 lighting = CalculateLighting(L, radiance, N, V, F0, albedo, metallic, roughness);
        
    float shadow;
    if (shadows_enabled)
        shadow = DirectionalShadowCalculation(FragPosLightDirectional);
    else
        shadow = 0.0;

    return lighting * (1.0 - shadow);
}

vec3 CalculateSpotLight(vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness) {
    vec3 L = normalize(spot_light_position - FragPos);
    float distance = length(spot_light_position - FragPos);
    
    vec3 spotDir = normalize(spot_light_direction);
    float theta = dot(L, -spotDir);
    
    float epsilon = spot_light_inner_cone - spot_light_outer_cone;
    float intensity = clamp((theta - spot_light_outer_cone) / epsilon, 0.0, 1.0);
    
    float attenuation = spot_light_intensity / (distance * distance);
    vec3 radiance = spot_light_color * attenuation * intensity;
    
    vec3 lighting = CalculateLighting(L, radiance, N, V, F0, albedo, metallic, roughness);
    
    float shadow;
    if (shadows_enabled)
        shadow = SpotShadowCalculation(FragPosLight);
    else
        shadow = 0.0;
    
    return lighting * (1.0 - shadow);
}

void main() { 

    //FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    //return ;

    vec4 diffuseSample = texture(diffuse, TexCoord);
    vec3 albedo = diffuseSample.rgb;
    float alpha = diffuseSample.a;
    
    if (use_alpha_clipping && alpha < alpha_cutoff) {
        discard;
    }

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
    
    float metallic = 0.0;
    float roughness = 0.5;
    
    if (has_metallic_roughness) {
        vec3 mrSample = texture(metallic_roughness, TexCoord).rgb;
        metallic = mrSample.b;
        roughness = mrSample.g;
    }
    
    // view direction
    vec3 V = normalize(viewPos - FragPos);
    
    // F0
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Locating which cluster this fragment is part of
    vec3 fragViewPos = vec3(viewMatrix * vec4(FragPos, 1.0));
    uint zTile = uint((log(abs(fragViewPos.z) / zNear) * gridSize.z) / log(zFar / zNear));
    vec2 tileSize = screenDimensions / gridSize.xy;
    uvec3 tile = uvec3(gl_FragCoord.xy / tileSize, zTile);
    uint tileIndex = tile.x + (tile.y * gridSize.x) + (tile.z * gridSize.x * gridSize.y);

    uint light_count;
    if (forward_plus)
        light_count = clusters[tileIndex].count;
    else
        light_count = num_lights;

    //float normalizedCount = float(light_count) / 200.0;
    //FragColor = vec4(normalizedCount, 0.0, 0.0, 1.0);
    //return;

    //float normalizedCount = float(tileIndex) / 16 * 9 * 24;
    //FragColor = vec4(normalizedCount, 0.0, 0.0, 1.0);
    //return;

    //float normalizedZ = float(zTile) / float(gridSize.z);
    //FragColor = vec4(normalizedZ, 0.0, 1.0 - normalizedZ, 1.0);
    //return;

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < light_count; i++) {
        GPU_Light light;
        if (forward_plus) {
            uint lightIndex = clusters[tileIndex].lightIndices[i];
            light = lights[lightIndex];
        } else {
            light = lights[i];
        }

        int light_type = int(light.direction_type.w);

        if (light_type == 0) {
            Lo += CalculatePointLight(N, V, F0, albedo, metallic, roughness, light);
        }
        else if (light_type == 1) {
            //Lo += CalculateSpotLight(N, V, F0, albedo, metallic, roughness, light);
        }
    }
    //Lo += CalculatePointLight(N, V, F0, albedo, metallic, roughness);
    Lo += CalculateDirectionalLight(N, V, F0, albedo, metallic, roughness);
    //Lo += CalculateSpotLight(N, V, F0, albedo, metallic, roughness);

    float brightness = dot(Lo, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        BrightColor = vec4(Lo, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    vec3 ambient = vec3(ambient_light) * albedo;
    vec3 color = ambient + Lo;
    
    // HDR tonemapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}