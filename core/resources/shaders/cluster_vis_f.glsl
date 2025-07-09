#version 460 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

// Uniforms matching your compute shader
uniform float zNear;
uniform float zFar;
uniform mat4 projection;
uniform uvec3 gridSize;
uniform uvec2 screenDimensions;

// Function to convert depth buffer value to linear depth
float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Convert to NDC
    return (2.0 * zNear) / (zFar + zNear - z * (zFar - zNear));
}

// Generate a distinct color for each of 24 Z slices
vec3 indexToColor(uint index) {
    // Pre-defined colors for 24 slices with good contrast
    vec3 colors[24] = vec3[](
        vec3(1.0, 0.0, 0.0),     // 0: Red
        vec3(0.0, 1.0, 0.0),     // 1: Green
        vec3(0.0, 0.0, 1.0),     // 2: Blue
        vec3(1.0, 1.0, 0.0),     // 3: Yellow
        vec3(1.0, 0.0, 1.0),     // 4: Magenta
        vec3(0.0, 1.0, 1.0),     // 5: Cyan
        vec3(1.0, 0.5, 0.0),     // 6: Orange
        vec3(0.5, 1.0, 0.0),     // 7: Yellow-Green
        vec3(0.0, 0.5, 1.0),     // 8: Light Blue
        vec3(1.0, 0.0, 0.5),     // 9: Red-Pink
        vec3(0.5, 0.0, 1.0),     // 10: Blue-Magenta
        vec3(0.0, 1.0, 0.5),     // 11: Blue-Green
        vec3(0.8, 0.8, 0.8),     // 12: Light Gray
        vec3(0.5, 0.5, 0.5),     // 13: Gray
        vec3(0.3, 0.3, 0.3),     // 14: Dark Gray
        vec3(1.0, 1.0, 1.0),     // 15: White
        vec3(0.8, 0.4, 0.2),     // 16: Brown
        vec3(1.0, 0.8, 0.8),     // 17: Pink
        vec3(0.8, 1.0, 0.8),     // 18: Light Green
        vec3(0.8, 0.8, 1.0),     // 19: Light Purple
        vec3(0.4, 0.8, 0.8),     // 20: Teal
        vec3(0.8, 0.4, 0.8),     // 21: Purple
        vec3(0.4, 0.4, 0.8),     // 22: Dark Blue
        vec3(0.8, 0.6, 0.4)      // 23: Tan
    );
    
    return colors[index % 24];
}

void main() {
    // Get the fragment's position in screen space
    vec2 screenCoord = gl_FragCoord.xy;
    
    // Get the fragment's depth from the depth buffer
    float fragDepth = gl_FragCoord.z;
    
    // DEBUG STEP 1: Check raw depth values
    // Uncomment to see raw depth buffer values (should be 0.0 to 1.0)
    //FragColor = vec4(vec3(fragDepth), 1.0); return;
    
    // Convert to linear depth in view space
    float linearDepth = linearizeDepth(fragDepth);
    
    // DEBUG STEP 2: Check if linearization is working
    // Scale by a reasonable factor based on your scene
    //FragColor = vec4(vec3(linearDepth / 1.0), 1.0); return;
    
    // Calculate which Z slice this fragment belongs to
    // Using the same exponential distribution as your compute shader
    float depthSlice = log(linearDepth / zNear) / log(zFar / zNear);
    
    // DEBUG STEP 3: Check depth slice calculation
    //FragColor = vec4(vec3(depthSlice), 1.0); return;
    
    uint zSlice = uint(depthSlice * 100.f);
    
    // DEBUG STEP 4: Check if zSlice is reasonable
    //FragColor = vec4(vec3(float(zSlice) / 1.0), 1.0); return;
    
    // Clamp to valid range
    zSlice = min(zSlice, gridSize.z - 1);
    
    // DEBUG STEP 5: Final zSlice check
    // FragColor = vec4(vec3(float(zSlice) / 99.0), 1.0); return;
    
    // Calculate tile coordinates (same as compute shader)
    vec2 tileSize = vec2(screenDimensions) / vec2(gridSize.xy);
    uvec2 tileID = uvec2(screenCoord / tileSize);
    
    // Calculate the cluster index
    uint clusterIndex = tileID.x + (tileID.y * gridSize.x) + (zSlice * gridSize.x * gridSize.y);
    
    // Generate color based on Z slice (0-99 for your 100 slices)
    vec3 color = indexToColor(zSlice);
    
    // Optional: Add some brightness variation based on depth
    float brightness = 0.7 + 0.3 * (1.0 - clamp(depthSlice, 0.0, 1.0));
    color *= brightness;
    
    // Optional: Add grid lines to show tile boundaries
    vec2 tilePos = mod(screenCoord, tileSize);
    if (tilePos.x < 2.0 || tilePos.y < 2.0 || 
        tilePos.x > tileSize.x - 2.0 || tilePos.y > tileSize.y - 2.0) {
        color = mix(color, vec3(1.0), 0.3); // White grid lines
    }
    
    FragColor = vec4(color, 1.0);
    BrightColor = vec4(0.0); // No bloom for debug visualization
}