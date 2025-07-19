#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <string>

typedef size_t texture_handle;

namespace Texture_Manager {
    void init();
    void cleanup();

    texture_handle load_from_path(const std::string& file_path);
    texture_handle load_msdf(const std::string& file_path);
    texture_handle create_render_texture(int width, int height, bool hdr = false);
    texture_handle create_bloom_texture(int width, int height);

    void bind(texture_handle texture_id, uint32_t texture_unit = 0);
    uint32_t get_ogl_id(texture_handle texture_id);
    size_t get_texture_count();
    std::string get_name(texture_handle texture_id);

    bool bindless_loaded_already(const std::string& new_path, size_t& existing_idx);
    uint64_t load_bindless_from_path(const std::string& file_path);

}
#endif
