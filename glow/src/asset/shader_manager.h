#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <chrono>

#include "shader.h"
#include "compute_shader.h"

namespace fs = std::filesystem;

typedef size_t shader_handle;

struct Shader_Data {
    Shader shader;
    std::string vertex_name;
    std::string fragment_name;
    fs::file_time_type vertex_last_modified;
    fs::file_time_type fragment_last_modified;
};

struct Compute_Data {
    Compute_Shader shader;
    fs::file_time_type last_modified;
};

namespace Shader_Manager {
    void init(std::string base_path);
    void cleanup();

    void load_from_paths(const std::string& name, const std::string& vertex_name, const std::string& fragment_name);
    void load_from_name(const std::string& shader_name);
    void load_compute(const std::string& shader_name);

    Shader* get_shader(const std::string& name);
    Compute_Shader* get_compute(const std::string& name);
    //Shader* get_shader_by_name(const std::string& name);

    bool reload(const std::string& name);
    bool reload_compute(const std::string& name);
    void hot_reload_all();

    size_t get_shader_count();
    //std::string get_name(shader_handle handle);

    fs::file_time_type get_file_time(const std::string& name);
}
