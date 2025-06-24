#include "entity.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

//Entity::Entity(
//    Model_ass* model, 
//    glm::vec3 position, 
//    bool physics_enabled, 
//    glm::vec3 scale, 
//    float mass,
//    glm::quat orientation,
//    bool fade, float ttl, float max_ttl, float collider_radius
//) : 
//    model(model),
//    model_id(0),
//    position(position), 
//    scale(scale),
//    fade(fade), ttl(ttl), max_ttl(max_ttl), 
//    rotation(glm::vec3(0.0f))
//{
//}

Entity::Entity(
    model_handle model_id,
    glm::vec3 position,
    bool physics_enabled,
    glm::vec3 scale,
    float mass,
    glm::quat orientation,
    bool fade, float ttl, float max_ttl
) :
    model_id(model_id),
    position(position),
    physics_enabled(physics_enabled),
    scale(scale),
    fade(fade), ttl(ttl), max_ttl(max_ttl),
    rotation(glm::vec3(0.0f))
{
    // consume bb
    aabb = Model_manager::get_aabb(model_id);
    // add to physics simulation
    if (physics_enabled)
        physics_id = Physics::addBox(position, (aabb.max - aabb.min) * scale, false);
}

Entity::Entity(
    std::string model_name,
    glm::vec3 position,
    bool physics_enabled,
    glm::vec3 scale,
    float mass,
    glm::quat,
    bool fade,
    float ttl,
    float max_ttl
) :
    position(position),
    physics_enabled(physics_enabled),
    scale(scale),
    fade(fade), ttl(ttl), max_ttl(max_ttl),
    rotation(glm::vec3(0.0f)) 
{
    model_id = Model_manager::load_model(model_name);
    // consume bb
    aabb = Model_manager::get_aabb(model_id);
    // add to physics simulation
    //glm::vec3 middle = (aabb.max + aabb.min) / 2.0f;
    //float x = aabb.max.x - aabb.min.x;
    //float y = aabb.max.y - aabb.min.y;
    //float z = aabb.max.z - aabb.min.z;
    if (physics_enabled)
        physics_id = Physics::addBox(position, (aabb.max - aabb.min) * scale, false);
}

Entity::~Entity() = default;

glm::mat4 Entity::get_model_matrix() const {
    glm::mat4 modelMat(1.0f);
    //glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), physics_enabled ? Physics::getBodyPosition(physics_id) : position);
    //glm::mat4 rot = glm::mat4_cast(rotation);
    glm::mat4 rot = glm::mat4_cast(Physics::getBodyRotation(physics_id));
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);

    modelMat = translation * rot * scaling;
    return modelMat;
}

void Entity::draw(const Shader* shader, bool shadow_pass) {

    //if (model_id == 0)
        //model->draw(shader);
    //Model_ass tmp = Model_manager::get_model(model_id);
    //tmp.draw(shader);
    //else {
        //printf("model drawn with id, %s", Model_manager::get_name(model_id).c_str());
        Model_manager::draw(shader, model_id, shadow_pass);
    //}
}

bool Entity::collides(const glm::vec3& pos, const glm::vec3& dir, glm::vec3& hit_pos) {
    // For a simple sphere collision check with radius=1
    glm::vec3 oc = pos - position;  // from ray start to entity center
    float a = glm::dot(dir, dir);
    float b = 2.0f * glm::dot(oc, dir);
    float c = glm::dot(oc, oc) - 1.0f;  // sphere radius^2 is 1^2 = 1

    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0) {
        return false; 
    }

    float sqrtDisc = glm::sqrt(discriminant);
    float t = (-b - sqrtDisc) / (2.0f * a);
    if (t < 0) {
        t = (-b + sqrtDisc) / (2.0f * a);
        if (t < 0) {
            return false;
        }
    }
    hit_pos = pos + t * dir;
    return true;
}

glm::vec3 Entity::get_physics_position() {
    return Physics::getBodyPosition(physics_id);
}

//Util::aabb Entity::get_aabb() {
//    return Physics::getShapeBounds(physics_id);
//}
