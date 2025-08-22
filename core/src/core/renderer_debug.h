#pragma once

#include <vector>

#include "glm/glm.hpp"
#include <glm/gtc/quaternion.hpp>

#include "asset/shader.h"
#include "util/obb.h"

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
    ~Renderer_Debug() {}

    void init();
    void add_line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
    //void add_axes(const glm::vec3& position, const glm::quat& orientation, float length = 1.0f);
    void add_bbox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color);
    void draw_frustum(const glm::vec3& cameraPos, const glm::vec3& cameraDir, const glm::vec3& cameraUp, const float& fov, const float& aspect, const float& near, const float& far, const glm::vec3& color);

    void render(Shader* debug_shader, const glm::mat4& projection, const glm::mat4& view, uint32_t num_lights);

private:
    std::vector<float> line_vertices;
    uint32_t line_vao, line_vbo;
    
    //void build_sphere_geometry();
    //void build_cube_geometry();
};
