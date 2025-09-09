#version 460

layout(std430, binding = 0) readonly buffer Bones {
    mat4 bones[];
};

uniform mat4 mvp;
uniform uint base_bone;

void main() {
    uint bone_index = base_bone + gl_VertexID;
    mat4 boneTransform = bones[bone_index];
    vec3 bonePosition = vec3(boneTransform[3]);
    
    gl_Position = mvp * vec4(bonePosition, 1.0);
    gl_PointSize = 10.0;
}