#include "core/scene.h"

#include "asset/material_manager.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "dearimgui/imgui.h"

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
            GPU_Mesh gpu_m;
            gpu_m.transform = m.transform;
            gpu_m.base_vertex = m.base_vertex;
            gpu_m.vertex_count = m.vertex_count;
            gpu_m.base_index = m.base_index;
            gpu_m.index_count = m.index_count;
            gpu_m.bounding_sphere = m.bounding_sphere;
            gpu_m.bounding_sphere.w *= std::max(ntitty.scale.x, std::max(ntitty.scale.y, ntitty.scale.z));
            gpu_m.entity_index = index;
            gpu_m.skinned_to_static_offset = skinned_to_static_offset;
            gpu_m.bone_offset = bone_offset;

            if (ntitty.is_animated)
                animated_mesh_to_all_mesh_mapping.push_back(gpu_meshes.size());
            
            gpu_meshes.push_back(gpu_m);

            //printf("[%u] sphere %f %f %f %f\n", index, gpu_m.bounding_sphere.x, gpu_m.bounding_sphere.y, gpu_m.bounding_sphere.z, gpu_m.bounding_sphere.w);

            Per_Object_Data obj_data;
            // obj_data.model_matrix = g.transform * m.transform; // todo write in gpu
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

    glNamedBufferSubData(animated_mesh_to_all_mesh_mapping_ssbo, 0, sizeof(uint32_t) * animated_mesh_to_all_mesh_mapping.size(), animated_mesh_to_all_mesh_mapping.data());
}

void Scene::update_dirty() {
    for (size_t i = 0; i < entities.size(); ++i) {
        Entity& entity = entities[i];
        entity.check_moved();

        if (entity.is_dirty) {
            glm::mat4 new_transform = entity.get_model_matrix();
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
    if (ImGui::Begin("Scene Inspector")) {
        
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
                    glm::vec3 pos = entity.physics_enabled ? entity.get_physics_position() : entity.position;
                    ImGui::Text("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
                    ImGui::Text("Scale: %.2f, %.2f, %.2f", entity.scale.x, entity.scale.y, entity.scale.z);
                    ImGui::Text("Rotation: %.2f, %.2f, %.2f, %.2f", 
                               entity.rotation.x, entity.rotation.y, entity.rotation.z, entity.rotation.w);
                    
                    // Show model matrix
                    glm::mat4 model_mat = entity.get_model_matrix();
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
        
        // Timed Entities
        if (!timed_entities.empty() && ImGui::CollapsingHeader("Timed Entities")) {
            for (size_t i = 0; i < timed_entities.size(); ++i) {
                const Entity& entity = timed_entities[i];
                
                std::string label = "Timed Entity " + std::to_string(i);
                if (ImGui::TreeNode(label.c_str())) {
                    ImGui::Text("TTL: %.2f / %.2f", entity.ttl, entity.max_ttl);
                    float progress = entity.ttl / entity.max_ttl;
                    ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
                    
                    glm::vec3 pos = entity.physics_enabled ? entity.get_physics_position() : entity.position;
                    ImGui::Text("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
                    ImGui::Text("Model ID: %u", entity.model_id);
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
