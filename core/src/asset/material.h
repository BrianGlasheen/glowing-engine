#pragma once

#include <glm/glm.hpp>
#include <dearimgui/imgui.h>

#include "texture_manager.h"
#include "shader.h"

class Shader;

class Material {
public:
    Material(texture_handle albedo_map = 0, 
             texture_handle normal_map = 0, 
             texture_handle metallic_roughness_map = 0, 
             texture_handle occlusion_map = 0, 
             texture_handle emissive_map = 0) 
        : albedo_map(albedo_map), 
          normal_map(normal_map), 
          metallic_roughness_map(metallic_roughness_map),
          occlusion_map(occlusion_map),
          emissive_map(emissive_map)
    {
        has_albedo = albedo_map != 0;
        has_normal = normal_map != 0;
        has_metallic_roughness = metallic_roughness_map != 0;
        has_occlusion = occlusion_map != 0;
        has_emissive = emissive_map != 0;

        //printf("a: %d, n: %d\n", has_albedo, has_normal);
    }

    ~Material() {
        // free all textures
    }

    //glm::vec3 base_color;
    
    // todo pack?
    bool has_albedo;
    bool has_normal;
    bool has_metallic_roughness;
    bool has_occlusion;
    bool has_emissive;
    texture_handle albedo_map;
    texture_handle normal_map;
    texture_handle metallic_roughness_map;
    texture_handle occlusion_map;
    texture_handle emissive_map;

    //glm::vec2 texture_scale = glm::vec2(1.0f, 1.0f);
    //glm::vec2 texture_offset = glm::vec2(0.0f, 0.0f);

    void apply(Shader& shader) const {
        // apply base color texture
        // apply normal map texture
    }

private:
};
