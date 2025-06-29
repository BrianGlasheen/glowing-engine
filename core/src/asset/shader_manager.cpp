#include "shader_manager.h"

// #include <glad/glad.h>
#include "core/opengl.h"

#include <iostream>
#include <cassert>

namespace Shader_Manager {

    static std::vector<Shader_Data> shaders;
    static std::string base_path;

    void init(const std::string path) 
    {
        std::cout << "[SHADER] Shader manager initialized" << std::endl;
        base_path = path;
    }

    void cleanup() 
    {
        std::cout << "[SHADER] Cleaning up " << shaders.size() << " shaders" << std::endl;
        for (auto& shader_data : shaders) {
            if (shader_data.shader.ID != 0) {
                glDeleteProgram(shader_data.shader.ID);
            }
        }
        shaders.clear();
    }

    shader_handle load_from_paths(const std::string& name, const std::string& vertex_name, const std::string& fragment_name) 
    {
        shader_handle existing_handle;
        if (loaded_already(vertex_name, fragment_name, existing_handle)) {
            std::cout << "[SHADER] Already loaded: " << vertex_name << " + " << fragment_name << std::endl;
            return existing_handle;
        }

        Shader_Data shader_data;
        shader_data.name = name;
        shader_data.vertex_name = vertex_name;
        shader_data.fragment_name = fragment_name;
        shader_data.vertex_last_modified = get_file_time(vertex_name);
        shader_data.fragment_last_modified = get_file_time(fragment_name);

        std::string vertex_path = base_path + vertex_name;
        std::string fragment_path = base_path + fragment_name;
        bool success = shader_data.shader.init(vertex_path.c_str(), fragment_path.c_str());

        if (success) {
            shader_handle handle = shaders.size();
            shaders.push_back(shader_data);

            std::cout << "[SHADER] Loaded: " << vertex_path << " + " << fragment_path << std::endl;
            return handle;
        }
        else {
            std::cout << "[SHADER] Failed to load: " << vertex_path << " + " << fragment_path << std::endl;
            assert(false);
        }
    }

    shader_handle load_from_name(const std::string& shader_name) 
    {
        //"deferred_v.glsl"
        std::string vert = shader_name + "_v.glsl";
        std::string frag = shader_name + "_f.glsl";
        return load_from_paths(shader_name, vert, frag);
    }

    Shader* get_shader(shader_handle handle) 
    {
        if (handle < shaders.size()) {
            return &shaders[handle].shader;
        }
        assert(false);
    }

    Shader* get_shader_by_name(const std::string& name) 
    {
        for (size_t i = 0; i < shaders.size(); i++) {
            if (shaders[i].name == name)
                return &shaders[i].shader;
        }
        assert(false);
    }

    bool reload(shader_handle handle) 
    {
        //if (handle >= shaders.size()) return false;
        Shader_Data& shader_data = shaders[handle];



        fs::file_time_type current_vertex_time = get_file_time(shader_data.vertex_name);
        fs::file_time_type current_fragment_time = get_file_time(shader_data.fragment_name);

        bool vertex_changed = current_vertex_time > shader_data.vertex_last_modified;
        bool fragment_changed = current_fragment_time > shader_data.fragment_last_modified;

        if (vertex_changed || fragment_changed) {
            std::cout << "[SHADER] Detected changes in: " << shader_data.vertex_name << " + " << shader_data.fragment_name<< std::endl;

            Shader new_shader;
            std::string vertex_path = base_path + shader_data.vertex_name;
            std::string fragment_path = base_path + shader_data.fragment_name;
            bool success = new_shader.init(vertex_path.c_str(), fragment_path.c_str());

            if (success) {
                if (shader_data.shader.ID != 0) {
                    glDeleteProgram(shader_data.shader.ID);
                }

                shader_data.shader = new_shader;
                shader_data.vertex_last_modified = current_vertex_time;
                shader_data.fragment_last_modified = current_fragment_time;

                std::cout << "[SHADER] Successfully reloaded: " << shader_data.vertex_name << " + " << shader_data.fragment_name << std::endl;
                return true;
            }
            else {
                std::cout << "[SHADER] Failed to reload: " << shader_data.vertex_name << " + " << shader_data.fragment_name << std::endl;
                assert(false);
            }
        }

        return false;
    }

    void hot_reload_all() 
    {
        std::cout << "[SHADER] Checking all shaders for changes..." << std::endl;
        bool any_reloaded = false;

        for (size_t i = 0; i < shaders.size(); i++) {
            if (reload(i)) {
                any_reloaded = true;
            }
        }

        if (!any_reloaded) {
            std::cout << "[SHADER] No shader changes detected" << std::endl;
        }
    }

    size_t get_shader_count() 
    {
        return shaders.size();
    }

    bool loaded_already(const std::string& vertex_name, const std::string& fragment_name, shader_handle& existing_handle) 
    {
        for (size_t i = 0; i < shaders.size(); i++) {
            if (vertex_name == shaders[i].vertex_name && fragment_name == shaders[i].fragment_name) {
                existing_handle = i;
                return true;
            }
        }
        return false;
    }

    fs::file_time_type get_file_time(const std::string& name) 
    {
        try {
            return fs::last_write_time(base_path + name);
        }
        catch (const fs::filesystem_error& e) {
            std::cout << "[SHADER] Warning: Could not get file time for " << name << ": " << e.what() << std::endl;
            return fs::file_time_type{};
        }
    }
}