#version 460 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main() {    
    
    //vec4 sky = texture(skybox, TexCoords);
    //vec3 color = vec3(sky.x, sky.y, sky.z);

    //color = color / (color + vec3(1.0));
    //color = pow(color, vec3(1.0/2.2));

    FragColor = texture(skybox, TexCoords);
    //FragColor = vec4(vec3(color), sky.w);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}