#pragma once

#include <glow.h>

#include <vector>

#include <glm/glm.hpp>

#include "shader.h"
#include "material.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};
 
class Mesh {
    public:
        std::vector<Vertex>       vertices;
        std::vector<uint32_t> indices;
        Material material;

        Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, Material material);
        
        void draw(const Shader* shader, bool shadow_pass) const;
        void update_vertex_buffer();

    private:
        GLuint VAO, VBO, EBO;

        void setup_mesh();
};
