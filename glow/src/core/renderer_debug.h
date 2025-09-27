#pragma once

#include "core/scene.h"
#include "asset/shader.h"
#include "util/obb.h"

#include "glm/glm.hpp"
#include <glm/gtc/quaternion.hpp>

#include <vector>

struct Debug_Line {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 color;
};

struct Debug_sphere {
    glm::vec3 center;
    float radius;
    glm::vec3 color;
};

struct Debug_cube {
    glm::vec3 center;
    float size;
    glm::vec3 color;
};

class Renderer_Debug {
public:
    Renderer_Debug() = default;
    ~Renderer_Debug() = default; 

    void init();
    void shutdown();

    void add_line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
    //void add_axes(const glm::vec3& position, const glm::quat& orientation, float length = 1.0f);
    void add_bbox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color);
    void draw_frustum(const glm::vec3& cameraPos, const glm::vec3& cameraDir, const glm::vec3& cameraUp, const float& fov, const float& aspect, const float& near, const float& far, const glm::vec3& color);
    
    void draw_bounding_sphere(Shader* debug_shader, const glm::vec3& center, float radius, const glm::vec3& color, const glm::mat4& vp);
    void draw_scene_bounding_spheres(Shader* debug_shader, const Scene& scene, const glm::mat4& view_proj);

    void render(Shader* debug_shader, const glm::mat4& projection, const glm::mat4& view, uint32_t num_lights);

private:
    std::vector<float> line_vertices;
    uint32_t line_vao, line_vbo;

    std::vector<glm::vec3> sphere_vertices;
    uint32_t sphere_vao, sphere_vbo;
    int sphere_vertex_count;
    
    //void build_sphere_geometry();
    //void build_cube_geometry();
};
