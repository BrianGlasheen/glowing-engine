#include "shader_manager.h"

#include "core/opengl.h"

#include <iostream>

#include <cassert>
#include <unordered_map>

namespace Shader_Manager {

    static std::unordered_map<std::string, Shader_Data> shaders;
    static std::unordered_map<std::string, Compute_Data> compute_shaders;
    static std::string base_path;

    void init(const std::string path) {
        std::cout << "[SHADER] Shader manager initialized" << std::endl;
        base_path = path;
    }

    void cleanup() {
        //for (auto& shader_data : shaders) {
        //    if (shader_data.shader.ID != 0) {
        //        glDeleteProgram(shader_data.shader.ID);
        //    }
        //}
        //shaders.clear();
    }

    void load_from_paths(const std::string& name, const std::string& vertex_name, const std::string& fragment_name) {
        auto it = shaders.find(name);
        if (it != shaders.end()) {
            printf("shader already loaded with this name %s\n", name.c_str());
            assert(false);
        }

        Shader_Data shader_data;
        shader_data.vertex_name = vertex_name;
        shader_data.fragment_name = fragment_name;

        std::string vertex_path = base_path + vertex_name;
        std::string fragment_path = base_path + fragment_name;

        shader_data.vertex_last_modified = get_file_time(vertex_path);
        shader_data.fragment_last_modified = get_file_time(fragment_path);

        bool success = shader_data.shader.init(vertex_path.c_str(), fragment_path.c_str());

        if (success) {
            shaders[name] = shader_data;
            printf("[SHADER] loaded: %s & %s\n", vertex_path.c_str(), fragment_path.c_str());
        }
        else {
            printf("[SHADER] failed to load: %s & %s\n", vertex_path.c_str(), fragment_path.c_str());
            assert(false);
        }
    }

    void load_from_name(const std::string& shader_name) {
        std::string vert = shader_name + "_v.glsl";
        std::string frag = shader_name + "_f.glsl";
        load_from_paths(shader_name, vert, frag);
    }

    void load_compute(const std::string& name) {
        auto it = compute_shaders.find(name);
        if (it != compute_shaders.end()) {
            printf("shader already loaded with this name %s\n", name.c_str());
            assert(false);
        }

        const std::string path = base_path + "compute/" + name + ".comp";

        Compute_Data shader_data;
        shader_data.last_modified = get_file_time(path);
        bool success = shader_data.shader.init(path.c_str());

        if (success) {
            compute_shaders[name] = shader_data;
            printf("[SHADER] loaded: %s\n", path.c_str());
        }
        else {
            printf("[SHADER] failed to load: %s\n", path.c_str());
            assert(false);
        }
    }

    Shader* get_shader(const std::string& name) {
        auto it = shaders.find(name);

        if (it != shaders.end())
            return &it->second.shader;

        printf("shader %s doesn't exist\n", name.c_str());
        assert(false);
    }

    Compute_Shader* get_compute(const std::string& name) {
        auto it = compute_shaders.find(name);

        if (it != compute_shaders.end())
            return &it->second.shader;

        printf("compute shader %s doesn't exist\n", name.c_str());
        assert(false);
    }

    //Shader* get_shader_by_name(const std::string& name) {    }

    bool reload(const std::string& name) {
        //if (handle >= shaders.size()) return false;
        Shader_Data& shader_data = shaders[name];

        std::string vertex_path = base_path + shader_data.vertex_name;
        std::string fragment_path = base_path + shader_data.fragment_name;

        fs::file_time_type current_vertex_time = get_file_time(vertex_path);
        fs::file_time_type current_fragment_time = get_file_time(fragment_path);

        bool vertex_changed = current_vertex_time > shader_data.vertex_last_modified;
        bool fragment_changed = current_fragment_time > shader_data.fragment_last_modified;

        if (vertex_changed || fragment_changed) {
            std::cout << "[SHADER] Detected changes in: " << shader_data.vertex_name << " + " << shader_data.fragment_name<< std::endl;

            Shader new_shader;

            bool success = new_shader.init(vertex_path.c_str(), fragment_path.c_str());

            if (success) {
                if (shader_data.shader.ID != 0) {
                    glDeleteProgram(shader_data.shader.ID);
                }

                shader_data.shader = new_shader;
                shader_data.vertex_last_modified = current_vertex_time;
                shader_data.fragment_last_modified = current_fragment_time;

                printf("[SHADER] reloaded: %s & %s", shader_data.vertex_name.c_str(), shader_data.fragment_name.c_str());
                return true;
            }
            else {
                printf("[SHADER] failed to reload:: %s & %s", shader_data.vertex_name.c_str(), shader_data.fragment_name.c_str());
                assert(false);
            }
        }

        return false;
    }

    bool reload_compute(const std::string& name) {
        auto it = compute_shaders.find(name);
        if (it == compute_shaders.end()) {
            return false;
        }

        Compute_Data& shader_data = it->second;
        const std::string path = base_path + "compute/" + name + ".comp";
        printf("%s\n", path.c_str());

        fs::file_time_type current_time = get_file_time(path);

        if (current_time > shader_data.last_modified) {
            std::cout << "[SHADER] Detected changes in compute shader: " << name << std::endl;

            Compute_Shader new_shader;
            bool success = new_shader.init(path.c_str());

            if (success) {
                if (shader_data.shader.ID != 0) {
                    glDeleteProgram(shader_data.shader.ID);
                }

                shader_data.shader = new_shader;
                shader_data.last_modified = current_time;

                printf("[SHADER] Successfully reloaded compute shader: %s", name.c_str());
                return true;
            }
            else {
                printf("[SHADER] Failed to reload compute shader: %s", name.c_str());
            }
        }

        return false;
    }

    void hot_reload_all() {
        printf("[SHADER] Checking all shaders for changes...\n");
        bool any_reloaded = false;

        for (auto it = shaders.begin(); it != shaders.end(); it++) {
            if (reload(it->first)) {
                any_reloaded = true;
            }
        }

        for (auto it = compute_shaders.begin(); it != compute_shaders.end(); it++) {
            if (reload_compute(it->first)) {
                any_reloaded = true;
            }
        }

        if (!any_reloaded) {
            std::cout << "[SHADER] No shader changes detected" << std::endl;
        }
    }

    size_t get_shader_count() {
        return shaders.size();
    }

    fs::file_time_type get_file_time(const std::string& name) {
        try {
            return fs::last_write_time(name);
        }
        catch (const fs::filesystem_error& e) {
            std::cout << "[SHADER] Warning: Could not get file time for " << name << ": " << e.what() << std::endl;
            return fs::file_time_type{};
        }
    }
}