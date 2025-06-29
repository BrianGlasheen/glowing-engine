#pragma once

#include <vector>
#include <string>
#include <iostream>

#include <stb_image.h>

class Skybox {
public:
    Skybox(const std::string& skybox_name);
    
    void bind() const;

    void draw() const;

private:
    uint32_t vao, vbo, texture_id;

    void setup_cube();

    void load_cubemap(const std::vector<std::string>& faces);

};
