#pragma once

#include "core/scene.h"
#include "asset/shader.h"
#include "util/obb.h"
#include "util/math.h"


#include <vector>

struct Debug_Line {
    vec3 start;
    vec3 end;
    vec3 color;
};

struct Debug_sphere {
    vec3 center;
    float radius;
    vec3 color;
};

struct Debug_cube {
    vec3 center;
    float size;
    vec3 color;
};

class Renderer_Debug {
public:
    Renderer_Debug() = default;
    ~Renderer_Debug() = default; 

    void init();
    void shutdown();

    void add_line(const vec3& start, const vec3& end, const vec3& color);
    //void add_axes(const vec3& position, const quat& orientation, float length = 1.0f);
    void add_bbox(const vec3& min, const vec3& max, const vec3& color);
    void draw_frustum(const vec3& cameraPos, const vec3& cameraDir, const vec3& cameraUp, const float& fov, const float& aspect, const float& near, const float& far, const vec3& color);
    
    void draw_bounding_sphere(Shader* debug_shader, const vec3& center, float radius, const vec3& color, const mat4& vp);
    void draw_scene_bounding_spheres(Shader* debug_shader, const Scene& scene, const mat4& view_proj);

    void render(Shader* debug_shader, const mat4& projection, const mat4& view, uint32_t num_lights);

private:
    std::vector<float> line_vertices;
    uint32_t line_vao, line_vbo;

    std::vector<vec3> sphere_vertices;
    uint32_t sphere_vao, sphere_vbo;
    int sphere_vertex_count;
    
    //void build_sphere_geometry();
    //void build_cube_geometry();
};
