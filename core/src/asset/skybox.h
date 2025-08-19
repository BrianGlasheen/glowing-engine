#pragma once

#include "glow.h"

#include <vector>
#include <string>
#include <iostream>

#include <stb_image.h>

class Skybox {
public:
    Skybox(const std::string& skybox_name);
    
    void bind(uint32_t slot) const;
    void draw() const;

    uint32_t num_mips;

private:
    uint32_t vao, vbo, texture_id;

    void setup_cube();
    void load_cubemap(const std::vector<std::string>& faces);

};
