#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <chrono>

#include "shader.h"

namespace fs = std::filesystem;

typedef size_t shader_handle;

struct Shader_Data {
    Shader shader;
    std::string name;
    std::string vertex_name;
    std::string fragment_name;
    fs::file_time_type vertex_last_modified;
    fs::file_time_type fragment_last_modified;
};

namespace Shader_Manager {
    void init(std::string base_path);
    void cleanup();

    shader_handle load_from_paths(const std::string& name, const std::string& vertex_name, const std::string& fragment_name);
    shader_handle load_from_name(const std::string& shader_name);

    Shader* get_shader(shader_handle handle);
    Shader* get_shader_by_name(const std::string& name);

    bool reload(shader_handle handle);
    void hot_reload_all();

    size_t get_shader_count();
    //std::string get_name(shader_handle handle);

    bool loaded_already(const std::string& vertex_name, const std::string& fragment_name, shader_handle& existing_handle);
    fs::file_time_type get_file_time(const std::string& name);
}
