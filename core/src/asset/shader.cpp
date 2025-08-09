#include "shader.h"

#include "glow.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <string.h>

// #include <glad/glad.h>
#include "core/opengl.h"

bool Shader::init(const char* vertexPath, const char* fragmentPath) {
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }

    size_t offset = strlen("#define BINDLESS") + 1; // number

    size_t v_bindless_pos = vertexCode.find("#define BINDLESS");
    if (v_bindless_pos != std::string::npos) {
        #if BINDLESS
            vertexCode[v_bindless_pos + offset] = '1';
        #else
            vertexCode[v_bindless_pos + offset] = '0';
        #endif
    }
    size_t f_bindless_pos = fragmentCode.find("#define BINDLESS");
    if (f_bindless_pos != std::string::npos) {
        #if BINDLESS
            fragmentCode[f_bindless_pos + offset] = '1';
        #else
            fragmentCode[f_bindless_pos + offset] = '0';
        #endif
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    // 2. compile shaders
    uint32_t vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    check_compile_errors(vertex, "VERTEX");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    check_compile_errors(fragment, "FRAGMENT");
    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    check_compile_errors(ID, "PROGRAM");
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return true;
}

void Shader::use() const {
    glUseProgram(ID);
}

void Shader::set_bool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
void Shader::set_int(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::set_uint(const std::string& name, unsigned int value) const {
    glUniform1ui(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::set_float(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::set_vec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::set_vec2(const std::string& name, float x, float y) const {
    glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}
void Shader::set_vec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::set_vec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}
void Shader::set_vec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::set_vec4(const std::string& name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}
void Shader::set_uvec2(const std::string& name, const glm::uvec2& value) const {
    glUniform2uiv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::set_uvec2(const std::string& name, unsigned int x, unsigned int y) const {
    glUniform2ui(glGetUniformLocation(ID, name.c_str()), x, y);
}
void Shader::set_uvec3(const std::string& name, const glm::uvec3& value) const {
    glUniform3uiv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::set_uvec3(const std::string& name, unsigned int x, unsigned int y, unsigned int z) const {
    glUniform3ui(glGetUniformLocation(ID, name.c_str()), x, y, z);
}
void Shader::set_mat2(const std::string& name, const glm::mat2& mat) const {
    glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::set_mat3(const std::string& name, const glm::mat3& mat) const {
    glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::set_mat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

// utility function for checking shader compilation/linking errors.
void Shader::check_compile_errors(uint32_t shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}
