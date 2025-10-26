#version 460 core

// todo cleanup

in vec3 WorldPos;
in vec4 LightSpacePos;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out uint PickingId;

uniform vec3 gCameraWorldPos;
uniform float gGridSize = 300.0;
uniform float gGridMinPixelsBetweenCells = 2.0;
uniform float gGridCellSize = 1.0;

uniform vec4 gGridColorThin = vec4(vec3(0.25), 1.0);
uniform vec4 gGridColorThick = vec4(vec3(0.5), 1.0);

uniform vec4 gGridColorAxis = vec4(0.0, 0.0, 0.0, 1.0);
uniform vec4 gGridColorXAxis = vec4(1.0, 0.0, 0.0, 1.0);
uniform vec4 gGridColorZAxis = vec4(0.0, 0.0, 1.0, 1.0);
uniform vec4 gGridColorXAxisNeg = vec4(1.0, 0.0, 1.0, 1.0);
uniform vec4 gGridColorZAxisNeg = vec4(0.0, 1.0, 1.0, 1.0);

float log10(float x) {
    float f = log(x) / log(10.0);
    return f;
}

float satf(float x) {
    float f = clamp(x, 0.0, 1.0);
    return f;
}

vec2 satv(vec2 x) {
    vec2 v = clamp(x, vec2(0.0), vec2(1.0));
    return v;
}

float max2(vec2 v) {
    float f = max(v.x, v.y);
    return f;
}

void main() {
    vec2 dvx = vec2(dFdx(WorldPos.x), dFdy(WorldPos.x));
    vec2 dvy = vec2(dFdx(WorldPos.z), dFdy(WorldPos.z));
    float lx = length(dvx);
    float ly = length(dvy);
    vec2 dudv = vec2(lx, ly);
    float l = length(dudv);
    
    float LOD = max(0.0, log10(l * gGridMinPixelsBetweenCells / gGridCellSize) + 1.0);
    float GridCellSizeLod0 = gGridCellSize * pow(10.0, floor(LOD));
    float GridCellSizeLod1 = GridCellSizeLod0 * 10.0;
    float GridCellSizeLod2 = GridCellSizeLod1 * 10.0;
    
    dudv *= 4.0;
    
    vec2 mod_div_dudv = mod(WorldPos.xz, GridCellSizeLod0) / dudv;
    float Lod0a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));
    
    mod_div_dudv = mod(WorldPos.xz, GridCellSizeLod1) / dudv;
    float Lod1a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));
    
    mod_div_dudv = mod(WorldPos.xz, GridCellSizeLod2) / dudv;
    float Lod2a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));
    
    float axisThreshold = 0.5;
    vec2 axisDistance = abs(WorldPos.xz) / dudv;
    float axisLineX = 1.0 - satf(axisDistance.y / axisThreshold);
    float axisLineZ = 1.0 - satf(axisDistance.x / axisThreshold);
    float axisLine = max(axisLineX, axisLineZ);
    
    float LOD_fade = fract(LOD);
    vec4 Color;
    
    if (Lod2a > 0.0) {
        Color = gGridColorThick;
        Color.a *= Lod2a;
    } else {
        if (Lod1a > 0.0) {
            Color = mix(gGridColorThick, gGridColorThin, LOD_fade);
            Color.a *= Lod1a;
        } else {
            Color = gGridColorThin;
            Color.a *= (Lod0a * (1.0 - LOD_fade));
        }
    }

    if (axisLineX > 0.0) {
        vec4 xAxisColor = WorldPos.x >= 0.0 ? gGridColorXAxis : gGridColorXAxisNeg;
        Color = mix(Color, xAxisColor, axisLineX);
        Color.a = 1.0;
    }
    
    if (axisLineZ > 0.0) {
        vec4 zAxisColor = WorldPos.z >= 0.0 ? gGridColorZAxis : gGridColorZAxisNeg;
        Color = mix(Color, zAxisColor, axisLineZ);
        Color.a = 1.0;
    }
    
    float OpacityFalloff = (1.0 - satf(length(WorldPos.xz - gCameraWorldPos.xz) / gGridSize));
    Color.a *= OpacityFalloff;
    
    FragColor = Color;
    BrightColor = vec4(0.0f);
}