#version 460

in vec3 FragPos; // World space position from vertex shader
uniform vec3 point_light_position;
//uniform float point_light_far_plane;

out float distance;

void main() {
    distance = length(FragPos - point_light_position);
    
    //distance = distance / point_light_far_plane;
    //gl_FragDepth = distance;
}