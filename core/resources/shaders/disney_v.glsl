#version 460

// Input vertex attributes
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

// Output to fragment shader
out vec3 frag_position;
out vec3 frag_normal;
out vec2 frag_tex_coord;
out vec3 frag_tangent;
out vec3 frag_bitangent;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normal_matrix; // Inverse transpose of model matrix for normal transformation

void main() {
    // Calculate vertex position in world space
    frag_position = vec3(model * vec4(position, 1.0));
    
    // Transform normal, tangent and bitangent to world space
    frag_normal = normalize(normal_matrix * normal);
    frag_tangent = normalize(normal_matrix * tangent);
    frag_bitangent = normalize(normal_matrix * bitangent);
    
    // Pass texture coordinates to fragment shader
    frag_tex_coord = tex_coord;
    
    // Calculate final position
    gl_Position = projection * view * model * vec4(position, 1.0);
}