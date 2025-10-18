#version 460 core

in float Height;
in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out uint PickingId;

layout(binding = 1) uniform sampler2D heightmap_color;

uniform bool lines;

layout(binding = 8) uniform sampler2DArray directional_shadow_map;
const int num_cascades = 4;
in vec4 light_space_pos[num_cascades];
in float view_space_z;
uniform float cascade_distances[num_cascades];

int GetCascadeIndex(float depth) {
    for (int i = 0; i < num_cascades; i++) {
        if (depth < cascade_distances[i]) {
            return i;
        }
    }
    return - 1;
}

float DirectionalShadowCalculation() {
    int cascadeIndex = GetCascadeIndex(view_space_z);

    if (cascadeIndex == -1) return 0.0;

    vec4 LightSpacePos = light_space_pos[cascadeIndex];
    vec3 ProjCoords = LightSpacePos.xyz / LightSpacePos.w;

    vec2 UVCoords;
    UVCoords.x = 0.5 * ProjCoords.x + 0.5;
    UVCoords.y = 0.5 * ProjCoords.y + 0.5;

    float z = ProjCoords.z;
    float depth = texture(directional_shadow_map, vec3(UVCoords, cascadeIndex)).r;
    float bias = 0.001;

    if (z < depth - bias)
        return 1.0;
    else
        return 0.0;
}

void main() {
	float h = (Height + 16)/64.0f;
	if (lines)
		FragColor = vec4(1.0, 0, 0, 1.0);
	else {
		// vec3 color = texture(heightmap_color, TexCoord).rgb;
	    // color = color / (color + vec3(1.0));
		// color = pow(color, vec3(2.2));
		float shadow = DirectionalShadowCalculation();
		if (shadow > .5)
			FragColor = vec4(vec3(0.0), 1.0);
		else
			FragColor = vec4(vec3(0.2), 1.0);
	}
	//FragColor = vec4(1.0, 0, 0, 1.0);
	//BrightColor = vec4(1.0, 0, 0, 1.0);
}