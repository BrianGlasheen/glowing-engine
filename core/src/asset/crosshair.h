#pragma once

#include <glm/glm.hpp>
#include <dearimgui/imgui.h>

#include "asset/shader.h"

class Crosshair {
public:
    Crosshair(float thickness,
              float gap,
              float height,
              float width,
              float opacity,
              glm::vec3 color)
        : thickness(thickness), 
          gap(gap), 
          height(height), 
          width(width), 
          opacity(opacity), 
          color(color)
    {
        glGenVertexArrays(1, &no_buffer);
    }

    ~Crosshair() {
        glDeleteVertexArrays(1, &no_buffer);
    }

    void draw(const Shader* shader, const int& screen_width, const int& screen_height) const 
    {
        shader->set_float("thickness", thickness);
        shader->set_float("gap", gap);
        shader->set_float("height", height);
        shader->set_float("width", width);
        shader->set_float("opacity", opacity);
        shader->set_vec3("color", color);
        shader->set_vec2("screen_size", glm::vec2(screen_width, screen_height));

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindVertexArray(no_buffer);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    void gui() 
    {
        ImGui::Begin("Crosshair");

        ImGui::SliderFloat("Thickness", &thickness, 0.5f, 10.0f);
        ImGui::SliderFloat("Gap", &gap, 0.0f, 50.0f);
        ImGui::SliderFloat("Height", &height, 1.0f, 100.0f);
        ImGui::SliderFloat("Width", &width, 1.0f, 100.0f);
        ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f);

        ImGui::ColorEdit3("Color", &color.x);

        ImGui::End();
    }

//private:
    GLuint no_buffer;
    float thickness, gap, height, width, opacity;
    glm::vec3 color;
};
