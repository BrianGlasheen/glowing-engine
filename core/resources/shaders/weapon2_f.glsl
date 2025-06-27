#version 460

out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// Uniforms
uniform samplerCube skybox;
uniform float time;

//----------------------------------------
// Helper: Convert RGB -> HSV
//----------------------------------------
vec3 rgb2hsv(vec3 c)
{
    float maxC = max(c.r, max(c.g, c.b));
    float minC = min(c.r, min(c.g, c.b));
    float delta = maxC - minC;

    float h = 0.0;
    if(delta < 1e-6) {
        h = 0.0; // undefined
    } else if(maxC == c.r) {
        h = mod(((c.g - c.b) / delta), 6.0);
    } else if(maxC == c.g) {
        h = ((c.b - c.r) / delta) + 2.0;
    } else {
        h = ((c.r - c.g) / delta) + 4.0;
    }
    h /= 6.0; // normalize to [0,1]

    float s = (maxC < 1e-6) ? 0.0 : (delta / maxC);
    float v = maxC;

    return vec3(h, s, v);
}

//----------------------------------------
// Helper: Convert HSV -> RGB
//----------------------------------------
vec3 hsv2rgb(vec3 c)
{
    float h = c.x * 6.0;
    float s = c.y;
    float v = c.z;
    
    float i = floor(h);
    float f = h - i;
    float p = v * (1.0 - s);
    float q = v * (1.0 - s * f);
    float t = v * (1.0 - s * (1.0 - f));

    if(i == 0.0) return vec3(v, t, p);
    else if(i == 1.0) return vec3(q, v, p);
    else if(i == 2.0) return vec3(p, v, t);
    else if(i == 3.0) return vec3(p, q, v);
    else if(i == 4.0) return vec3(t, p, v);
    else return vec3(v, p, q);
}

void main()
{
    // Choose any base color you want (e.g. a watery blue)
    // FragColor = vec4(0.0, 0.6, 1.0, 1.0);
    // return;

    // If you have a texture, you can multiply it in or blend it
    FragColor = texture(skybox, normalize(WorldPos));
    return;
    /*
    if(useTexture) {
        vec3 texColor = texture(diffuseTexture, TexCoords).rgb;
        baseColor = mix(baseColor, texColor, 0.5); 
        // The 0.5 is just an example mix factor
    }
    
    // Convert to HSV
    vec3 hsvColor = rgb2hsv(baseColor);

    // Shift the hue over time for a rainbow effect
    // e.g. cycle the hue once every ~5 seconds
    float hueSpeed = 0.2; // cycles per second (1.0 = 1 full cycle/sec)
    hsvColor.x = fract(hsvColor.x + time * hueSpeed);

    // Optionally, we can also modulate saturation or value for pulsation
    // hsvColor.y = 0.8 + 0.2 * sin(time * 2.0);
    // hsvColor.z = 0.8 + 0.2 * cos(time * 2.5);

    // Convert back to RGB
    vec3 finalColor = hsv2rgb(hsvColor);

    // Add some "glow" factor (pseudo-emissive) by just boosting brightness
    float glowStrength = 1.5;  // tweak to taste
    finalColor *= glowStrength;

    // If you have real lighting to apply, you'd do
    // a normal dot light direction, etc. For now, we skip or do minimal.
    
    // Output
    FragColor = vec4(finalColor, 1.0);
    */
}
