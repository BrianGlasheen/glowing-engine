#pragma once
#include "core/opengl.h"

class SSBO {
public:
    SSBO() = default;
    ~SSBO();

    void init();
    void bind(uint32_t binding) const;
    void set_data(uint32_t size, const void* data, GLenum usage);
    void update_sub_data(uint32_t offset, uint32_t size, const void* data);
    uint32_t get_id() const;

private:
    uint32_t id;
};
