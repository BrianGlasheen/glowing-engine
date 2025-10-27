#include "scene/scene.h"

#include "glow_config.h"

#include "asset/material_manager.h"

#include "util/math.h"
#include "util/profiler.h"

#include <dearimgui/imgui.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <algorithm>

Scene::Scene() {
    entities = std::vector<Entity>();
    timed_entities = std::vector<Entity>();

    gpu_meshes = std::vector<GPU_Mesh>();
    gpu_entities = std::vector<GPU_Entity>();
    per_mesh_data = std::vector<Per_Object_Data>();
    animated_mesh_to_all_mesh_mapping = std::vector<uint32_t>();
}

Scene::~Scene() {}

void Scene::init(const std::string& path) {
    skybox.load(path);

    // terrain.init();

    create_buffers();
}

void Scene::create_buffers() {
    // todo lol
    terrain.init(2000.0f, 2000.0f, 50, 50, "../resources/textures/terrain/atx.png");

    // gen buffers
    glCreateBuffers(1, &gpu_mesh_ssbo);
    glNamedBufferStorage(gpu_mesh_ssbo, sizeof(GPU_Mesh) * 8000, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &gpu_entity_ssbo);
    glNamedBufferStorage(gpu_entity_ssbo, sizeof(GPU_Entity) * 8000, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &per_mesh_ssbo);
    glNamedBufferStorage(per_mesh_ssbo, sizeof(Per_Object_Data) * 8000, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &animated_mesh_to_all_mesh_mapping_ssbo);
    glNamedBufferStorage(animated_mesh_to_all_mesh_mapping_ssbo, sizeof(uint32_t) * 8000, nullptr, GL_DYNAMIC_STORAGE_BIT);
}

void Scene::include(Entity& ntitty) { // and maybe dont copy everything in Lol
    // todo 
    // here want to put entities into certain buckets
    // static geometry has its own buffer, maybe static geom isnt an entity, but should exist as its own object, but here in the scene (maybe anotehr function to add)
    //   - dont need to check if its dirty
    //   - certain physics path
    // 
    // dynamic entities
    // need to check dirty, update state in buffer
    // more to come!?

    if (ntitty.fade) {
        assert(false);
        timed_entities.push_back(ntitty); // maybe dont copy everything in, fine for now
    }
    else {
        entities.push_back(ntitty);
        add_entity_to_gpu_buffers(ntitty);
    }
}

void Scene::add_entity_to_gpu_buffers(Entity& ntitty) {
    uint32_t index = gpu_entities.size();
    GPU_Entity g = { 0 };
    g.transform = ntitty.get_model_matrix();
    //g.animation_command_index = ntitty.is_animated ? Model_Manager::get_num_animation_commands() : 0xFFFFFFFF;
    // todo set needs flag default
    gpu_entities.push_back(g);

#if GPU_ANIMATION
    if (ntitty.is_animated)
        Model_Manager::submit_animation_command(ntitty.model_id);
#endif
    // if animated set flags
    // add animation command to animation system with entity index
    //void submit_animation_command(uint32_t model_id)
    //n_cmds

    // todo EW!
    const std::vector<Mesh>& meshes = ntitty.is_animated
        ? Model_Manager::get_animated_model(ntitty.model_id).m_meshes
        : Model_Manager::get_model(ntitty.model_id).m_meshes;

    uint32_t skinned_to_static_offset = ntitty.is_animated ? Model_Manager::get_animated_model(ntitty.model_id).animation_offset : 0xFFFFFFFF;
    uint32_t bone_offset = ntitty.is_animated ? Model_Manager::get_animated_model(ntitty.model_id).bone_offset : 0xFFFFFFFF;

    for (const Mesh& m : meshes) {
        const Material& mater = m.material;

        GPU_Mesh gpu_m = {
            .transform = m.transform,
            .base_vertex = (int32_t)m.base_vertex,
            .vertex_count = m.vertex_count,
            .base_index = m.base_index,
            .index_count = m.index_count,
            .bounding_sphere = m.bounding_sphere,
            .entity_index = index,
            .skinned_to_static_offset = skinned_to_static_offset,
            .bone_offset = bone_offset,
            .transparent = mater.blend_mode != Blend_Mode::disabled ? 1u : 0u
        };
        gpu_m.bounding_sphere.w *= std::max(ntitty.m_scale.x, std::max(ntitty.m_scale.y, ntitty.m_scale.z));

        if (ntitty.is_animated)
            animated_mesh_to_all_mesh_mapping.push_back(gpu_meshes.size());

        gpu_meshes.push_back(gpu_m);

        //printf("[%u] sphere %f %f %f %f\n", index, gpu_m.bounding_sphere.x, gpu_m.bounding_sphere.y, gpu_m.bounding_sphere.z, gpu_m.bounding_sphere.w);

        Per_Object_Data obj_data = { 0 };
        obj_data.model_matrix = g.transform * m.transform; // gets written in gpu when entity changes pos
        obj_data.normal_matrix = transpose(inverse(obj_data.model_matrix));
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
        obj_data.id = index;
        per_mesh_data.push_back(obj_data);
    }
}

void Scene::upload_buffers() {
    glNamedBufferSubData(gpu_mesh_ssbo, 0, sizeof(GPU_Mesh) * gpu_meshes.size(), gpu_meshes.data());
    glNamedBufferSubData(gpu_entity_ssbo, 0, sizeof(GPU_Entity) * gpu_entities.size(), gpu_entities.data());
    glNamedBufferSubData(per_mesh_ssbo, 0, sizeof(Per_Object_Data) * per_mesh_data.size(), per_mesh_data.data());
    glNamedBufferSubData(animated_mesh_to_all_mesh_mapping_ssbo, 0, sizeof(uint32_t) * animated_mesh_to_all_mesh_mapping.size(), animated_mesh_to_all_mesh_mapping.data());
}

void Scene::update_dirty() {
    PROFILE_SCOPE_COLOR("update scene dirty", legit::Colors::clouds);

    for (size_t i = 0; i < entities.size(); ++i) {
        Entity& entity = entities[i];
        entity.check_moved();

        if (entity.is_dirty) {
            mat4 new_transform = entity.get_model_matrix();
            gpu_entities[i].transform = new_transform; // maybe dont need to store?
            gpu_entities[i].is_dirty = true;

            // if scale changes also change bounding sphere w (pre baked entity scale)

            glNamedBufferSubData(gpu_entity_ssbo, i * sizeof(GPU_Entity), sizeof(GPU_Entity), &gpu_entities[i]);

            entity.is_dirty = false;
        }
    }
}

void Scene::refresh() {
    gpu_entities.clear();
    gpu_meshes.clear();
    per_mesh_data.clear();
    animated_mesh_to_all_mesh_mapping.clear();

    for (Entity& e : entities)
        add_entity_to_gpu_buffers(e);

    upload_buffers();
}

namespace YAML {
    template<>
    struct convert<vec3> {
        static Node encode(const vec3& v) {
            Node node;
            node.push_back(v.x);
            node.push_back(v.y);
            node.push_back(v.z);
            return node;
        }

        static bool decode(const Node& node, vec3& rhs) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };


    template<>
    struct convert<quat> {
        static Node encode(const quat& q) {
            Node node;
            node.push_back(q.w);
            node.push_back(q.x);
            node.push_back(q.y);
            node.push_back(q.z);
            return node;
        }

        static bool decode(const Node& node, quat& rhs) {
            if (!node.IsSequence() || node.size() != 4) {
                return false;
            }

            rhs.w = node[0].as<float>();
            rhs.x = node[1].as<float>();
            rhs.y = node[2].as<float>();
            rhs.z = node[3].as<float>();
            return true;
        }
    };

    inline Emitter& operator<<(Emitter& out, const vec3& v) {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    inline Emitter& operator<<(Emitter& out, const quat& q) {
        out << YAML::Flow;
        out << YAML::BeginSeq << q.w << q.x << q.y << q.z << YAML::EndSeq;
        return out;
    }
}

void Scene::serialize(std::string path) {

    printf("WRITING SCENE\n");

    YAML::Emitter out;
    out << YAML::BeginMap;

    // todo scene name
    out << YAML::Key << "Scene" << YAML::Value << "Untitled";

    out << YAML::Key << "Skybox" << YAML::Value << skybox.name;

    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
    for (const auto& entity : entities) {
        out << YAML::BeginMap;

        out << YAML::Key << "position" << YAML::Value << entity.position;
        out << YAML::Key << "rotation" << YAML::Value << entity.rotation;
        out << YAML::Key << "scale"    << YAML::Value << entity.m_scale;
        out << YAML::Key << "model" << YAML::Value << Model_Manager::get_model_name(entity.model_id, entity.is_animated);
        out << YAML::Key << "physics" << YAML::Value << entity.physics_enabled;
        out << YAML::Key << "animated" << YAML::Value << entity.is_animated;

        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;
    std::ofstream fout(path);
    fout << out.c_str();
}

void Scene::load_from_file(std::string path) {
    printf("LOADING SCENE\n");

    YAML::Node data = YAML::LoadFile(path);

    if (!data["Scene"] || !data["Entities"]) {
        assert(false);
    }

    // std::string scene_name = data["Scene"].as<std::string>();
    skybox.load(data["Skybox"].as<std::string>());

    create_buffers();


    for (const auto& node : data["Entities"]) {
        vec3 position = node["position"].as<vec3>();
        quat rotation = node["rotation"].as<quat>();
        vec3 scale = node["scale"].as<vec3>();
        std::string model_name = node["model"].as<std::string>();
        bool animated = node["animated"].as<bool>();
        bool physics_enabled = node["physics"].as<bool>();
        
        Entity e(position, rotation, scale, model_name, physics_enabled, 0, 0, 0, animated);
        include(e);
    }

    printf("Loaded %zu entities.\n", entities.size());
}


//int Scene::cast_ray(const vec3& pos, const vec3& dir, vec3& hit_pos) {
//    int hits = 0;
//    float min_dist = 999999999.0f;
//    vec3 hit_pos_temp(0.0f);
//
//    for (Entity e : entities) {
//        if (e.collides(pos, dir, hit_pos_temp)) {
//            hits++;
//            float dist = distance(hit_pos_temp, pos);
//            if (dist < min_dist) {
//                min_dist = dist;
//                hit_pos  = hit_pos_temp;
//            }
//        }
//    }
//
//    return hits;
//}
