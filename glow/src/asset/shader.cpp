#include "shader.h"

#include "glow_config.h"
#include "core/opengl.h"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string.h>

bool Shader::init(const char* vertex_path, const char* fragment_path) {
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertex_code, fragment_code;
    std::ifstream vertex_file, fragment_file;
    // ensure ifstream objects can throw exceptions:
    vertex_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragment_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // open files
        vertex_file.open(vertex_path);
        fragment_file.open(fragment_path);
        std::stringstream vertex_stream, fragment_stream;
        // read file's buffer contents into streams
        vertex_stream << vertex_file.rdbuf();
        fragment_stream << fragment_file.rdbuf();
        // close file handlers
        vertex_file.close();
        fragment_file.close();
        // convert stream into string
        vertex_code = vertex_stream.str();
        fragment_code = fragment_stream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }

    size_t offset = strlen("#define BINDLESS") + 1; // number

    size_t v_bindless_pos = vertex_code.find("#define BINDLESS");
    if (v_bindless_pos != std::string::npos) {
#if BINDLESS
        vertex_code[v_bindless_pos + offset] = '1';
#else
        vertex_code[v_bindless_pos + offset] = '0';
#endif
    }
    size_t f_bindless_pos = fragment_code.find("#define BINDLESS");
    if (f_bindless_pos != std::string::npos) {
#if BINDLESS
        fragment_code[f_bindless_pos + offset] = '1';
#else
        fragment_code[f_bindless_pos + offset] = '0';
#endif
    }

    const char* vShaderCode = vertex_code.c_str();
    const char* fShaderCode = fragment_code.c_str();
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

bool Shader::init(const char* vertexPath, const char* fragmentPath,
    const char* tessControlPath, const char* tessEvalPath) {
    // 1. retrieve the shader source code from filePaths
    std::string vertex_code, fragment_code, tess_control_code, tess_eval_code;
    std::ifstream vertex_file, fragment_file, tc_file, te_file;

    // ensure ifstream objects can throw exceptions:
    vertex_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragment_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    tc_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    te_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        // open files
        vertex_file.open(vertexPath);
        fragment_file.open(fragmentPath);
        tc_file.open(tessControlPath);
        te_file.open(tessEvalPath);

        std::stringstream vertex_stream, fragment_stream, tc_stream, te_stream;

        // read file's buffer contents into streams
        vertex_stream << vertex_file.rdbuf();
        fragment_stream << fragment_file.rdbuf();
        tc_stream << tc_file.rdbuf();
        te_stream << te_file.rdbuf();

        // close file handlers
        vertex_file.close();
        fragment_file.close();
        tc_file.close();
        te_file.close();

        // convert stream into string
        vertex_code = vertex_stream.str();
        fragment_code = fragment_stream.str();
        tess_control_code = tc_stream.str();
        tess_eval_code = te_stream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        return false;
    }

    const char* vShaderCode = vertex_code.c_str();
    const char* fShaderCode = fragment_code.c_str();
    const char* tcShaderCode = tess_control_code.c_str();
    const char* teShaderCode = tess_eval_code.c_str();

    // 2. compile shaders
    uint32_t vertex, fragment, tessControl, tessEval;

    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    check_compile_errors(vertex, "VERTEX");

    // tessellation control shader
    tessControl = glCreateShader(GL_TESS_CONTROL_SHADER);
    glShaderSource(tessControl, 1, &tcShaderCode, NULL);
    glCompileShader(tessControl);
    check_compile_errors(tessControl, "TESS_CONTROL");

    // tessellation evaluation shader
    tessEval = glCreateShader(GL_TESS_EVALUATION_SHADER);
    glShaderSource(tessEval, 1, &teShaderCode, NULL);
    glCompileShader(tessEval);
    check_compile_errors(tessEval, "TESS_EVAL");

    // fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    check_compile_errors(fragment, "FRAGMENT");

    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, tessControl);
    glAttachShader(ID, tessEval);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    check_compile_errors(ID, "PROGRAM");

    // delete the shaders as they're linked into our program now
    glDeleteShader(vertex);
    glDeleteShader(tessControl);
    glDeleteShader(tessEval);
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
void Shader::set_uint64(const std::string& name, uint64_t value) {
    glUniformHandleui64ARB(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::set_float(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::set_float_array(const std::string& name, const float* floats, uint32_t count) const {
    glUniform1fv(glGetUniformLocation(ID, name.c_str()), count, floats);
}
void Shader::set_vec2(const std::string& name, const vec2& value) const {
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::set_vec2(const std::string& name, float x, float y) const {
    glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}
void Shader::set_vec3(const std::string& name, const vec3& value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::set_vec3(const std::string& name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}
void Shader::set_vec4(const std::string& name, const vec4& value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::set_vec4(const std::string& name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}
void Shader::set_uvec2(const std::string& name, const uvec2& value) const {
    glUniform2uiv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::set_uvec2(const std::string& name, unsigned int x, unsigned int y) const {
    glUniform2ui(glGetUniformLocation(ID, name.c_str()), x, y);
}
void Shader::set_uvec3(const std::string& name, const uvec3& value) const {
    glUniform3uiv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
void Shader::set_uvec3(const std::string& name, unsigned int x, unsigned int y, unsigned int z) const {
    glUniform3ui(glGetUniformLocation(ID, name.c_str()), x, y, z);
}
void Shader::set_mat2(const std::string& name, const mat2& mat) const {
    glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::set_mat3(const std::string& name, const mat3& mat) const {
    glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::set_mat4(const std::string& name, const mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
void Shader::set_mat4_array(const std::string& name, const mat4* matrices, uint32_t count) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), count, GL_FALSE, value_ptr(matrices[0]));
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
