#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "asset/model.h"
#include "asset/model_manager.h"
#include "asset/shader.h"
#include "util/aabb.h"
#include "physics.h"

//struct entity_creation {
//    
//};

class Entity {
public:
    Entity(
        glm::vec3 position,
        glm::quat rotation,
        glm::vec3 scale,
        model_handle model_id,
        bool physics_enabled,
        bool fade = false,
        float ttl = 0.0f,
        float max_ttl = 0.0f
    );

    Entity(
        glm::vec3 position,
        glm::quat rotation,
        glm::vec3 scale,
        std::string model_name,
        bool physics_enabled,
        bool fade = false,
        float ttl = 0.0f,
        float max_ttl = 0.0f
    );
    
    ~Entity();

    glm::mat4 get_model_matrix() const;
    glm::vec3 get_physics_position() const;
    //Util::AABB get_aabb() const; // return model or physics, or mesh?

    void draw(const Shader* shader, bool shadow_pass = false) const;

// private:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    model_handle model_id;
    bool physics_enabled;
    JPH::BodyID physics_id;
    
    bool fade;
    float ttl;
    float max_ttl;

    //Util::AABB aabb;

};
