#pragma once

#include <string>
#include <vector>
#include <cstdint>

typedef size_t texture_handle;

namespace Texture_Manager {
    void init();
    void cleanup();

    bool loaded_already(const std::string& new_path, size_t& existing_idx);
    uint64_t load(const std::string& file_path);
    uint64_t load_dds(const std::string& file_path);    
    uint64_t load_non_dds(const std::string& file_path);

    void resize(const texture_handle handle, int width, int height, int mips = 1);

    texture_handle load_msdf(const std::string& file_path);
    texture_handle create_depth_texture(int width, int height);
    texture_handle create_picking_texture(int width, int height);
    texture_handle create_render_texture(int width, int height, bool hdr = false);
    texture_handle create_bloom_texture(int width, int height);
    texture_handle create_ssao_texture(int width, int height);
    texture_handle create_noise_texture(const std::vector<float>& data, int width, int height);
    texture_handle create_moment_texture(int width, int height);
    
    //texture_handle create_3d_texture(int width, int height, int layers);
    texture_handle create_2d_array_texture(int width, int height, int layers);
    texture_handle load_heightmap(const std::string& name);

    void bind(texture_handle texture_id, uint32_t texture_unit = 0);
    void bind_array(texture_handle texture_id, uint32_t texture_unit);
    uint32_t get_ogl_id(texture_handle texture_id);
    size_t get_texture_count();
    std::string get_name(texture_handle texture_id);


    uint32_t get_ogl_format(uint32_t fourCC);
}
