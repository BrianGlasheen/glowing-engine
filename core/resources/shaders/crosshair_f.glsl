#version 460 core

uniform float thickness;
uniform float gap;
uniform float height;
uniform float width;
uniform float opacity;
uniform vec3 color;
uniform vec2 screen_size;

out vec4 FragColor;

float crosshair(vec2 screenPos, vec2 center) {
    vec2 p = abs(screenPos - center);
    
    float horizontal = step(p.y, thickness * 0.5) *          // Within thickness vertically
                      step(gap * 0.5, p.x) *                // Past the gap horizontally  
                      step(p.x, gap * 0.5 + width * 0.5);  // Before the end
    
    float vertical = step(p.x, thickness * 0.5) *            // Within thickness horizontally
                    step(gap * 0.5, p.y) *                  // Past the gap vertically
                    step(p.y, gap * 0.5 + height * 0.5);   // Before the end
    
    return max(horizontal, vertical);
}

void main() {
    vec2 screenPos = gl_FragCoord.xy;
    vec2 center = screen_size * 0.5;
    
    float crosshairMask = crosshair(screenPos, center);
   
    FragColor = vec4(color, opacity * crosshairMask);
}