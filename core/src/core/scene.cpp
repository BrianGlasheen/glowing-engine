#include "scene.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Scene::Scene() {
    entities = std::vector<Entity>();
    timed_entities = std::vector<Entity>();
}

Scene::~Scene() {}

void Scene::load_skybox(const std::string& path) {
    skybox.load(path);
}

void Scene::include(Entity ntitty) { // and maybe dont copy everything in Lol
    if (ntitty.fade) {
        timed_entities.push_back(ntitty); // maybe dont copy everything in, fine for now
    }
    else {
        entities.push_back(ntitty);
    }
}

void Scene::update_dirty() {
    for (Entity& entity : entities) {
        entity.check_moved();
    }
}


//int Scene::cast_ray(const glm::vec3& pos, const glm::vec3& dir, glm::vec3& hit_pos) {
//    int hits = 0;
//    float min_dist = 999999999.0f;
//    glm::vec3 hit_pos_temp(0.0f);
//
//    for (Entity e : entities) {
//        if (e.collides(pos, dir, hit_pos_temp)) {
//            hits++;
//            float dist = glm::distance(hit_pos_temp, pos);
//            if (dist < min_dist) {
//                min_dist = dist;
//                hit_pos  = hit_pos_temp;
//            }
//        }
//    }
//
//    return hits;
//}
