#pragma once

#include "core/opengl.h"
#include "util/math.h"

#include <string>

class Compute_Shader {
public:
    uint32_t ID;

    Compute_Shader() = default;
    //~Compute_Shader();

    bool init(const char* path);

    void use() const;
    void dispatch(uint32_t numGroupsX, uint32_t numGroupsY = 1, uint32_t numGroupsZ = 1) const;
    void wait(GLbitfield barrier) const;
    void dispatch_and_wait(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ, GLbitfield barrier) const;
    
    void set_bool(const std::string& name, bool value) const;
    void set_int(const std::string& name, int value) const;
    void set_uint(const std::string& name, uint32_t value) const;
    void set_float(const std::string& name, float value) const;
    void set_float_array(const std::string& name, const float* floats, uint32_t count) const;
    void set_vec2(const std::string& name, const vec2& value) const;
    void set_vec2(const std::string& name, float x, float y) const;
    void set_vec3(const std::string& name, const vec3& value) const;
    void set_vec3(const std::string& name, float x, float y, float z) const;
    void set_vec4(const std::string& name, const vec4& value) const;
    void set_vec4(const std::string& name, float x, float y, float z, float w) const;
    void set_ivec2(const std::string& name, const ivec2& value) const;
    void set_ivec3(const std::string& name, const ivec3& value) const;
    void set_ivec4(const std::string& name, const ivec4& value) const;
    void set_uvec2(const std::string& name, const uvec2& value) const;
    void set_uvec2(const std::string& name, unsigned int x, unsigned int y) const;
    void set_uvec3(const std::string& name, const uvec3& value) const;
    void set_uvec3(const std::string& name, unsigned int x, unsigned int y, unsigned int z) const;
    void set_mat2(const std::string& name, const mat2& mat) const;
    void set_mat3(const std::string& name, const mat3& mat) const;
    void set_mat4(const std::string& name, const mat4& mat) const;
    
    void bind_image_texture(uint32_t unit, uint32_t texture, int32_t level, bool layered, int32_t layer, uint32_t access, uint32_t format) const;
    
    static ivec3 get_max_work_group_count();
    static ivec3 get_max_work_group_size();
    static int32_t get_max_work_group_invocations();

private:
    void check_compile_errors(uint32_t shader, const std::string& type) const;
    bool compile_shader(const std::string& computeSource);
};