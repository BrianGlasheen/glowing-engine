#version 460 core

layout (quads, fractional_odd_spacing, cw) in;

// uniform sampler2D heightMap;  // the texture corresponding to our height map
layout(binding = 0) uniform sampler2D heightmap;

uniform mat4 vp;

const int NUM_CASCADES = 4;
in vec4 tcs_light_space_pos[][NUM_CASCADES];
in float tcs_view_space_z[];
out vec4 light_space_pos[NUM_CASCADES];
out float view_space_z;

// received from Tessellation Control Shader - all texture coordinates for the patch vertices
in vec2 TextureCoord[];

// send to Fragment Shader for coloring
out float Height;
out vec2 TexCoord;

void main() {
    // get patch coordinate
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    // retrieve control point texture coordinates
    vec2 t00 = TextureCoord[0];
    vec2 t01 = TextureCoord[1];
    vec2 t10 = TextureCoord[2];
    vec2 t11 = TextureCoord[3];

    // bilinearly interpolate texture coordinate across patch
    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    vec2 texCoord = (t1 - t0) * v + t0;

    // lookup texel at patch coordinate for height and scale + shift as desired
    Height = texture(heightmap, texCoord).r * 64.0 - 16.0;
    TexCoord = texCoord;

    // retrieve control point position coordinates
    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    // compute patch surface normal
    vec4 uVec = p01 - p00;
    vec4 vVec = p10 - p00;
    vec4 normal = normalize( vec4(cross(vVec.xyz, uVec.xyz), 0) );

    // bilinearly interpolate position coordinate across patch
    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 p = (p1 - p0) * v + p0;

    // displace point along normal
    p += normal * Height * 10;

    // output patch point position in clip space
    gl_Position = vp * p;
    
    float z0 = mix(tcs_view_space_z[0], tcs_view_space_z[1], u);
    float z1 = mix(tcs_view_space_z[3], tcs_view_space_z[2], u);
    view_space_z = mix(z0, z1, v);
    for (int i = 0; i < NUM_CASCADES; i++) {
        vec4 ls0 = mix(tcs_light_space_pos[0][i], tcs_light_space_pos[1][i], u);
        vec4 ls1 = mix(tcs_light_space_pos[3][i], tcs_light_space_pos[2][i], u);
        light_space_pos[i] = mix(ls0, ls1, v);
    }
}