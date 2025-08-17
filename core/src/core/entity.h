#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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
        float max_ttl = 0.0f,
        bool is_animated = false
    );

    Entity(
        glm::vec3 position,
        glm::quat rotation,
        glm::vec3 scale,
        std::string model_name,
        bool physics_enabled,
        bool fade = false,
        float ttl = 0.0f,
        float max_ttl = 0.0f,
        bool is_animated = false
    );

    static Entity Animated_Entity(
        glm::vec3 position,
        glm::quat rotation,
        glm::vec3 scale,
        std::string model_name,
        bool physics_enabled,
        bool fade = false,
        float ttl = 0.0f,
        float max_ttl = 0.0f
    );

    static Entity Animated_Entity(
        glm::vec3 position,
        glm::quat rotation,
        glm::vec3 scale,
        model_handle model_id,
        bool physics_enabled,
        bool fade = false,
        float ttl = 0.0f,
        float max_ttl = 0.0f
    );
    
    ~Entity();

    glm::mat4 get_model_matrix() const;
    glm::vec3 get_physics_position() const;
    void check_moved();
    //Util::AABB get_aabb() const; // return model or physics, or mesh?

// private:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    model_handle model_id;
    bool physics_enabled;
    JPH::BodyID physics_id;

    bool is_dirty;
    glm::vec3 prev_pos;
    glm::quat prev_rot;

    bool fade;
    float ttl;
    float max_ttl;

    bool is_animated;
    // maybe just keep this pointer so we dont have to Model_Manager::get_animated_model(model_id) to
    // to something to our animation data, idk maybe will rethink, maybe Animator::set_animation_state(model_id, animation_state);
    // to store less data per entity
    Animated_Model* animated_model; 

    //Util::AABB aabb;

};
