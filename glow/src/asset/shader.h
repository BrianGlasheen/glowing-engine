#pragma once

#include <string>

#include "glm/glm.hpp"

// todo add compute, maybe own class
class Shader {
public:
    uint32_t ID;

    Shader() = default;
    // todo destructor

    bool init(const char* vertex_path, const char* fragment_path);
    bool init(const char* vertex_path, const char* fragment_path, const char* tess_control_path, const char* tess_eval_path);
    void use() const;

    void set_bool(const std::string& name, bool value) const;
    void set_int(const std::string& name, int value) const;
    void set_uint(const std::string& name, unsigned int value) const;
    void set_uint64(const std::string& name, uint64_t handle);
    void set_float(const std::string& name, float value) const;
    void set_float_array(const std::string& name, const float* floats, uint32_t count) const;
    void set_vec2(const std::string& name, const glm::vec2& value) const;
    void set_vec2(const std::string& name, float x, float y) const;
    void set_vec3(const std::string& name, const glm::vec3& value) const;
    void set_vec3(const std::string& name, float x, float y, float z) const;
    void set_vec4(const std::string& name, const glm::vec4& value) const;
    void set_vec4(const std::string& name, float x, float y, float z, float w) const;
    void set_uvec2(const std::string& name, const glm::uvec2& value) const;
    void set_uvec2(const std::string& name, unsigned int x, unsigned int y) const;
    void set_uvec3(const std::string& name, const glm::uvec3& value) const;
    void set_uvec3(const std::string& name, unsigned int x, unsigned int y, unsigned int z) const;
    void set_mat2(const std::string& name, const glm::mat2& mat) const;
    void set_mat3(const std::string& name, const glm::mat3& mat) const;
    void set_mat4(const std::string& name, const glm::mat4& mat) const;
    void set_mat4_array(const std::string& name, const glm::mat4* matrices, uint32_t count) const;

private:
    void check_compile_errors(uint32_t shader, std::string type);
};
