#version 460 core

// Input uniforms for the Disney BRDF parameters
uniform vec3 base_color;     // Base color of the material
uniform float subsurface;    // Subsurface scattering amount (0-1)
uniform float metallic;      // Metallic property (0-1)
uniform float specular;      // Specular intensity (0-1)
uniform float specular_tint; // Tint specular using base_color (0-1)
uniform float roughness;     // Surface roughness (0-1)
uniform float anisotropic;   // Anisotropic reflection (0-1)
uniform float sheen;         // Sheen amount (0-1)
uniform float sheen_tint;    // Tint sheen using base_color (0-1)
uniform float clearcoat;     // Clearcoat layer amount (0-1)
uniform float clearcoat_gloss; // Clearcoat glossiness (0-1)
uniform float IOR;           // Index of refraction (typically 1.5 for most materials)

// Textures
uniform sampler2D base_color_map;
uniform sampler2D normal_map;
uniform sampler2D roughness_map;
uniform sampler2D metallic_map;
uniform bool has_base_color_map;
uniform bool has_normal_map;
uniform bool has_roughness_map;
uniform bool has_metallic_map;

// Texture coordinates adjustment
uniform vec2 texture_scale;
uniform vec2 texture_offset;

// Light properties
uniform vec3 light_position;
uniform vec3 light_color;
uniform float light_intensity;
uniform vec3 view_position;

// Input from vertex shader
in vec3 frag_position;
in vec3 frag_normal;
in vec2 frag_tex_coord;
in vec3 frag_tangent;
in vec3 frag_bitangent;

// Output
out vec4 frag_color;

const float PI = 3.14159265359;

// Square function (used frequently)
float sqr(float x) { return x*x; }

// Schlick Fresnel approximation
vec3 F_Schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

// GTR1 (Generalized Trowbridge-Reitz) distribution function - used for clearcoat
float GTR1(float NdotH, float a) {
    if (a >= 1.0) return 1.0/PI;
    float a2 = sqr(a);
    float t = 1.0 + (a2 - 1.0) * sqr(NdotH);
    return (a2 - 1.0) / (PI * log(a2) * t);
}

// GTR2 (Generalized Trowbridge-Reitz) distribution function - used for specular
float GTR2(float NdotH, float a) {
    float a2 = sqr(a);
    float t = 1.0 + (a2 - 1.0) * sqr(NdotH);
    return a2 / (PI * sqr(t));
}

// GTR2 anisotropic
float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay) {
    float a = sqr(HdotX / ax) + sqr(HdotY / ay);
    float t = 1.0 + a;
    return 1.0 / (PI * ax * ay * sqr(t));
}

// Smith GGX geometric shadowing function
float smithG_GGX(float NdotV, float alphaG) {
    float a = sqr(alphaG);
    float b = sqr(NdotV);
    return 1.0 / (NdotV + sqrt(a + b - a * b));
}

// Smith GGX geometric shadowing function for anisotropic
float smithG_GGX_aniso(float NdotV, float VdotX, float VdotY, float ax, float ay) {
    float a = sqr(VdotX * ax) + sqr(VdotY * ay);
    float b = NdotV * NdotV;
    return 1.0 / (NdotV + sqrt(a + b - a * b));
}

// Disney Diffuse term
float disney_diffuse(float NdotV, float NdotL, float LdotH, float roughness) {
    float FL = pow(1.0 - NdotL, 5.0);
    float FV = pow(1.0 - NdotV, 5.0);
    float Fd90 = 0.5 + 2.0 * sqr(LdotH) * roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);
    return Fd / PI;
}

// Disney Subsurface term
float disney_subsurface(float NdotV, float NdotL, float LdotH, float roughness) {
    float FL = pow(1.0 - NdotL, 5.0);
    float FV = pow(1.0 - NdotV, 5.0);
    float Fss90 = sqr(LdotH) * roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (NdotL + NdotV) - 0.5) + 0.5);
    return ss / PI;
}

void main() {
    // frag_color = vec4(1.0, 0.0, 0.0, 1.0);
    // return ;

    // Adjust texture coordinates
    vec2 tex_coord = frag_tex_coord * texture_scale + texture_offset;
    
    // Get base color (from texture or uniform)
    vec3 material_base_color = base_color;
    if (has_base_color_map) {
        material_base_color = texture(base_color_map, tex_coord).rgb;
    }
    
    // Get roughness (from texture or uniform)
    float material_roughness = roughness;
    if (has_roughness_map) {
        material_roughness = texture(roughness_map, tex_coord).r;
    }
    
    // Get metallic (from texture or uniform)
    float material_metallic = metallic;
    if (has_metallic_map) {
        material_metallic = texture(metallic_map, tex_coord).r;
    }
    
    // Calculate normal (from normal map or fragment normal)
    vec3 N = normalize(frag_normal);
    if (has_normal_map) {
        // TBN matrix for normal mapping
        vec3 T = normalize(frag_tangent);
        vec3 B = normalize(frag_bitangent);
        
        // Get normal from normal map and transform to world space
        vec3 normal_map_value = texture(normal_map, tex_coord).rgb * 2.0 - 1.0;
        mat3 TBN = mat3(T, B, N);
        N = normalize(TBN * normal_map_value);
    }
    
    // Setup lighting vectors
    vec3 V = normalize(view_position - frag_position);
    vec3 L = normalize(light_position - frag_position);
    vec3 H = normalize(L + V);
    
    // Tangent and bitangent for anisotropic calculations
    vec3 T = normalize(frag_tangent);
    vec3 B = normalize(frag_bitangent);
    
    // Dot products for BRDF
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);
    
    // Skip calculations for surfaces facing away from light
    if (NdotL == 0.0 || NdotV == 0.0) {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Calculate anisotropic values
    float HdotX = dot(H, T);
    float HdotY = dot(H, B);
    float VdotX = dot(V, T);
    float VdotY = dot(V, B);
    
    // Compute alpha (roughness adjustment)
    float alpha = max(0.001, sqr(material_roughness));
    float aspect = sqrt(1.0 - anisotropic * 0.9);
    float ax = max(0.001, alpha / aspect);
    float ay = max(0.001, alpha * aspect);

    // Luminance of base color
    float cdlum = 0.3 * material_base_color.r + 0.6 * material_base_color.g + 0.1 * material_base_color.b;
    
    // Normalize luminance to isolate hue and saturation
    vec3 ctint = cdlum > 0.0 ? material_base_color / cdlum : vec3(1.0);
    
    // Specular tint (lerp between base color hue and white)
    vec3 cspec0 = mix(specular * 0.08 * mix(vec3(1.0), ctint, specular_tint), material_base_color, material_metallic);
    
    // Sheen tint (lerp between white and base color hue)
    vec3 csheen = mix(vec3(1.0), ctint, sheen_tint);

    // BRDF Components
    float Fd = 0.0;  // Diffuse
    float Fs = 0.0;  // Specular
    float Fc = 0.0;  // Clearcoat
    
    // ------- Diffuse Component -------
    // Mix between diffuse and subsurface
    if (subsurface == 0.0) {
        Fd = disney_diffuse(NdotV, NdotL, LdotH, material_roughness);
    } else {
        float Fss = disney_subsurface(NdotV, NdotL, LdotH, material_roughness);
        Fd = mix(disney_diffuse(NdotV, NdotL, LdotH, material_roughness), Fss, subsurface);
    }
    
    // ------- Specular Component -------
    float Ds = 0.0;
    float Gs = 0.0;
    
    if (anisotropic == 0.0) {
        // Isotropic case
        Ds = GTR2(NdotH, alpha);
        Gs = smithG_GGX(NdotV, alpha) * smithG_GGX(NdotL, alpha);
    } else {
        // Anisotropic case
        Ds = GTR2_aniso(NdotH, HdotX, HdotY, ax, ay);
        Gs = smithG_GGX_aniso(NdotV, VdotX, VdotY, ax, ay) * 
             smithG_GGX_aniso(NdotL, dot(L, T), dot(L, B), ax, ay);
    }
    
    vec3 Fs_vec = Gs * Ds * F_Schlick(LdotH, cspec0);
    Fs = (Fs_vec.r + Fs_vec.g + Fs_vec.b) / 3.0;
    
    // ------- Sheen Component -------
    vec3 Fsheen = sheen * pow(1.0 - LdotH, 5.0) * csheen;
    
    // ------- Clearcoat Component -------
    if (clearcoat > 0.0) {
        float Dr = GTR1(NdotH, mix(0.1, 0.001, clearcoat_gloss));
        float Fr = mix(0.04, 1.0, pow(1.0 - LdotH, 5.0));
        float Gr = smithG_GGX(NdotL, 0.25) * smithG_GGX(NdotV, 0.25);
        Fc = clearcoat * Fr * Gr * Dr;
    }
    
    // Light attenuation (for point light)
    float distance = length(light_position - frag_position);
    float attenuation = light_intensity / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    vec3 radiance = light_color * attenuation;
    
    // Combine the components weighted by their respective parameters
    // (1-metallic) for diffuse since metals have no diffuse
    vec3 diffuse_color = (1.0 - material_metallic) * material_base_color * Fd * NdotL;
    vec3 specular_color = Fs_vec * NdotL;
    vec3 sheen_color = (1.0 - material_metallic) * Fsheen * material_base_color * NdotL;
    vec3 clearcoat_color = vec3(Fc * 0.25 * NdotL);
    
    // Final color is the sum of all components multiplied by light radiance
    vec3 final_color = (diffuse_color + specular_color + sheen_color + clearcoat_color) * radiance;
    
    // Add ambient light
    final_color += material_base_color * 0.03;

    // Apply tone mapping (simple Reinhard operator)
    final_color = final_color / (final_color + vec3(1.0));
    
    // Output final color (with gamma correction)
    frag_color = vec4(pow(final_color, vec3(1.0/2.2)), 1.0);
}