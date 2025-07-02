#include "core/SSBO.h"

SSBO::SSBO() {
    glGenBuffers(1, &id);
}

SSBO::~SSBO() {
    glDeleteBuffers(1, &id);
}

void SSBO::bind(uint32_t binding) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
}

void SSBO::set_data(uint32_t size, const void* data, GLenum usage) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
}

void SSBO::update_sub_data(uint32_t offset, uint32_t size, const void* data) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
}

uint32_t SSBO::get_id() const {
    return id;
}
