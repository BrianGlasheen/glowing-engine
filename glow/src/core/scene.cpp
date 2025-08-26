#include "scene.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Scene::Scene() {
    entities = std::vector<Entity>();
    timed_entities = std::vector<Entity>();

    gpu_meshes = std::vector<GPU_Mesh>();
    gpu_entities = std::vector<GPU_Entity>();
    per_mesh_data = std::vector<Per_Object_Data>();
}

Scene::~Scene() {}

void Scene::init(const std::string& path) {
    skybox.load(path);

    // gen buffers
    glCreateBuffers(1, &gpu_mesh_ssbo);
    glNamedBufferStorage(gpu_mesh_ssbo, sizeof(GPU_Mesh) * 4000, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &gpu_entity_ssbo);
    glNamedBufferStorage(gpu_entity_ssbo, sizeof(GPU_Entity) * 4000, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &per_mesh_ssbo);
    glNamedBufferStorage(per_mesh_ssbo, sizeof(Per_Object_Data) * 4000, nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void Scene::include(Entity ntitty) { // and maybe dont copy everything in Lol
    if (ntitty.fade) {
        timed_entities.push_back(ntitty); // maybe dont copy everything in, fine for now
    }
    else { // todo move per object data here
        entities.push_back(ntitty);
        uint32_t index = gpu_entities.size();
        GPU_Entity g;
        g.transform = ntitty.get_model_matrix();
        gpu_entities.push_back(g);

        // todo EW!
        for (Mesh& m : Model_Manager::get_model_ind(ntitty.model_id).m_meshes) {
            GPU_Mesh gpu_m;
            gpu_m.base_vertex = m.base_vertex;
            gpu_m.vertex_count = m.vertex_count;
            gpu_m.base_index = m.base_index;
            gpu_m.index_count = m.index_count;
            gpu_m.bounding_sphere = m.bounding_sphere;
            gpu_m.entity_index = index;
            gpu_meshes.push_back(gpu_m);

            Per_Object_Data obj_data;
            obj_data.model_matrix = g.transform * m.transform;
            obj_data.normal_matrix = glm::transpose(glm::inverse(obj_data.model_matrix));
            const Material& mater = m.material;
            obj_data.albedo = mater.albedo;
            obj_data.normal = mater.normal;
            obj_data.met_rough = mater.met_rough;
            obj_data.emissive = mater.emissive;
            obj_data.amb_occ = mater.amb_occ;
            obj_data.emissive_factor = mater.emissive_factor;
            obj_data.metallic_factor = mater.metallic_factor; // 4
            obj_data.roughness_factor = mater.roughness_factor; // 4
            obj_data.base_color = mater.base_color;
            obj_data.alpha_cutoff = mater.alpha_cutoff;
            per_mesh_data.push_back(obj_data);
        }
    }
}

void Scene::upload_buffers() {
    glNamedBufferSubData(gpu_mesh_ssbo, 0, sizeof(GPU_Mesh) * gpu_meshes.size(), gpu_meshes.data());

    glNamedBufferSubData(gpu_entity_ssbo, 0, sizeof(GPU_Entity) * gpu_entities.size(), gpu_entities.data());

    glNamedBufferSubData(per_mesh_ssbo, 0, sizeof(Per_Object_Data) * per_mesh_data.size(), per_mesh_data.data());
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
