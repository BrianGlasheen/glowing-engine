#pragma once

#include <vector>

// #include <GLFW/glfw3.h>
#include "asset/model_manager.h"
#include "core/opengl.h"

#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/quaternion.hpp>

#include "asset/shader_manager.h"
#include "asset/font.h"
#include "asset/text.h"
#include "core/renderer.h"
#include "util/colors.h"
#include "core/camera.h"
#include "util/decompose.h"
#include "util/math.h"

#include "editor/themes.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <imguizmo/ImGuizmo.h>

enum class view_type {
    TOP_DOWN = 0,
    FRONT,
    SIDE,
    SCENE
};

enum gizmo_modes {
    NONE = 0,
    TRANSLATE,
    ROTATE,
    SCALE
};
static std::string gize_mode_strs[]{ "none", "translate", "rotate", "scale" };

// struct view_type_data {
//     view_type type;

//     // TODO make not shit
//     Font FIX_font;

//     // Camera positioning
//     float zoom_level;           // Orthographic size multiplier (smaller = more zoomed in)
//     glm::vec2 pan_offset;       // X/Y offset for panning in view space
//     float camera_distance;      // Distance from target point

//     // View bounds and limits
//     float min_zoom;            // Minimum zoom level (max zoom in)
//     float max_zoom;            // Maximum zoom level (max zoom out)
//     float zoom_speed;          // How fast zoom responds to input
//     float pan_speed;           // How fast panning responds to input
//     glm::vec2 pan_limits;      // Maximum pan distance from center

//     // Input state
//     bool is_panning;           // Currently panning with mouse
//     glm::vec2 last_mouse_pos;  // Last mouse position for delta calculation
//     bool is_zooming;           // Currently zooming
//     gizmo_modes gizmo_mode;

//     // Visual settings
//     bool show_grid;            // Show grid overlay
//     float grid_size;           // Grid cell size in world units
//     glm::vec3 grid_color;      // Grid line color
//     bool show_axes;            // Show world axes
//     bool show_bounds;          // Show scene bounds
//     Text view_text;

//     view_type_data(view_type view_type = view_type::TOP_DOWN)
//         : type(view_type)
//         , zoom_level(1.0f)
//         , pan_offset(0.0f, 0.0f)
//         , camera_distance(50.0f)
//         , min_zoom(0.1f)
//         , max_zoom(10.0f)
//         , zoom_speed(0.1f)
//         , pan_speed(0.1f)
//         , pan_limits(100.0f, 100.0f)
//         , is_panning(false)
//         , last_mouse_pos(0.0f, 0.0f)
//         , is_zooming(false)
//         , gizmo_mode(gizmo_modes::NONE)
//         , show_grid(true)
//         , grid_size(1.0f)
//         , grid_color(0.3f, 0.3f, 0.3f)
//         , show_axes(true)
//         , show_bounds(false)
//     {
//     }

//     void init_text(std::string text) {
//         FIX_font = Font("tx02");
//         view_text.load(FIX_font, text, 0, 1, 100.0f, glm::vec3(1.0f));
//     }

//     // Calculate the actual orthographic size based on zoom
//     float get_ortho_size() const {
//         return 20.0f * zoom_level; // Base size * zoom multiplier
//     }

//     glm::vec3 get_camera_position() const {
//         switch (type) {
//         case view_type::TOP_DOWN:
//             return glm::vec3(0.0f, camera_distance, 0.0f);
//         case view_type::FRONT:
//             return glm::vec3(0.0f, 0.0f, camera_distance);
//         case view_type::SIDE:
//             return glm::vec3(camera_distance, 0.0f, 0.0f);
//         default:
//             assert(false);
//         }
//     }

//     glm::vec3 get_target_position() const {
//         glm::vec3 target = glm::vec3(0.0f);

//         switch (type) {
//         case view_type::TOP_DOWN:
//             target.x += pan_offset.x;
//             target.z += pan_offset.y;
//             break;
//         case view_type::FRONT:
//             target.x += pan_offset.x;
//             target.y += pan_offset.y;
//             break;
//         case view_type::SIDE:
//             target.z += pan_offset.x;
//             target.y += pan_offset.y;
//             break;
//         }

//         return target;
//     }

//     glm::vec3 get_up_vector() const {
//         switch (type) {
//         case view_type::TOP_DOWN:
//             return glm::vec3(0.0f, 0.0f, -1.0f);
//         case view_type::FRONT:
//         case view_type::SIDE:
//             return glm::vec3(0.0f, 1.0f, 0.0f);
//         default:
//             return glm::vec3(0.0f, 1.0f, 0.0f);
//         }
//     }

//     void handle_zoom(float zoom_delta) {
//         if (zoom_delta != 0.0f) {
//             is_zooming = true;
//             zoom_level = glm::clamp(zoom_level + zoom_delta * zoom_speed, min_zoom, max_zoom);
//         }
//         else {
//             is_zooming = false;
//         }
//     }

//     // Handle pan input
//     void handle_pan(const glm::vec2& mouse_delta) {
//         if (is_panning) {
//             glm::vec2 pan_delta = mouse_delta * pan_speed * zoom_level;

//             switch (type) {
//             case view_type::TOP_DOWN:
//                 pan_delta *= -1;
//                 break;
//             case view_type::FRONT:
//                 pan_delta.x *= -1;
//                 break;
//             case view_type::SIDE:
//                 //pan_delta.y *= -1;
//                 break;
//             default:
//                 assert(false);
//             }
//             pan_offset.x = glm::clamp(pan_offset.x + pan_delta.x, -pan_limits.x, pan_limits.x);
//             pan_offset.y = glm::clamp(pan_offset.y + pan_delta.y, -pan_limits.y, pan_limits.y);
//         }
//     }

//     // Start panning
//     void start_pan(const glm::vec2& mouse_pos) {
//         is_panning = true;
//         last_mouse_pos = mouse_pos;
//     }

//     // Stop panning
//     void stop_pan() {
//         is_panning = false;
//     }

//     void set_gizmo_mode(gizmo_modes gm) {
//         gizmo_mode = gm;
//         view_text.update_text(gize_mode_strs[gm]);
//     }
// };

// struct editor_viewports_struct {
//     view_type_data top;
//     view_type_data side;
//     view_type_data front;
//     view_type_data scene;

//     editor_viewports_struct()
//         : top(view_type::TOP_DOWN)
//         , side(view_type::SIDE)
//         , front(view_type::FRONT)
//         , scene(view_type::SCENE)
//     {
//     }
// };

class Editor {
public:
    int init(Renderer* pointy_renderer) {
        // make viewports
        // editor_viewports.top.init_text("top------"); // pad to 9 xD
        // editor_viewports.front.init_text("front----");
        // editor_viewports.side.init_text("side-----");
        // editor_viewports.scene.init_text("scene----");
        renderer = pointy_renderer;

        Shader_Manager::load_from_name("editor");
        Shader_Manager::load_from_name("grid");

        return 0;
    }

    void show(Scene& scene) {
        static bool dockspace_initialized = false;
        static bool dockspace_open = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("DockSpace Window", &dockspace_open, window_flags);
        ImGui::PopStyleVar(2);

        ImGuiID dockspace_id = ImGui::GetID("EditorDockspaceID");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        if (!dockspace_initialized) {
            dockspace_initialized = true;

            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            ImGuiID left, right, bottom, center;
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, &left, &dockspace_id);
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, &right, &dockspace_id);
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, &bottom, &center);
            ImGuiID left_top, left_bottom;
            ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.25f, &left_bottom, &left_top);

            ImGui::DockBuilderDockWindow("Scene", left_top);
            ImGui::DockBuilderDockWindow("Renderer", left_top);
            ImGui::DockBuilderDockWindow("Assets", left_bottom);
            ImGui::DockBuilderDockWindow("Frametime", left_bottom);
            
            ImGui::DockBuilderDockWindow("Inspector", right);
            ImGui::DockBuilderDockWindow("Preview", center);
            ImGui::DockBuilderDockWindow("Console", bottom);

            ImGui::DockBuilderFinish(dockspace_id);

            Themes::gruvbox();
        }

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    // todo exit
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Style")) {
                if (ImGui::MenuItem("Gruvbox"))
                    Themes::gruvbox();
                if (ImGui::MenuItem("purpleish"))
                    Themes::purpleish();
                if (ImGui::MenuItem("dark"))
                    Themes::dark();
                if (ImGui::MenuItem("green"))
                    Themes::green();

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();

        main_view();
        asset_browser();
        scene_imgui(scene);
        renderer_imgui();
        inspector(scene);
    }

    void main_view() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Preview");

        static ImVec2 last_size = ImVec2(0, 0);
        ImVec2 size = ImGui::GetContentRegionAvail();
        if (first_draw || (size.x > 0 && size.y > 0 && (size.x != last_size.x || size.y != last_size.y))) {
            renderer->resize((int)size.x, (int)size.y);
            last_size = size;
            first_draw = false;
        }

        ImVec2 image_pos = ImGui::GetCursorScreenPos();
        ImGui::Image((ImTextureID)(intptr_t)(Texture_Manager::get_ogl_id(renderer->output_texture)), size, ImVec2(0, 1), ImVec2(1, 0));

        ImVec2 image_min = ImGui::GetItemRectMin();
        ImVec2 image_size = ImGui::GetItemRectSize();
        //render_gizmo(scene, active_view, active_proj, image_min, image_size);

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver()) {
            ImVec2 mouse_pos = ImGui::GetMousePos();

            float local_x = mouse_pos.x - image_min.x;
            float local_y = mouse_pos.y - image_min.y;

            if (local_x >= 0 && local_x < image_size.x &&
                local_y >= 0 && local_y < image_size.y) {

                int flipped_y = (int)image_size.y - (int)local_y - 1;
                int px = (int)local_x;
                int py = flipped_y;

                pick(px, py);
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void asset_browser() {
        std::string current_path = "../resources/";
        std::string selected_file = "";

        ImGui::Begin("Assets");
        
        ImGui::Text("Path: %s", current_path.c_str());
        ImGui::Separator();
        
        if (current_path != "assets") {
            if (ImGui::Selectable("../", false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    fs::path p(current_path);
                    current_path = p.parent_path().string();
                }
            }
        }
        
        try {
            for (const auto& entry : fs::directory_iterator(current_path)) {
                std::string filename = entry.path().filename().string();
                bool is_dir = entry.is_directory();
                
                std::string icon = is_dir ? "[DIR] " : "[FILE] ";
                std::string display_name = icon + filename;
                
                bool is_selected = (selected_file == entry.path().string());
                
                if (ImGui::Selectable(display_name.c_str(), is_selected, 
                                     ImGuiSelectableFlags_AllowDoubleClick)) {
                    selected_file = entry.path().string();
                    
                    if (ImGui::IsMouseDoubleClicked(0) && is_dir) {
                        current_path = entry.path().string();
                        selected_file = "";
                    }
                    else if (ImGui::IsMouseDoubleClicked(0) && !is_dir) {
                        // Load the asset (model, texture, etc.)
                        // load_asset(selected_file);
                    }
                }
                
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Open")) {
                        if (is_dir) {
                            current_path = entry.path().string();
                        } else {
                            // load_asset(selected_file);
                        }
                    }
                    if (!is_dir && ImGui::MenuItem("Delete")) {
                        fs::remove(entry.path());
                    }
                    ImGui::EndPopup();
                }
            }
        } catch (const fs::filesystem_error& e) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", e.what());
        }
        
        ImGui::End();
    }

    void renderer_imgui() {
        ImGui::Begin("Renderer");

        // ImGui::Checkbox("depth pre-pass", &use_depth_prepass);
        // ImGui::Checkbox("shadows enabled", &shadows_enabled); // todo maybe use
        ImGui::SliderInt("num_lights", &renderer->num_lights, 0, 1000);
        ImGui::Checkbox("light quads", &renderer->do_draw_light_quads);
        ImGui::Checkbox("forward+", &renderer->forward_plus);
        ImGui::Checkbox("bloom_enabled", &renderer->bloom_enabled);

        // ssao settings
        ImGui::Checkbox("ssao_enabled", &renderer->ssao_enabled);
        ImGui::SliderFloat("ssao_radius", &renderer->ssao_radius, 0, 5.0);
        ImGui::SliderFloat("ssao_bias", &renderer->ssao_bias, 0, 1.0f);
        ImGui::SliderInt("ssao_samples", &renderer->ssao_samples, 0, 64);
        ImGui::SliderFloat("min_depth", &renderer->min_depth, -0.01, 0.2f);
        ImGui::SliderFloat("power", &renderer->power, -2, 4);

        ImGui::End();
    }

    void scene_imgui(Scene& scene) {
        if (ImGui::Begin("Scene")) {
            // Scene Overview
            // if (ImGui::CollapsingHeader("Scene Overview", ImGuiTreeNodeFlags_DefaultOpen)) {
            //     ImGui::Text("Total Entities: %zu", entities.size());
            //     ImGui::Text("Timed Entities: %zu", timed_entities.size());
            //     ImGui::Text("GPU Meshes: %zu", gpu_meshes.size());
            //     ImGui::Text("GPU Entities: %zu", gpu_entities.size());
            //     ImGui::Text("Animated Meshes: %zu", animated_mesh_to_all_mesh_mapping.size());

            //     ImGui::Separator();

            //     // Buffer info
            //     ImGui::Text("Buffer Sizes:");
            //     ImGui::Indent();
            //     ImGui::Text("GPU Mesh SSBO: %u", gpu_mesh_ssbo);
            //     ImGui::Text("GPU Entity SSBO: %u", gpu_entity_ssbo);
            //     ImGui::Text("Per Mesh SSBO: %u", per_mesh_ssbo);
            //     ImGui::Text("Animation Mapping SSBO: %u", animated_mesh_to_all_mesh_mapping_ssbo);
            //     ImGui::Unindent();
            // }

            if (ImGui::TreeNode("Sun Light")) {
                ImGui::SliderFloat3("Direction", &scene.sun_direction.x, -1.0f, 1.0f);
                if (ImGui::Button("Normalize Direction")) {
                    scene.sun_direction = glm::normalize(scene.sun_direction);
                }
                ImGui::ColorEdit3("Color", &scene.sun_color.x);
                ImGui::SliderFloat("Intensity", &scene.sun_strength, 0.0f, 5.0f);
                if (ImGui::Button("Noon")) {
                    scene.sun_direction = vec3(0.0f, -1.0f, 0.0f);
                    scene.sun_color = vec3(1.0f, 1.0f, 0.98f);
                    scene.sun_strength = 1.5f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Sunset")) {
                    scene.sun_direction = vec3(0.7f, -0.3f, 0.0f);
                    scene.sun_color = vec3(1.0f, 0.6f, 0.3f);
                    scene.sun_strength = 0.8f;
                }
                ImGui::SameLine();
                if (ImGui::Button("Night")) {
                    scene.sun_direction = vec3(0.0f, 1.0f, 0.0f);
                    scene.sun_color = vec3(0.3f, 0.4f, 0.6f);
                    scene.sun_strength = 0.1f;
                }

                ImGui::TreePop();
            }

            // Entities List
            if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
                static int selected_entity = -1;

                // Entity list
                if (ImGui::BeginChild("EntityList", ImVec2(0, 200), true)) {
                    for (size_t i = 0; i < scene.entities.size(); ++i) {
                        const Entity& entity = scene.entities[i];

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
                //if (selected_entity >= 0 && selected_entity < (int)entities.size()) {
                //    ImGui::Separator();
                //    ImGui::Text("Entity %d Details:", selected_entity);
                //    
                //    const Entity& entity = entities[selected_entity];

                    //if (ImGui::TreeNode("Transform")) {
                    //    vec3 pos = entity.physics_enabled ? entity.get_physics_position() : entity.position;
                    //    ImGui::Text("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
                    //    ImGui::Text("Scale: %.2f, %.2f, %.2f", entity.m_scale.x, entity.m_scale.y, entity.m_scale.z);
                    //    ImGui::Text("Rotation: %.2f, %.2f, %.2f, %.2f", 
                    //               entity.rotation.x, entity.rotation.y, entity.rotation.z, entity.rotation.w);
                    //    
                    //    // Show model matrix
                    //    mat4 model_mat = entity.get_model_matrix();
                    //    if (ImGui::TreeNode("Model Matrix")) {
                    //        for (int row = 0; row < 4; ++row) {
                    //            ImGui::Text("%.2f  %.2f  %.2f  %.2f", 
                    //                       model_mat[0][row], model_mat[1][row], 
                    //                       model_mat[2][row], model_mat[3][row]);
                    //        }
                    //        ImGui::TreePop();
                    //    }
                    //    ImGui::TreePop();
                    //}

                    // Entity properties
                    //if (ImGui::TreeNode("Properties")) {
                    //    ImGui::Text("Model ID: %u", entity.model_id);
                    //    ImGui::Text("Physics Enabled: %s", entity.physics_enabled ? "Yes" : "No");
                    //    ImGui::Text("Is Animated: %s", entity.is_animated ? "Yes" : "No");
                    //    ImGui::Text("Is Dirty: %s", entity.is_dirty ? "Yes" : "No");
                    //    ImGui::Text("Fade: %s", entity.fade ? "Yes" : "No");
                    //    
                    //    if (entity.fade) {
                    //        ImGui::Text("TTL: %.2f / %.2f", entity.ttl, entity.max_ttl);
                    //        float progress = entity.ttl / entity.max_ttl;
                    //        ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
                    //    }
                    //    
                    //    if (entity.physics_enabled) {
                    //        ImGui::Text("Physics ID: %u", entity.physics_id);
                    //    }
                    //    ImGui::TreePop();
                    //}
                //}
            }

            // GPU Data
            if (ImGui::CollapsingHeader("GPU Data")) {
                static int selected_mesh = -1;

                if (ImGui::BeginChild("MeshList", ImVec2(0, 150), true)) {
                    for (size_t i = 0; i < scene.gpu_meshes.size(); ++i) {
                        const GPU_Mesh& mesh = scene.gpu_meshes[i];

                        std::string label = "Mesh " + std::to_string(i) + " (Entity " + std::to_string(mesh.entity_index) + ")";
                        bool is_selected = (selected_mesh == (int)i);
                        if (ImGui::Selectable(label.c_str(), is_selected)) {
                            selected_mesh = (int)i;
                        }
                    }
                }
                ImGui::EndChild();

                if (selected_mesh >= 0 && selected_mesh < (int)scene.gpu_meshes.size()) {
                    const GPU_Mesh& mesh = scene.gpu_meshes[selected_mesh];

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
                    scene.upload_buffers();
                }
                ImGui::SameLine();
                if (ImGui::Button("Update Dirty")) {
                    scene.update_dirty();
                }

                ImGui::Separator();

                // Add some scene statistics
                int dirty_count = 0;
                int physics_count = 0;
                int animated_count = 0;

                for (const Entity& entity : scene.entities) {
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

    void inspector(Scene& scene) {
        if (ImGui::Begin("Inspector")) {
            if (selected < 0 || selected >= scene.entities.size()) {
                ImGui::Text("Click something");
            }
            else {
                Entity& entity = scene.entities[selected];
                ImGui::Text("Entity Index: %d", selected);
                ImGui::Separator();

                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Text("Position");
                    ImGui::PushItemWidth(80);
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
                    ImGui::DragFloat("##PosX", &entity.position.x, 0.1f); ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
                    ImGui::DragFloat("##PosY", &entity.position.y, 0.1f); ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z"); ImGui::SameLine();
                    ImGui::DragFloat("##PosZ", &entity.position.z, 0.1f);
                    ImGui::PopItemWidth();

                    vec3 euler = glm::eulerAngles(entity.rotation) * (180.0f / PI);
                    ImGui::Text("Rotation");
                    ImGui::PushItemWidth(80);
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
                    float euler_x = euler.x;
                    if (ImGui::DragFloat("##RotX", &euler_x, 1.0f)) {
                        euler.x = euler_x;
                        entity.rotation = quat(euler * (PI / 180.0f));
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
                    float euler_y = euler.y;
                    if (ImGui::DragFloat("##RotY", &euler_y, 1.0f)) {
                        euler.y = euler_y;
                        entity.rotation = quat(euler * (PI / 180.0f));
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z"); ImGui::SameLine();
                    float euler_z = euler.z;
                    if (ImGui::DragFloat("##RotZ", &euler_z, 1.0f)) {
                        euler.z = euler_z;
                        entity.rotation = quat(euler * (PI / 180.0f));
                    }
                    ImGui::PopItemWidth();

                    ImGui::Text("Scale");
                    ImGui::SameLine();
                    static bool scale_locked = true;
                    if (ImGui::Button(scale_locked ? "Locked" : "Unlocked")) {
                        scale_locked = !scale_locked;
                    }
                    ImGui::PushItemWidth(80);
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "X"); ImGui::SameLine();
                    float prev_scale_x = entity.m_scale.x;
                    if (ImGui::DragFloat("##ScaleX", &entity.m_scale.x, 0.01f)) {
                        if (scale_locked) {
                            float ratio = entity.m_scale.x / prev_scale_x;
                            entity.m_scale.y *= ratio;
                            entity.m_scale.z *= ratio;
                        }
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y"); ImGui::SameLine();
                    float prev_scale_y = entity.m_scale.y;
                    if (ImGui::DragFloat("##ScaleY", &entity.m_scale.y, 0.01f)) {
                        if (scale_locked) {
                            float ratio = entity.m_scale.y / prev_scale_y;
                            entity.m_scale.x *= ratio;
                            entity.m_scale.z *= ratio;
                        }
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z"); ImGui::SameLine();
                    float prev_scale_z = entity.m_scale.z;
                    if (ImGui::DragFloat("##ScaleZ", &entity.m_scale.z, 0.01f)) {
                        if (scale_locked) {
                            float ratio = entity.m_scale.z / prev_scale_z;
                            entity.m_scale.x *= ratio;
                            entity.m_scale.y *= ratio;
                        }
                    }
                    ImGui::PopItemWidth();
                }

                if (ImGui::BeginCombo("Model", Model_Manager::get_model_name(entity.model_id, entity.is_animated).c_str())) {
                    uint32_t num_models = Model_Manager::get_num_models();
                    
                    for (uint32_t model = 0; model < num_models; model++) {
                        bool is_selected = (model == entity.model_id && !entity.is_animated);
                        if (ImGui::Selectable(Model_Manager::get_model_name(model, false).c_str(), is_selected)) {
                            entity.model_id = model;
                            entity.is_animated = false;
                            entity.animated_model = nullptr;
                            scene.refresh();
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    //if () {
                    //    ImGui::Separator();
                    //}

                    // animated models

                    ImGui::EndCombo();
                }

                if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Checkbox("Physics Enabled", &entity.physics_enabled);
                    if (entity.physics_enabled) {
                        // change physics properties
                        //ImGui::Text("Body ID: %u", entity.physics_id.GetIndexAndSequenceNumber());
                    }
                }
            }
        }
        ImGui::End();
    }

    void render_selected_outlined(const Scene& scene, const mat4& vp) {
        if (selected < 0 || selected > (scene.entities.size() - 1)) return;

        Shader* shader = Shader_Manager::get_shader("outline");
        shader->use();
        
        shader->set_vec3("color", Util::orange);

        glBindVertexArray(Model_Manager::get_big_vao());
        
        // for (size_t selected : selected_entites) {
        // printf("selected %d\n", selected);
            const Entity& selected_entity = scene.entities[selected];
            
            glCullFace(GL_BACK);
            glEnable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDisable(GL_DEPTH_TEST);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);
            glDisable(GL_BLEND);

            shader->set_float("scale", 0.0);
            for (const Mesh& m : Model_Manager::get_model(selected_entity.model_id).m_meshes) {
                shader->set_mat4("mvp", vp * selected_entity.get_model_matrix() * m.transform);
                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, m.index_count, GL_UNSIGNED_INT, (void*)(m.base_index * sizeof(unsigned int)), 1, m.base_vertex, 0);
            }

            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDisable(GL_CULL_FACE);

            shader->set_float("scale", outline_scale);
            for (const Mesh& m : Model_Manager::get_model(selected_entity.model_id).m_meshes) {
                shader->set_mat4("mvp", vp * selected_entity.get_model_matrix() * m.transform);
                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, m.index_count, GL_UNSIGNED_INT, (void*)(m.base_index * sizeof(unsigned int)), 1, m.base_vertex, 0);
            }

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_STENCIL_TEST);
        // }
    }

    void draw_grid(const mat4& vp, const vec3& cam_pos) {

    }

    void pick(int x, int y, int append = 0) {
        printf("p[icking!] %d %d \n", x, y);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer->render_target);
        glReadBuffer(GL_COLOR_ATTACHMENT2);

        unsigned int picked = 0;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &picked);
        printf("picked_id: %d\n", picked);
        selected = picked;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }


    //void render_debug(Player& player) {
    //    Shader* shader = Shader_Manager::get_shader(debug_shader);
    //    glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
    //    shader->set_mat4("projection", projection);
    //    glm::mat4 view = player.get_view_matrix();
    //    shader->set_mat4("view", view);

    //    if (editor_mode) {
    //        int half_width = scr_width / 2;
    //        int half_height = scr_height / 2;
    //        glViewport(0, half_height, half_width, half_height);
    //        debug_renderer.render(shader, projection, view);
    //        glViewport(0, 0, scr_width, scr_height);
    //    }
    //    else
    //        debug_renderer.render(shader, projection, view);

    //}

    //void render_scene_editor(Player& player, Scene& scene, float delta_time) {
    //    int half_width = scr_width / 2;
    //    int half_height = scr_height / 2;
    //    //float quadrant_aspect_ratio = (float)half_width / (float)half_height;

    //    glViewport(0, half_height, half_width, half_height); // Top-left quadrant
    //    render_scene(player, scene, delta_time);
    //    //render_gizmo(scene, player, half_width, half_height);
    //    render_hud_text(editor_viewports.scene.view_text);

    //    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //    // Top-Right
    //    glViewport(half_width, half_height, half_width, half_height);
    //    render_scene_ortho(player, scene, delta_time, editor_viewports.top);

    //    // Bottom-Left
    //    glViewport(0, 0, half_width, half_height);
    //    render_scene_ortho(player, scene, delta_time, editor_viewports.side);

    //    // Bottom-Right
    //    glViewport(half_width, 0, half_width, half_height);
    //    render_scene_ortho(player, scene, delta_time, editor_viewports.front);

    //    glViewport(0, 0, scr_width, scr_height);
    //    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //}

    void render_gizmo(Scene& scene, const mat4& view, const mat4& proj, ImVec2 image_min, ImVec2 image_size) {
        if (selected < 0 || selected > (scene.entities.size() - 1)) return;

        ImGuizmo::BeginFrame();
        // ImGui::SetNextWindowPos(ImVec2(0, 0));
        // ImGui::SetNextWindowSize(ImVec2(w, h));
        // ImGui::Begin("gizmode",
        //     nullptr,
        //     ImGuiWindowFlags_NoTitleBar |
        //     ImGuiWindowFlags_NoResize |
        //     ImGuiWindowFlags_NoMove |
        //     ImGuiWindowFlags_NoScrollbar |
        //     ImGuiWindowFlags_NoBackground);

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(image_min.x, image_min.y, image_size.x, image_size.y);

        ImGuizmo::OPERATION guizmo_op;
        // if (editor_viewports.scene.gizmo_mode == gizmo_modes::TRANSLATE)
            guizmo_op = ImGuizmo::OPERATION::TRANSLATE;
        // else if (editor_viewports.scene.gizmo_mode == gizmo_modes::ROTATE)
            // guizmo_op = ImGuizmo::OPERATION::ROTATE;
        // else if (editor_viewports.scene.gizmo_mode == gizmo_modes::SCALE)
            // guizmo_op = ImGuizmo::OPERATION::SCALE;
        // else
            // assert(false);

        // no snap
        bool smooth = false;//glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        float snap_value = 0.5f;
        if (guizmo_op == ImGuizmo::OPERATION::ROTATE)
            snap_value = 15.0f;
        float snap_values[3] = { snap_value, snap_value, snap_value };

        Entity& e = scene.entities[selected];
        glm::mat4 model = e.get_model_matrix();

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), guizmo_op, ImGuizmo::LOCAL, glm::value_ptr(model), nullptr, smooth ? nullptr : snap_values)) {
            glm::vec3 position, scale, rotation;
            Util::decompose(model, position, scale, rotation);
            
            if (e.physics_enabled) {
                Physics::set_body_position(scene.entities[selected].physics_id, position);
                Physics::set_body_rotation(scene.entities[selected].physics_id, glm::quat(rotation));
            }
            else {
                printf("%d\n", position.x);
                e.position = position;
                e.rotation = rotation;
                e.m_scale = scale;
            }
        }
        // ImGui::End();
    }

    //view_type_data* get_viewport_at_mouse(double xpos, double ypos) {
    //    if (!editor_mode) return nullptr;

    //    double half_width = scr_width / 2.0;
    //    double half_height = scr_height / 2.0;

    //    if (xpos < half_width && ypos < half_height) {
    //        return &editor_viewports.scene; // Top-Left
    //    }
    //    else if (xpos >= half_width && ypos < half_height) {
    //        return &editor_viewports.top; // Top-Right
    //    }
    //    else if (xpos < half_width && ypos >= half_height) {
    //        return &editor_viewports.side; // Bottom-Left
    //    }
    //    else {
    //        return &editor_viewports.front; // Bottom-Right
    //    }
    //}

    //void render_scene_ortho(Player& player, Scene& scene, float deltaTime, const view_type_data& view_data) {
    //    Shader* shader = Shader_Manager::get_shader(editor_shader);
    //    shader->use();

    //    int half_width = scr_width / 2;
    //    int half_height = scr_height / 2;

    //    float aspect_ratio = (float)half_width / (float)half_height;
    //    float ortho_size = view_data.get_ortho_size();

    //    glm::mat4 projection = glm::ortho(
    //        -ortho_size * aspect_ratio, ortho_size * aspect_ratio,  // left, right
    //        -ortho_size, ortho_size,                                // bottom, top
    //        0.1f, FAR_PLANE                                         // near, far
    //    );

    //    shader->set_mat4("projection", projection);

    //    glm::vec3 target_pos = view_data.get_target_position();
    //    glm::vec3 view_camera_pos = target_pos + view_data.get_camera_position();
    //    glm::vec3 up_vector = view_data.get_up_vector();

    //    glm::mat4 view = glm::lookAt(view_camera_pos, target_pos, up_vector);

    //    shader->set_mat4("view", view);
    //    //used_shader.set_vec3("view_position", view_camera_pos);

    //    for (Entity& entity : scene.entities) {
    //        glm::mat4 model = entity.get_model_matrix();
    //        shader->set_mat4("model", model);

    //        glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
    //        shader->set_mat3("normal_matrix", normal_matrix);

    //        entity.draw(shader);

    //        /*           if (entity.physics_enabled) {
    //                       Util::OBB collision_box = Physics::get_world_OBB(entity.physics_id);
    //                       debug_renderer.add_obb(collision_box, glm::vec3(0.0f, 1.0f, 0.0f));
    //                   }*/
    //    }

    //    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //    render_hud_text(view_data.view_text);
    //    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //}

    void mouse_button_callback(GLFWwindow* glfw_window, int button, int action, int mods);
    void mouse_callback(GLFWwindow* glfw_window, double xpos, double ypos);
    void scroll_callback(GLFWwindow* glfw_window, double xoffset, double yoffset);
    void key_callback(GLFWwindow* glfw_window, int key, int scancode, int action, int mods);
    void char_callback(GLFWwindow* glfw_window, uint32_t key);

    Camera camera = Camera(vec3(2.0f));
    bool cam_orbiting = false, cam_panning = false;

    // editor_viewports_struct editor_viewports;
    shader_handle editor_shader;
    std::vector<size_t> selected_entites;
    float outline_scale = 0.1f;
    Renderer* renderer;

    uint32_t selected = 0;

    bool first_draw = true;
};