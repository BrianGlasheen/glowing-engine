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
uniform uint max_bone;
uniform uint bone_offset;

uniform uint draw_mode;

vec3 perpendicular(vec3 v) {
    vec3 a = abs(v);
    if (a.x < a.y && a.x < a.z) {
        return vec3(0.0, -v.z, v.y);
    } else if (a.y < a.z) {
        return vec3(-v.z, 0.0, v.x);
    } else {
        return vec3(-v.y, v.x, 0.0);
    }
}

void main() {
    if (draw_mode == 0) {
        vec4 pos = mvp * vec4(bones[base_bone + gl_VertexID][3].xyz, 1.0);
        gl_Position = pos;
        gl_PointSize = max(2.0, 50.0 / pos.w);
    }
    else if (draw_mode == 1) {
        uint line_vertex = gl_VertexID;
        uint bone_idx = line_vertex / 2;
        uint vertex_in_line = line_vertex % 2;
        
        if ((bone_idx + base_bone) >= max_bone) {
            gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
            return;
        }
        
        uint current_bone = base_bone + bone_idx;
        uint parent_idx = gpu_bones[current_bone - bone_offset].parent_bone;
        
        if (parent_idx == 0xFFFFFFFF) {
            gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
            return;
        }
        
        vec3 position;
        if (vertex_in_line == 0) {
            position = vec3(bones[current_bone][3].xyz);
        } else {
            position = vec3(bones[parent_idx][3].xyz);
        }
        
        gl_Position = mvp * vec4(position, 1.0);
    }
    else if (draw_mode == 2) {
        uint vertices_per_bone = 24;
        uint bone_idx = gl_VertexID / 24;
        uint vertex_in_bone = gl_VertexID % 24;
        
        if ((bone_idx + base_bone) >= max_bone) {
            gl_Position = vec4(0.0);
            return;
        }
        
        uint current_bone = base_bone + bone_idx;
        uint parent_idx = gpu_bones[current_bone - bone_offset].parent_bone;
        
        if (parent_idx == 0xFFFFFFFF) {
            gl_Position = vec4(0.0);
            return;
        }
        
        vec3 bone_start = bones[current_bone][3].xyz;
        vec3 bone_end = bones[parent_idx][3].xyz;
        vec3 bone_dir = bone_end - bone_start;
        float bone_length = length(bone_dir);
        
        if (bone_length < 0.0001) {
            gl_Position = vec4(0.0);
            return;
        }
        
        bone_dir = normalize(bone_dir);
        
        vec3 perp1 = normalize(perpendicular(bone_dir));
        vec3 perp2 = normalize(cross(bone_dir, perp1));
        
        float base_scale = bone_length * 0.2;
        float mid_point = bone_length * 0.66;
        
        vec3 mid_pos = bone_start + bone_dir * mid_point;
        
        vec3 base_points[4];
        base_points[0] = mid_pos + perp1 * base_scale;
        base_points[1] = mid_pos + perp2 * base_scale;
        base_points[2] = mid_pos - perp1 * base_scale;
        base_points[3] = mid_pos - perp2 * base_scale;
        
        vec3 position;
        
        if (vertex_in_bone < 8) {
            uint line = vertex_in_bone / 2;
            uint vtx = vertex_in_bone % 2;
            position = vtx == 0 ? base_points[line] : base_points[(line + 1) % 4];
        }
        else if (vertex_in_bone < 16) {
            uint line = (vertex_in_bone - 8) / 2;
            uint vtx = (vertex_in_bone - 8) % 2;
            position = vtx == 0 ? base_points[line] : bone_start;
        }
        else {
            uint line = (vertex_in_bone - 16) / 2;
            uint vtx = (vertex_in_bone - 16) % 2;
            position = vtx == 0 ? base_points[line] : bone_end;
        }
        
        gl_Position = mvp * vec4(position, 1.0);
    }
}