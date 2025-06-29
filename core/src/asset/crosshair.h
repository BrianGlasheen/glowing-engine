#pragma once

#include <glm/glm.hpp>

#include "asset/shader.h"

class Crosshair {
public:
    Crosshair(float thickness, float gap, float height, float width, float opacity, glm::vec3 color);
    ~Crosshair();

    void draw(const Shader* shader, const int& screen_width, const int& screen_height) const;

    void gui();

private:
    uint32_t no_buffer;
    float thickness, gap, height, width, opacity;
    glm::vec3 color;
};
