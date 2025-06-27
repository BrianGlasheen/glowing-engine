#version 460

layout (location = 0) in vec3 aPos;       // Position
layout (location = 1) in vec3 aNormal;    // (Optional) Normal
layout (location = 2) in vec2 aTexCoords; // (Optional) UVs, if you want to sample a texture

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

// Outputs to fragment shader
out vec2 TexCoords; 
out vec3 WorldPos;   // We'll pass the final world-space position, if you want further lighting
out vec3 Normal;     // Pass transformed normal, if you want lighting

void main()
{
    // Apply a simple sine-wave displacement on the mesh
    // You can tweak amplitude/frequency/phase for different wave styles
    float amplitude = 0.04;      // wave height
    float frequency = 4.0;       // how many waves across the mesh
    float speed = 2.0;           // how fast they move

    // Modify copy of the original position
    vec3 displacedPos = aPos;
    
    // Example wave: shift y by sin(x + time)
    displacedPos.y += sin((aPos.x + time * speed) * frequency) * amplitude;

    // (Optional) Another wave dimension: shift x by cos(z + time)
    // displacedPos.x += cos((aPos.z + time * speed) * frequency) * amplitude * 0.5;

    // Transform normal if you want realistic shading. For a trivial effect, we can skip it.
    // But here's the standard normal transform:
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    Normal = normalize(normalMatrix * aNormal);

    // Convert local -> world
    vec4 worldPosition = model * vec4(displacedPos, 1.0);
    WorldPos = worldPosition.xyz;

    // Pass UVs if your mesh uses them
    TexCoords = aTexCoords;

    // Finally, project to clip space
    gl_Position = projection * view * worldPosition;
}
