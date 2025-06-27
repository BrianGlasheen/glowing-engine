#version 460

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// For a wave effect:
uniform float time; // pass elapsed time from the CPU

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

void main()
{
    // Simple wave along the y-axis
    float waveAmplitude = 0.01;   // how high the wave goes
    float waveFrequency = 5.0;    // how many waves across the mesh
    vec3 displacedPos = aPos;
    
    // Example: wave offset is tied to x-position + time
    displacedPos.y += sin(waveFrequency + time * 2.0) * waveAmplitude;

    // Standard transformations
    TexCoords = aTexCoords;
    
    // Transform normal correctly (assumes model has uniform scale)
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Compute world position and pass to fragment shader
    vec4 worldPos = model * vec4(displacedPos, 1.0);
    FragPos = worldPos.xyz;

    // Finally, output clip-space position
    gl_Position = projection * view * worldPos;
}