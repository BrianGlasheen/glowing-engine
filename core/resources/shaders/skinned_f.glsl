#version 460 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Tangentout;
in vec3 Bitangentout;
in flat uvec4 BonesOut;
in vec4 BoneWeightsOut;

uniform uint bone = 0;

void main() { 
    bool found = false;

    //FragColor = vec4(BoneWeightsOut[0], 0.0, 0.0, 1.0);
    //return ;
    vec3 col;

    for (int i = 0; i < 4; i++) {
        if (BonesOut[i] == bone) {
            if (BoneWeightsOut[i] >= 0.7)
                col = vec3(1.0, 0.0, 0.0) * BoneWeightsOut[i];
            else if (BoneWeightsOut[i] >= 0.4 && BoneWeightsOut[i] < 0.7)
                col = vec3(0.0, 1.0, 0.0) * BoneWeightsOut[i];
            else if (BoneWeightsOut[i] >= 0.1)
                col = vec3(1.0, 1.0, 0.0) * BoneWeightsOut[i];
            
            found = true;
            break;
        }
    }
    
    if (!found)
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    else
        FragColor = vec4(col, 1.0);
}