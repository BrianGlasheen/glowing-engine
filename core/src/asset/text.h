#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

#include "asset/font.h"
#include "shader.h"
#include "asset/texture_manager.h"

class Text {
    //private:
public:

    struct Vertex {
        glm::vec2 position;
        glm::vec2 texCoord;
    };

    GLuint VAO, VBO, EBO;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    texture_handle atlas_texture_id;
    glm::vec3 color;
    size_t indexCount;

    Font* used_font;
    float x;
    float y;
    float scale;

    void compute_buffers(const std::string& text) {
        Font font = *used_font;

        vertices.clear();
        indices.clear();

        float currentX = x;
        float currentY = y;
        uint32_t vertexOffset = 0;

        for (char c : text) {

            if (c == 32) { // space
                currentX += 0.5f * scale;
                continue;
            }

            auto it = font.characters.find(c);
            if (it == font.characters.end()) {
                printf("didnt find %hu\n", c);
                continue;
            }

            const Font::Glyph& glyph = it->second;

            float xpos = currentX + glyph.planeLeft * scale;
            float ypos = currentY + glyph.planeBottom * scale;
            float w = (glyph.planeRight - glyph.planeLeft) * scale;
            float h = (glyph.planeTop - glyph.planeBottom) * scale;

            vertices.push_back({ {xpos, ypos}, {glyph.atlasLeft, glyph.atlasBottom} });
            vertices.push_back({ {xpos + w, ypos}, {glyph.atlasRight, glyph.atlasBottom} });
            vertices.push_back({ {xpos + w, ypos + h}, {glyph.atlasRight, glyph.atlasTop} });
            vertices.push_back({ {xpos, ypos + h}, {glyph.atlasLeft, glyph.atlasTop} });

            indices.push_back(vertexOffset + 0);
            indices.push_back(vertexOffset + 1);
            indices.push_back(vertexOffset + 2);
            indices.push_back(vertexOffset + 2);
            indices.push_back(vertexOffset + 3);
            indices.push_back(vertexOffset + 0);

            vertexOffset += 4;
            currentX += glyph.advance * scale;
        }

        indexCount = indices.size();
    }

    void updateText(const std::string& text) {
        compute_buffers(text);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());
    }

    void upload_buffers() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // vertex data
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        // index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        // texture coordinate attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    Text() = default;

    Text(Font& font,
         const std::string& text,
         float x, float y,
         float scale,
         const glm::vec3& textColor)
    : color(textColor),
      atlas_texture_id(font.atlas_texture_id),
      used_font(&font),
      x(x), y(y), scale(scale)
    {
        compute_buffers(text);
        upload_buffers();
    }

    void load(Font& font, const std::string& text, float _x, float _y, float _scale, const glm::vec3& textColor) {
        color = textColor;
        atlas_texture_id = font.atlas_texture_id;
        used_font = &font;
        x = _x;
        y = _y;
        scale = _scale;

        compute_buffers(text);
        upload_buffers();
    }

    ~Text() 
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    void draw(Shader* shader, const glm::mat4& projection) const 
    {
        shader->use();
        shader->set_mat4("projection", projection);
        shader->set_vec3("textColor", color);

        Texture_Manager::bind(atlas_texture_id);
        shader->set_int("msdfTexture", 0);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

};
