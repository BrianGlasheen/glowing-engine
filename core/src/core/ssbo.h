#pragma once
#include "core/opengl.h"

class SSBO {
public:
    SSBO();
    ~SSBO();

    void bind(uint32_t binding);
    void set_data(uint32_t size, const void* data, GLenum usage);
    void update_sub_data(uint32_t offset, uint32_t size, const void* data);
    uint32_t get_id() const;

private:
    uint32_t id;
};
