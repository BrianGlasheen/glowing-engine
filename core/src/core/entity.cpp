#include "entity.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "core/physics.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

//struct Entity_CreateInfo {
//    glm::vec3 position;
//    glm::quat rotation;
//    glm::vec3 scale;
//    model_handle model_id;
//    bool physics_enabled;
//    bool fade;
//    float ttl;
//    float max_ttl;
//};
//
//Entity::Entity(const Entity_CreateInfo& CI)
//        : position(CI.position),
//        rotation(CI.rotation),
//        scale(CI.scale),
//        model_id(CI.model_id),
//        physics_enabled(CI.physics_enabled),
//        fade(CI.fade), ttl(CI.ttl), max_ttl(CI.max_ttl)
//{
//    Util::AABB aabb = Model_Manager::get_aabb(model_id);
//    if (physics_enabled)
//        physics_id = Physics::add_box(position, (aabb.max - aabb.min) * scale, false);
//}

Entity::Entity(glm::vec3 position,
               glm::quat rotation,
               glm::vec3 scale,
               model_handle model_id,
               bool physics_enabled,
               bool fade, float ttl, float max_ttl,
               bool is_animated) 
        : position(position),
          rotation(rotation),
          scale(scale),
          model_id(model_id),
          physics_enabled(physics_enabled),
          fade(fade), ttl(ttl), max_ttl(max_ttl), 
          is_dirty(true), prev_pos(position), prev_rot(rotation),
          is_animated(is_animated)
{
    if (is_animated)
        animated_model = &Model_Manager::get_animated_model(model_id);

    Util::AABB aabb = Model_Manager::get_aabb_indirect(model_id);
    if (physics_enabled)
        physics_id = Physics::add_box(position, (aabb.max - aabb.min) * scale, false);
}

Entity::Entity(glm::vec3 position,
               glm::quat rotation,
               glm::vec3 scale,
               std::string model_name,
               bool physics_enabled,
               bool fade, float ttl, float max_ttl,
               bool is_animated)
        : position(position),
          rotation(rotation),
          scale(scale),
          physics_enabled(physics_enabled),
          fade(fade), ttl(ttl), max_ttl(max_ttl), 
          is_dirty(true), prev_pos(position), prev_rot(rotation),
          is_animated(is_animated)
{
    if (is_animated) {
        model_id = Model_Manager::load_animated_model(model_name);
        // todo maybe can set animation state, some kind of animation create info?
        animated_model = &Model_Manager::get_animated_model(model_id);
    }
    else
        model_id = Model_Manager::load_model(model_name);
    
    Util::AABB aabb = Model_Manager::get_aabb_indirect(model_id);
    if (physics_enabled)
        physics_id = Physics::add_box(position, (aabb.max - aabb.min) * scale, false);
}

Entity Entity::Animated_Entity(glm::vec3 position, // todo maybe dont even need
                               glm::quat rotation,
                               glm::vec3 scale,
                               model_handle model_id,
                               bool physics_enabled,
                               bool fade, float ttl, float max_ttl)
{
    bool animated = true;
    return Entity(position, rotation, scale, model_id, physics_enabled, fade, ttl, max_ttl, animated);
}

Entity Entity::Animated_Entity(glm::vec3 position, // todo maybe dont even need
    glm::quat rotation,
    glm::vec3 scale,
    std::string model_name,
    bool physics_enabled,
    bool fade, float ttl, float max_ttl)
{
    bool animated = true;
    return Entity(position, rotation, scale, model_name, physics_enabled, fade, ttl, max_ttl, animated);
}

//Entity Animated_Entity(
//    glm::vec3 position,
//    glm::quat rotation,
//    glm::vec3 scale,
//    model_handle model_id,
//    bool physics_enabled,
//    bool fade = false,
//    float ttl = 0.0f,
//    float max_ttl = 0.0f
//);

Entity::~Entity() = default;

glm::mat4 Entity::get_model_matrix() const { // todo cache matrix with is_dirty so dont do this every time
    glm::mat4 model_matrix(1.0f);
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), physics_enabled ? Physics::get_body_position(physics_id) : position);
    glm::mat4 rot = glm::mat4_cast(physics_enabled ? Physics::get_body_rotation(physics_id) : rotation);
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);

    model_matrix = translation * rot * scaling;
    return model_matrix;
}

void Entity::check_moved() {
    if (physics_enabled) {
        if (Physics::is_active(physics_id)) 
        {
            glm::vec3 pos = Physics::get_body_position(physics_id);
            if (pos != prev_pos) {
                prev_pos = pos;
                is_dirty = true;
                // update mesh AABB
                return;
            }

            glm::quat rot = Physics::get_body_rotation(physics_id);
            if (rot != prev_rot) {
                prev_rot = rot;
                is_dirty = true;
                // update mesh AABB
                return;
            }

            // todo add scale
        }
    }
    is_dirty = false;
}

glm::vec3 Entity::get_physics_position() const {
    return Physics::get_body_position(physics_id);
}

// todo implement
//Util::AABB Entity::get_aabb() {
//    return Physics::getShapeBounds(physics_id);
//}
