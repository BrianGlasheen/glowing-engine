#pragma once

#include "asset/texture_manager.h"

#include <cstdint>
#include <string>

class Terrain {
public:
    Terrain() = default;
    ~Terrain() = default;

    void init(float width, float height, uint32_t num_patches_x, uint32_t num_patches_z, const std::string& heightmap);
    void cleanup();

//private:
    uint32_t vao, vbo;

    uint32_t vertex_count;
    texture_handle heightmap;
    texture_handle heightmap_texture;
};
