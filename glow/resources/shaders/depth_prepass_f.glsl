#version 460
#define BINDLESS 0

#if BINDLESS
    #extension GL_ARB_bindless_texture : require
#endif

#extension GL_ARB_gpu_shader_int64: enable

#if !BINDLESS
    layout(binding = 0) uniform sampler2D albedo_texture;
#endif

in vec2 TexCoord;

in flat uint64_t albedo_handle;
in flat vec4 base_color_factor;
in flat float alpha_cutoff;

// todo add opacity
void main() {
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

}