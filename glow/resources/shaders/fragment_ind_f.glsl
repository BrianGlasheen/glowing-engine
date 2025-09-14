#version 460 core
#define BINDLESS 0

#if BINDLESS
    #extension GL_ARB_bindless_texture : require
#endif

#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

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
    uint lightIndices[99];
};

layout(std430, binding = 1) restrict buffer clusterSSBO {
    Cluster clusters[];
};

layout(std430, binding = 2) restrict buffer lightSSBO {
    GPU_Light lights[];
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Tangentout;
in vec3 Bitangentout;

in flat vec4 base_color_factor;
in flat uint64_t albedo_handle;
in flat uint64_t normal_handle;
in flat uint64_t met_rough_handle;
in flat uint64_t emissive_handle;
in flat uint64_t amb_occ_handle;
in flat vec4 emissive;
in flat float metallic_factor;
in flat float roughness_factor;
in flat float alpha_cutoff;

#if !BINDLESS
    layout(binding = 0) uniform sampler2D albedo_texture;
    layout(binding = 1) uniform sampler2D normal_texture;
    layout(binding = 2) uniform sampler2D metallic_roughness;
    layout(binding = 3) uniform sampler2D emissive_texture;
    layout(binding = 4) uniform sampler2D occlusion_texture;
#endif

layout(binding = 7) uniform sampler2D ssao;
layout(binding = 8) uniform sampler2DArray directional_shadow_map;
layout(binding = 9) uniform samplerCube skybox;
uniform uint num_skybox_mips;

uniform bool use_alpha_clipping; 
//float alpha_cutoff = 0.5;
uniform bool shadows_enabled;
uniform bool ssao_enabled;
uniform bool blend;

uniform vec3 view_pos;
uniform int num_lights;
uniform bool forward_plus;
uniform float zNear;
uniform float zFar;
uniform mat4 viewMatrix;
uniform uvec3 gridSize;
uniform uvec2 screenDimensions;

uniform mat4 cascade_matrices[4];
uniform float cascade_distances[5];
uniform int num_cascades;
uniform vec3 directional_light_direction;
uniform vec3 directional_light_color;
uniform float directional_light_intensity;

//uniform sampler2D shadow_map; // todo shadow atlas

const float PI = 3.14159265359;
const float ambient_light = .001;

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

int GetCascadeIndex(float depth) {
    for (int i = 0; i < num_cascades; i++) {
        if (depth < cascade_distances[i + 1]) {
            return i;
        }
    }
    return num_cascades - 1;
}

float DirectionalShadowCalculation(vec3 fragPosWorldSpace, vec3 fragViewPos) {
    int cascadeIndex = GetCascadeIndex(abs(fragViewPos.z));

    vec4 fragPosLightSpace = cascade_matrices[cascadeIndex] * vec4(fragPosWorldSpace, 1.0);

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float shadowDepth = texture(directional_shadow_map, vec3(projCoords.xy, cascadeIndex)).r;

    float currentDepth = projCoords.z;
  //  float bias = max(0.0005 * (1.0 - dot(normalize(fragNormal), -directional_light_direction)), 0.00005);
    float shadow = currentDepth + 0.005 < shadowDepth ? 1.0 : 0.0;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        shadow = 0.0;
    }

    return shadow;
}

vec3 CalculateDirectionalLight(vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness, vec3 fragViewPos) {
    vec3 L = normalize(-directional_light_direction);
    vec3 radiance = directional_light_color * directional_light_intensity;
    
    vec3 lighting = CalculateLighting(L, radiance, N, V, F0, albedo, metallic, roughness);
    
    float shadow = 0.0;
    //if (shadows_enabled) {
        shadow = DirectionalShadowCalculation(FragPos, fragViewPos);
    //}
    return lighting * (1.0 - shadow);
}

vec3 CalculateEnvironmentReflection(vec3 N, vec3 V, vec3 F0, float roughness, float metallic) {
    vec3 R = reflect(-V, N);
    
    float mip = roughness * (num_skybox_mips - 1);
    //if (mip > 2)
      //  mip = 2;
    vec3 envColor = textureLod(skybox, R, mip).rgb;
    
    float cosTheta = max(dot(N, V), 0.0);
    vec3 F = fresnelSchlick(cosTheta, F0);
    
    vec3 kS = F;
    float reflectionStrength = metallic; // todo
    
    return envColor * kS * reflectionStrength * 1.0;
}

void main() { 

/*
    vec3 fragViewPos2 = vec3(viewMatrix * vec4(FragPos, 1.0));
    float depth = abs(fragViewPos2.z);
    int cascadeIndex = GetCascadeIndex(depth);

    vec3 color23;
    if (cascadeIndex == 0)
        color23 = vec3(1.0, 0.0, 0.0);
    if (cascadeIndex == 1)
        color23 = vec3(0.0, 1.0, 0.0);
    if (cascadeIndex == 2)
        color23 = vec3(0.0, 0.0, 1.0);
    if (cascadeIndex == 3)
        color23 = vec3(0.0, 1.0, 1.0);
    if (cascadeIndex == 4)
        color23 = vec3(1.0, 1.0, 1.0);

    FragColor = vec4(color23, 1.0);
    return;
    */

    //FragColor = vec4(vec3(gl_FragCoord.w), 1.0);
    //FragColor = color;
    //FragColor = vec4(metallic_factor, 0, 0, 1);'

    //vec2 screenUV = gl_FragCoord.xy / vec2(1600.0, 900.0);
    //vec4 ssaoValue = texture(ssao, screenUV);
    //FragColor = vec4(ssaoValue.rgb, 1);
    //return ;

    vec3 albedo;
    float alpha;

    vec4 baseColorSample = vec4(1.0);
    if (albedo_handle != 0) {
        #if BINDLESS
            sampler2D albedo_texture = sampler2D(albedo_handle);
        #endif
        baseColorSample = texture(albedo_texture, TexCoord);
    }
    vec4 baseColor = base_color_factor * baseColorSample;
    albedo = baseColor.rgb;
    alpha = baseColor.a;
    
    if (alpha < alpha_cutoff) {
        discard;
    }

    vec3 N = normalize(Normal);
    if (normal_handle != 0) {
        #if BINDLESS
            sampler2D normal_texture = sampler2D(normal_handle);
        #endif
        vec3 normalMap = texture(normal_texture, TexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0;
    
        vec3 T = normalize(Tangentout);
        vec3 B = normalize(Bitangentout);
        mat3 TBN = mat3(T, B, N);
        N = normalize(TBN * normalMap);
    }
    
    #if BINDLESS
        sampler2D metallic_roughness = sampler2D(met_rough_handle); // todo maybe check
    #endif
    vec3 mrSample = texture(metallic_roughness, TexCoord).rgb;
    float metallic = mrSample.b * metallic_factor;
    float roughness = mrSample.g * roughness_factor;
    
    // view direction
    vec3 V = normalize(view_pos - FragPos);
    
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
    Lo += CalculateDirectionalLight(N, V, F0, albedo, metallic, roughness, fragViewPos); // todo CSM

    vec3 envReflection = CalculateEnvironmentReflection(N, V, F0, roughness, metallic);
    Lo += envReflection;

    if (emissive_handle != 0) {
        #if BINDLESS
            sampler2D emissive_texture = sampler2D(emissive_handle);
        #endif
        Lo += texture(emissive_texture, TexCoord).rgb * emissive.rgb * emissive.a;
    } 
    else
        Lo += emissive.rgb * emissive.a;

    float brightness = dot(Lo, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        BrightColor = vec4(Lo, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    float ao = 1.0;
    if (amb_occ_handle != 0) {
        #if BINDLESS
            sampler2D occlusion_texture = sampler2D(amb_occ_handle);
        #endif
        ao = texture(occlusion_texture, TexCoord).r; 
    }

    float ssao_val = 1.0;
    //if (ssao_enabled) {
        vec2 screenUV = gl_FragCoord.xy / vec2(1600.0, 900.0);
        ssao_val = texture(ssao, screenUV).r;
        ssao_val = 0.0;
    //}

    vec3 ambient = vec3(ambient_light) * albedo * ao * ssao_val;
    vec3 color = ambient + Lo;
    
    // HDR tonemapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    if (blend)
        FragColor = vec4(color, alpha);
    else
        FragColor = vec4(color, 1.0);
}