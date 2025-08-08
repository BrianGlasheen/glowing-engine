/*
#version 460 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Tangentout;
in vec3 Bitangentout;
in flat uvec4 BonesOut;
in vec4 BoneWeightsOut;

uniform uint bone = 0;

void main() { 
    bool found = false;

    //FragColor = vec4(BoneWeightsOut[0], 0.0, 0.0, 1.0);
    //return ;
    vec3 col;

    for (int i = 0; i < 4; i++) {
        if (BonesOut[i] == bone) {
            if (BoneWeightsOut[i] >= 0.7)
                col = vec3(1.0, 0.0, 0.0) * BoneWeightsOut[i];
            else if (BoneWeightsOut[i] >= 0.4 && BoneWeightsOut[i] < 0.7)
                col = vec3(0.0, 1.0, 0.0) * BoneWeightsOut[i];
            else if (BoneWeightsOut[i] >= 0.1)
                col = vec3(1.0, 1.0, 0.0) * BoneWeightsOut[i];
            
            found = true;
            break;
        }
    }
    
    if (!found)
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    else
        FragColor = vec4(col, 1.0);
}
*/


#version 460 core
#extension GL_ARB_bindless_texture : require
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
    uint lightIndices[199];
};

layout(std430, binding = 1) restrict buffer clusterSSBO {
    Cluster clusters[];
};

layout(std430, binding = 2) restrict buffer lightSSBO {
    GPU_Light lights[];
};

layout(binding = 0) uniform sampler2D ssao;

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

uniform bool use_alpha_clipping; 
float alpha_cutoff = 0.5;
uniform bool shadows_enabled;
uniform bool ssao_enabled;

uniform vec3 view_pos;
uniform int num_lights;
uniform bool forward_plus;
uniform float zNear;
uniform float zFar;
uniform mat4 viewMatrix;
uniform uvec3 gridSize;
uniform uvec2 screenDimensions;

//uniform sampler2D shadow_map; // todo shadow atlas

//uniform sampler2D directional_shadow_map; // todo CSM
//uniform vec3 directional_light_direction;
//uniform vec3 directional_light_color;
//uniform float directional_light_intensity;

const float PI = 3.14159265359;
float ambient_light = .03;

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

void main() { 

    //FragColor = vec4(vec3(gl_FragCoord.w), 1.0);
    //FragColor = color;
    //FragColor = vec4(metallic_factor, 0, 0, 1);'

    //vec2 screenUV = gl_FragCoord.xy / vec2(1600.0, 900.0);
    //vec4 ssaoValue = texture(ssao, screenUV);
    //FragColor = vec4(1.0, 0.0, 0.0, 1);
    //return ;

    vec3 albedo;
    float alpha;

    vec4 baseColorSample = vec4(1.0);
    if (albedo_handle != 0) {
        sampler2D albedo_texture = sampler2D(albedo_handle);
        baseColorSample = texture(albedo_texture, TexCoord);
    }
    vec4 baseColor = base_color_factor * baseColorSample;
    albedo = baseColor.rgb;
    alpha = baseColor.a;
    
    if (alpha < alpha_cutoff) {
        discard;
    }

    //FragColor = vec4(baseColorSample.rgb, 1.0);
    //return ;

    vec3 N = normalize(Normal);
    if (normal_handle != 0) {
        sampler2D normal_texture = sampler2D(normal_handle);
        vec3 normalMap = texture(normal_texture, TexCoord).rgb;
        normalMap = normalMap * 2.0 - 1.0;
    
        vec3 T = normalize(Tangentout);
        vec3 B = normalize(Bitangentout);
        mat3 TBN = mat3(T, B, N);
        N = normalize(TBN * normalMap);
    }
    
    sampler2D metallic_roughness = sampler2D(met_rough_handle);
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
    //vec3 diffuse_color = albedo * (1.0 - metallic);
    //Lo += CalculatePointLight(N, V, F0, diffuseColor, metallic, roughness, light);

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
    //Lo += CalculateDirectionalLight(N, V, F0, albedo, metallic, roughness); // todo CSM

    if (emissive_handle != 0) {
        sampler2D emissive_texture = sampler2D(emissive_handle);
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
        sampler2D occlusion_texture = sampler2D(amb_occ_handle);
        ao = texture(occlusion_texture, TexCoord).r; 
    }

    float ssao_val = 1.0;
    //if (ssao_enabled) {
    //    vec2 screenUV = gl_FragCoord.xy / vec2(1600.0, 900.0);
    //    ssao_val = texture(ssao, screenUV).r;
    //}

    vec3 ambient = vec3(ambient_light) * albedo * ao * ssao_val;
    vec3 color = ambient + Lo;
    
    // HDR tonemapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}