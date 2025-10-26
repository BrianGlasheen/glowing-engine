#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D scene_color;
uniform sampler2D bright_color;
uniform sampler2D moment0;
uniform sampler2D moment1;
uniform sampler2D depth_texture;

float transmittance() {
    vec4 m0 = texture(moment0, TexCoord);
    float b0 = m0.x;

    if (b0 < 0.0001)
        return 1.0;

    vec4 m1 = texture(moment1, TexCoord);
    float b1 = m0.y;
    float b2 = m0.z;
    float b3 = m0.w;
    float b4 = m1.x;
    float b5 = m1.y;
    float b6 = m1.z;
    
    // normalize
    float b_1 = b1 / b0;
    float b_2 = b2 / b0;
    float b_3 = b3 / b0;
    float b_4 = b4 / b0;
    float b_5 = b5 / b0;
    float b_6 = b6 / b0;
    
    float mu_1 = b_1;
    float mu_2 = b_2 - b_1 * b_1;
    float mu_3 = b_3 - 3.0 * b_1 * b_2 + 2.0 * b_1 * b_1 * b_1;
    float mu_4 = b_4 - 4.0 * b_1 * b_3 + 6.0 * b_1 * b_1 * b_2 - 3.0 * b_1 * b_1 * b_1 * b_1;
    
    mu_2 = max(mu_2, 1e-6);
    mu_4 = max(mu_4, 1e-6);
    
    float bias_correction = mu_3 / (2.0 * mu_2);
    
    float z_hat = mu_1 - bias_correction;
    
    float var = mu_2 - mu_3 * mu_3 / (4.0 * mu_2);
    var = max(var, 1e-6);
    
    // alpha and beta for the rational function
    float alpha = (mu_4 - mu_2 * mu_2) / (mu_4 - mu_2 * mu_2 + var * var);
    alpha = clamp(alpha, 0.0, 1.0);
    
    // rational transmittance function
    float z = z_hat;
    float sigma_sq = var;
    
    // chebyshev's inequality
    float transmittance = sigma_sq / (sigma_sq + z * z);
    
    transmittance = mix(exp(-z), transmittance, alpha);
    
    return clamp(transmittance, 0.0, 1.0);
}

void main() {
    vec4 color = texture(scene_color, TexCoord);
    vec4 bloom = texture(bright_color, TexCoord);

    float t = transmittance();
    vec3 resolved = color.rgb + bloom.rgb * t;
    
    resolved = resolved / (resolved + vec3(1.0));
    resolved = pow(resolved, vec3(1.0 / 2.2));
    
    FragColor = vec4(resolved, 1.0);
}
