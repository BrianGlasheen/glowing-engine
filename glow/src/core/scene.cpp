#include "core/scene.h"

#include "asset/material_manager.h"

#include "util/math.h"

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
    // terrain.init(2000.0f, 2000.0f, 50, 50, "../resources/textures/terrain/atx.png");

    create_buffers();
}

void Scene::create_buffers() {
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
    else { // todo move per object data here
        entities.push_back(ntitty);

        uint32_t index = gpu_entities.size();
        GPU_Entity g;
        g.transform = ntitty.get_model_matrix();
        //g.animation_command_index = ntitty.is_animated ? Model_Manager::get_num_animation_commands() : 0xFFFFFFFF;
        // todo set needs flag default
        gpu_entities.push_back(g);

        if (ntitty.is_animated)
            Model_Manager::submit_animation_command(ntitty.model_id);

        // if animated set flags
        // add animation command to animation system with entity index
        //void submit_animation_command(uint32_t model_id)
        //n_cmds

        // todo EW!
        const std::vector<Mesh>& meshes = ntitty.is_animated
            ? Model_Manager::get_animated_model(ntitty.model_id).m_meshes
            : Model_Manager::get_model_ind(ntitty.model_id).m_meshes;

        uint32_t skinned_to_static_offset = ntitty.is_animated ? Model_Manager::get_animated_model(ntitty.model_id).animation_offset : 0xFFFFFFFF;
        uint32_t bone_offset = ntitty.is_animated ? Model_Manager::get_animated_model(ntitty.model_id).bone_offset : 0xFFFFFFFF;

        for (const Mesh& m : meshes) {
            const Material& mater = m.material;
            
            GPU_Mesh gpu_m;
            gpu_m.transform = m.transform;
            gpu_m.base_vertex = m.base_vertex;
            gpu_m.vertex_count = m.vertex_count;
            gpu_m.base_index = m.base_index;
            gpu_m.index_count = m.index_count;
            gpu_m.bounding_sphere = m.bounding_sphere;
            gpu_m.bounding_sphere.w *= std::max(ntitty.m_scale.x, std::max(ntitty.m_scale.y, ntitty.m_scale.z));
            gpu_m.entity_index = index;
            gpu_m.skinned_to_static_offset = skinned_to_static_offset;
            gpu_m.bone_offset = bone_offset;
            gpu_m.transparent = mater.blend_mode != Blend_Mode::disabled ? 1 : 0;

            if (ntitty.is_animated)
                animated_mesh_to_all_mesh_mapping.push_back(gpu_meshes.size());
            
            gpu_meshes.push_back(gpu_m);

            //printf("[%u] sphere %f %f %f %f\n", index, gpu_m.bounding_sphere.x, gpu_m.bounding_sphere.y, gpu_m.bounding_sphere.z, gpu_m.bounding_sphere.w);

            Per_Object_Data obj_data;
            // obj_data.model_matrix = g.transform * m.transform; // todo write in gpu
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
            per_mesh_data.push_back(obj_data);
        }
    }
}

void Scene::upload_buffers() {
    glNamedBufferSubData(gpu_mesh_ssbo, 0, sizeof(GPU_Mesh) * gpu_meshes.size(), gpu_meshes.data());

    glNamedBufferSubData(gpu_entity_ssbo, 0, sizeof(GPU_Entity) * gpu_entities.size(), gpu_entities.data());

    glNamedBufferSubData(per_mesh_ssbo, 0, sizeof(Per_Object_Data) * per_mesh_data.size(), per_mesh_data.data());

    glNamedBufferSubData(animated_mesh_to_all_mesh_mapping_ssbo, 0, sizeof(uint32_t) * animated_mesh_to_all_mesh_mapping.size(), animated_mesh_to_all_mesh_mapping.data());
}

void Scene::update_dirty() {
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

void Scene::imgui() {
    // Main Scene Inspector Window
    if (ImGui::Begin("Scene")) {
        // Scene Overview
        if (ImGui::CollapsingHeader("Scene Overview", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Total Entities: %zu", entities.size());
            ImGui::Text("Timed Entities: %zu", timed_entities.size());
            ImGui::Text("GPU Meshes: %zu", gpu_meshes.size());
            ImGui::Text("GPU Entities: %zu", gpu_entities.size());
            ImGui::Text("Animated Meshes: %zu", animated_mesh_to_all_mesh_mapping.size());
            
            ImGui::Separator();
            
            // Buffer info
            ImGui::Text("Buffer Sizes:");
            ImGui::Indent();
            ImGui::Text("GPU Mesh SSBO: %u", gpu_mesh_ssbo);
            ImGui::Text("GPU Entity SSBO: %u", gpu_entity_ssbo);
            ImGui::Text("Per Mesh SSBO: %u", per_mesh_ssbo);
            ImGui::Text("Animation Mapping SSBO: %u", animated_mesh_to_all_mesh_mapping_ssbo);
            ImGui::Unindent();
        }
        
        // Entities List
        if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
            static int selected_entity = -1;
            
            // Entity list
            if (ImGui::BeginChild("EntityList", ImVec2(0, 200), true)) {
                for (size_t i = 0; i < entities.size(); ++i) {
                    const Entity& entity = entities[i];
                    
                    // Create entity label
                    std::string label = "Entity " + std::to_string(i);
                    if (entity.is_animated) label += " (Animated)";
                    if (entity.physics_enabled) label += " (Physics)";
                    if (entity.is_dirty) label += " (Dirty)";
                    
                    bool is_selected = (selected_entity == (int)i);
                    if (ImGui::Selectable(label.c_str(), is_selected)) {
                        selected_entity = (int)i;
                    }
                }
            }
            ImGui::EndChild();
            
            // Selected entity details
            if (selected_entity >= 0 && selected_entity < (int)entities.size()) {
                ImGui::Separator();
                ImGui::Text("Entity %d Details:", selected_entity);
                
                const Entity& entity = entities[selected_entity];
                
                // Transform info
                if (ImGui::TreeNode("Transform")) {
                    vec3 pos = entity.physics_enabled ? entity.get_physics_position() : entity.position;
                    ImGui::Text("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
                    ImGui::Text("Scale: %.2f, %.2f, %.2f", entity.m_scale.x, entity.m_scale.y, entity.m_scale.z);
                    ImGui::Text("Rotation: %.2f, %.2f, %.2f, %.2f", 
                               entity.rotation.x, entity.rotation.y, entity.rotation.z, entity.rotation.w);
                    
                    // Show model matrix
                    mat4 model_mat = entity.get_model_matrix();
                    if (ImGui::TreeNode("Model Matrix")) {
                        for (int row = 0; row < 4; ++row) {
                            ImGui::Text("%.2f  %.2f  %.2f  %.2f", 
                                       model_mat[0][row], model_mat[1][row], 
                                       model_mat[2][row], model_mat[3][row]);
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
                
                // Entity properties
                if (ImGui::TreeNode("Properties")) {
                    ImGui::Text("Model ID: %u", entity.model_id);
                    ImGui::Text("Physics Enabled: %s", entity.physics_enabled ? "Yes" : "No");
                    ImGui::Text("Is Animated: %s", entity.is_animated ? "Yes" : "No");
                    ImGui::Text("Is Dirty: %s", entity.is_dirty ? "Yes" : "No");
                    ImGui::Text("Fade: %s", entity.fade ? "Yes" : "No");
                    
                    if (entity.fade) {
                        ImGui::Text("TTL: %.2f / %.2f", entity.ttl, entity.max_ttl);
                        float progress = entity.ttl / entity.max_ttl;
                        ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
                    }
                    
                    if (entity.physics_enabled) {
                        ImGui::Text("Physics ID: %u", entity.physics_id);
                    }
                    ImGui::TreePop();
                }
            }
        }
        
        // GPU Data
        if (ImGui::CollapsingHeader("GPU Data")) {
            static int selected_mesh = -1;
            
            if (ImGui::BeginChild("MeshList", ImVec2(0, 150), true)) {
                for (size_t i = 0; i < gpu_meshes.size(); ++i) {
                    const GPU_Mesh& mesh = gpu_meshes[i];
                    
                    std::string label = "Mesh " + std::to_string(i) + " (Entity " + std::to_string(mesh.entity_index) + ")";
                    bool is_selected = (selected_mesh == (int)i);
                    if (ImGui::Selectable(label.c_str(), is_selected)) {
                        selected_mesh = (int)i;
                    }
                }
            }
            ImGui::EndChild();
            
            if (selected_mesh >= 0 && selected_mesh < (int)gpu_meshes.size()) {
                const GPU_Mesh& mesh = gpu_meshes[selected_mesh];
                
                ImGui::Separator();
                ImGui::Text("GPU Mesh %d Details:", selected_mesh);
                ImGui::Text("Entity Index: %u", mesh.entity_index);
                ImGui::Text("Vertex Count: %u (Base: %u)", mesh.vertex_count, mesh.base_vertex);
                ImGui::Text("Index Count: %u (Base: %u)", mesh.index_count, mesh.base_index);
                ImGui::Text("Bounding Sphere: %.2f, %.2f, %.2f (R: %.2f)", 
                           mesh.bounding_sphere.x, mesh.bounding_sphere.y, 
                           mesh.bounding_sphere.z, mesh.bounding_sphere.w);
                
                if (mesh.skinned_to_static_offset != 0xFFFFFFFF) {
                    ImGui::Text("Animation Offset: %u", mesh.skinned_to_static_offset);
                }
                if (mesh.bone_offset != 0xFFFFFFFF) {
                    ImGui::Text("Bone Offset: %u", mesh.skinned_to_static_offset);
                }
            }
        }
        
        // Controls
        if (ImGui::CollapsingHeader("Controls")) {
            if (ImGui::Button("Upload Buffers")) {
                upload_buffers();
            }
            ImGui::SameLine();
            if (ImGui::Button("Update Dirty")) {
                update_dirty();
            }
            
            ImGui::Separator();
            
            // Add some scene statistics
            int dirty_count = 0;
            int physics_count = 0;
            int animated_count = 0;
            
            for (const Entity& entity : entities) {
                if (entity.is_dirty) dirty_count++;
                if (entity.physics_enabled) physics_count++;
                if (entity.is_animated) animated_count++;
            }
            
            ImGui::Text("Statistics:");
            ImGui::Indent();
            ImGui::Text("Dirty Entities: %d", dirty_count);
            ImGui::Text("Physics Entities: %d", physics_count);
            ImGui::Text("Animated Entities: %d", animated_count);
            ImGui::Unindent();
        }
    }
    ImGui::End();
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
