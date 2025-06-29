#pragma once

#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "asset/font.h"
#include "shader.h"

class Text {
public:

    Text() = default;

    Text(Font& font, const std::string& text, float x, float y, float scale, const glm::vec3& textColor);

    ~Text();

    void load(Font& font, const std::string& text, float _x, float _y, float _scale, const glm::vec3& textColor);
    void update_text(const std::string& text);
    void draw(Shader* shader, const glm::mat4& projection) const;

// private:
    void compute_buffers(const std::string& text);
    void upload_buffers();

    struct Vertex {
        glm::vec2 position;
        glm::vec2 texCoord;
    };

    uint32_t VAO, VBO, EBO;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    texture_handle atlas_texture_id;
    glm::vec3 color;
    size_t index_count;

    Font* used_font;
    float x;
    float y;
    float scale;
};
