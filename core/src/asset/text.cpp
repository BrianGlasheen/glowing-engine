#include "text.h"

// #include <glad/glad.h>
#include "core/opengl.h"

#include "asset/texture_manager.h"

Text::Text(Font& font,
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

Text::~Text()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Text::load(Font& font, const std::string& text, float _x, float _y, float _scale, const glm::vec3& textColor) {
    color = textColor;
    atlas_texture_id = font.atlas_texture_id;
    used_font = &font;
    x = _x;
    y = _y;
    scale = _scale;

    compute_buffers(text);
    upload_buffers();
}

void Text::update_text(const std::string& text) {
    compute_buffers(text);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());
}

void Text::draw(Shader* shader, const glm::mat4& projection) const
{
    shader->use();
    shader->set_mat4("projection", projection);
    shader->set_vec3("textColor", color);

    Texture_Manager::bind(atlas_texture_id);
    shader->set_int("msdfTexture", 0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Text::compute_buffers(const std::string& text) {
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

    index_count = indices.size();
}

void Text::upload_buffers() {
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
