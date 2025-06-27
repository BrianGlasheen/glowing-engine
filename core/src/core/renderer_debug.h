#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
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

class Renderer_Debug {
public:
    Renderer_Debug() {}
    ~Renderer_Debug() {}

    void init();
    void add_line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
    void add_sphere(const glm::vec3& center, float radius, const glm::vec3& color);
    void add_axes(const glm::vec3& position, const glm::quat& orientation, float length = 1.0f);
    void add_bbox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color);
    void add_obb(const Util::OBB obb, const glm::vec3& color);
    void draw_frustum(const glm::vec3 cameraPos, const glm::vec3 cameraDir, const glm::vec3 cameraUp, float fov, float aspect, float near, float far);
    void render(Shader* debug_shader, const glm::mat4& projection, const glm::mat4& view);

private:
    std::vector<Debug_Line> lines;
    GLuint line_vao, line_vbo;

    std::vector<Debug_sphere> spheres;
    GLuint sphere_vbo = 0, sphere_vao = 0, sphere_ebo = 0;
    int sphere_index_count = 0;
    
    void build_sphere_geometry();
};
