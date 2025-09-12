#version 460

struct GPU_Bone {
    mat4 inverse_bind;
    uint parent_bone;
    uint padding[3];
};

layout(std430, binding = 0) readonly buffer Bones {
    mat4 bones[];
};

layout(std430, binding = 1) buffer GPU_Bones {
    GPU_Bone gpu_bones[];
};

uniform mat4 mvp;
uniform uint base_bone;
uniform uint bone_count;

uniform uint mode;

void main() {

    if (mode == 0) {
        uint bone_index = base_bone + gl_VertexID;
        mat4 boneTransform = bones[bone_index];
        vec3 bonePosition = vec3(boneTransform[3]);
        
        vec4 pos = mvp * vec4(bonePosition, 1.0);
        gl_Position = pos;
        
        float distance = pos.w;
        gl_PointSize = max(2.0, 500.0 / distance);
    }
    else {
        uint line_vertex = gl_VertexID;
        uint bone_idx = line_vertex / 2;
        uint vertex_in_line = line_vertex % 2;
        
        if (bone_idx >= bone_count) {
            gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
            return;
        }
        
        uint current_bone = base_bone + bone_idx;
        uint parent_idx = gpu_bones[current_bone].parent_bone;
        
        if (parent_idx == 0xFFFFFFFF) {
            gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
            return;
        }
        
        vec3 position;
        if (vertex_in_line == 0) {
            position = vec3(bones[current_bone][3]);
        } else {
            position = vec3(bones[parent_idx][3]);
        }
        
        gl_Position = mvp * vec4(position, 1.0);
    }
}