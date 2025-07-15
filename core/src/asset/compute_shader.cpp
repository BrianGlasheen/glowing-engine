#include "compute_shader.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include <core/opengl.h>

Compute_Shader::~Compute_Shader() {
    if (ID != 0) {
        glDeleteProgram(ID);
    }
}

bool Compute_Shader::init(const char* path) {
    std::string computeCode;
    std::ifstream cShaderFile;

    cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        cShaderFile.open(path);
        std::stringstream cShaderStream;
        cShaderStream << cShaderFile.rdbuf();
        cShaderFile.close();
        computeCode = cShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "ERROR::COMPUTE_SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        return false;
    }

    bool status = compile_shader(computeCode);
    if (status)
        printf("[COMPUTE] LOADED %s\n", path);
    else
        printf("[COMPUTE] FAILED LOADING %s\n", path);
    
    return status;
}

bool Compute_Shader::init_from_source(const std::string& src) {
    return compile_shader(src);
}

bool Compute_Shader::compile_shader(const std::string& src) {
    const char* cShaderCode = src.c_str();

    uint32_t compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, NULL);
    glCompileShader(compute);
    check_compile_errors(compute, "COMPUTE");

    ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    check_compile_errors(ID, "PROGRAM");

    glDeleteShader(compute);
    return true;
}

void Compute_Shader::use() const {
    glUseProgram(ID);
}

void Compute_Shader::dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) const {
    glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
}

void Compute_Shader::wait() const {
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void Compute_Shader::dispatch_and_wait(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) const {
    dispatch(numGroupsX, numGroupsY, numGroupsZ);
    wait();
}

void Compute_Shader::set_bool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Compute_Shader::set_int(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Compute_Shader::set_uint(const std::string& name, uint32_t value) const {
    glUniform1ui(glGetUniformLocation(ID, name.c_str()), value);
}

void Compute_Shader::set_float(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Compute_Shader::set_vec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Compute_Shader::set_vec2(const std::string& name, float x, float y) const {
    glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}

void Compute_Shader::set_vec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Compute_Shader::set_vec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Compute_Shader::set_vec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Compute_Shader::set_vec4(const std::string& name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}

void Compute_Shader::set_ivec2(const std::string& name, const glm::ivec2& value) const {
    glUniform2iv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Compute_Shader::set_ivec3(const std::string& name, const glm::ivec3& value) const {
    glUniform3iv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Compute_Shader::set_ivec4(const std::string& name, const glm::ivec4& value) const {
    glUniform4iv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Compute_Shader::set_mat2(const std::string& name, const glm::mat2& mat) const {
    glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Compute_Shader::set_mat3(const std::string& name, const glm::mat3& mat) const {
    glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Compute_Shader::set_mat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Compute_Shader::bind_image_texture(uint32_t unit, uint32_t texture, int32_t level, bool layered, int32_t layer, uint32_t access, uint32_t format) const {
    glBindImageTexture(unit, texture, level, layered ? GL_TRUE : GL_FALSE, layer, access, format);
}

glm::ivec3 Compute_Shader::get_max_work_group_count() {
    glm::ivec3 maxCount;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxCount.x);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxCount.y);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxCount.z);
    return maxCount;
}

glm::ivec3 Compute_Shader::get_max_work_group_size() {
    glm::ivec3 maxSize;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxSize.x);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxSize.y);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxSize.z);
    return maxSize;
}

int32_t Compute_Shader::get_max_work_group_invocations() {
    int32_t maxInvocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvocations);
    return maxInvocations;
}

void Compute_Shader::check_compile_errors(uint32_t shader, const std::string& type) const {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::COMPUTE_SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::COMPUTE_PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}



void Compute_Shader::set_uvec2(const std::string& name, const glm::uvec2& value) const {
    glUniform2uiv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Compute_Shader::set_uvec2(const std::string& name, unsigned int x, unsigned int y) const {
    glUniform2ui(glGetUniformLocation(ID, name.c_str()), x, y);
}

// For uvec3 (3 unsigned integers)
void Compute_Shader::set_uvec3(const std::string& name, const glm::uvec3& value) const {
    glUniform3uiv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Compute_Shader::set_uvec3(const std::string& name, unsigned int x, unsigned int y, unsigned int z) const {
    glUniform3ui(glGetUniformLocation(ID, name.c_str()), x, y, z);
}
