#pragma once

#include <string>

#include "model.h"
#include "shader.h"
#include "util/aabb.h"

typedef size_t model_handle;

namespace Model_Manager {
    void init(std::string path);
    void cleanup();

    model_handle load_model(const std::string& model_name, int gltf = 1);
    Model& get_model_by_name(const std::string& model_name);
    Model& get_model(const model_handle model_id);

    //Model& get_model_by_name_load(const std::string& model_name);
    void draw(const Shader* shader, const model_handle model_id, bool shadow_pass = false);

    size_t get_model_count();
    std::string get_name(const model_handle& model_id);
    Util::AABB get_aabb(const model_handle& model_id);
}
