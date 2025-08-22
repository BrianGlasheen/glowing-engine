#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>

#include "core/entity.h"
#include "asset/skybox.h"

class Scene {
public:
    Scene();
    ~Scene();

    void load_skybox(const std::string& path);
    void include(Entity ntitty);
    void update_dirty();
    // returns the number of hits
    //int cast_ray(const glm::vec3& pos, const glm::vec3& dir, glm::vec3& hit_pos);

    std::vector<Entity> entities;
    std::vector<Entity> timed_entities;
    Skybox skybox;
};
#endif
