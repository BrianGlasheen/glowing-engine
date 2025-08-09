#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNor;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 FragPos;  // position in world space --> cam too
out vec3 Normal;   // normal in world space  --> move to camera space prolly both

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));  // Transform to world space
    Normal = aNor; //vec3(transpose(inverse(model))) * aNor;  // Correct normal transform

    gl_Position = projection * view * vec4(FragPos, 1.0);
    TexCoord = aTexCoord;
}
