#version 460 core

// #extension GL_ARB_gpu_shader_int64: enable

layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 Tangent;
layout (location = 2) in vec4 Bitangent;
layout (location = 3) in vec2 aTexCoord;

uniform mat4 mvp;
uniform float scale;

void main() {
    // #if BINDLESS
        // Per_Object_Data obj_data = per_object_data[gl_BaseInstance];
    // #else
        // Per_Object_Data obj_data = per_object_data[instance_id];
    // #endif
    vec3 norm = normalize(vec3(aPos.w, Tangent.w, Bitangent.w));
    gl_Position = mvp * vec4(vec3(aPos) + norm * scale, 1.0);
}
