#pragma once

#include <vector>

// #include <GLFW/glfw3.h>
#include "core/opengl.h"

#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/quaternion.hpp>

#include "asset/shader_manager.h"
#include "asset/font.h"
#include "asset/text.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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
    int init() {
        // make viewports
        // editor_viewports.top.init_text("top------"); // pad to 9 xD
        // editor_viewports.front.init_text("front----");
        // editor_viewports.side.init_text("side-----");
        // editor_viewports.scene.init_text("scene----");

        Shader_Manager::load_from_name("editor");

        return 0;
    }

    void show() {
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

            ImGui::DockBuilderDockWindow("Scene", left);
            ImGui::DockBuilderDockWindow("Renderer", left);
            ImGui::DockBuilderDockWindow("Inspector", right);
            ImGui::DockBuilderDockWindow("Preview", center);
            ImGui::DockBuilderDockWindow("Console", bottom);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    // todo exit
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();
    }

    //void render() {
    //    //renderer.render_scene_editor(player, scene, delta_time);

    //    render_selected_outlined(player, scene);

    //}

    //void render_selected_outlined(Player& player, const Scene& scene) {

    //    for (size_t selected : selected_entites) {
    //        glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //        Entity selected_entity = scene.entities[selected];

    //        glStencilFunc(GL_ALWAYS, 1, 0xFF);
    //        glStencilMask(0xFF);
    //        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    //        Shader* shader = Shader_Manager::get_shader(outline_shader);
    //        shader->use();

    //        glm::mat4 projection = glm::perspective(glm::radians(player.get_camera_zoom()), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
    //        shader->set_mat4("projection", projection);
    //        shader->set_mat4("view", player.get_view_matrix());

    //        glm::mat4 model = selected_entity.get_model_matrix();
    //        shader->set_mat4("model", model);
    //        //glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
    //        //shader->set_mat3("normal_matrix", normal_matrix);
    //        shader->set_float("scale", 0.0);
    //        selected_entity.draw(shader);

    //        shader->set_vec3("color", Util::orange);
    //        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    //        glStencilMask(0x00);
    //        //glDisable(GL_DEPTH_TEST);
    //        //glEnable(GL_DEPTH_TEST);
    //        //glCullFace(GL_FRONT);
    //        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    //        //shader->set_mat4("model", glm::scale(model, glm::vec3(outline_scale)));
    //        shader->set_float("scale", outline_scale);
    //        selected_entity.draw(shader);

    //        glStencilMask(0xFF);
    //        glStencilFunc(GL_ALWAYS, 0, 0xFF);
    //        //glCullFace(GL_BACK);
    //        glEnable(GL_DEPTH_TEST);
    //    }

    //}


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

        //void render_gizmo(const Scene& scene, const Player& player) {
    //    if (editor_viewports.scene.gizmo_mode != gizmo_modes::NONE && target_entity != -1) {
    //        float w = scr_width / 2;
    //        float h = scr_height / 2;

    //        ImGuizmo::BeginFrame();

    //        ImGui::SetNextWindowPos(ImVec2(0, 0));
    //        ImGui::SetNextWindowSize(ImVec2(w, h));
    //        ImGui::Begin("gizmode",
    //            nullptr,
    //            ImGuiWindowFlags_NoTitleBar |
    //            ImGuiWindowFlags_NoResize |
    //            ImGuiWindowFlags_NoMove |
    //            ImGuiWindowFlags_NoScrollbar |
    //            ImGuiWindowFlags_NoBackground);

    //        ImGuizmo::SetOrthographic(false);
    //        ImGuizmo::SetDrawlist();

    //        ImGuizmo::SetRect(0.0f, 0.0f, w, h);

    //        glm::mat4 projection = glm::perspective(glm::radians(player.camera.zoom), (float)scr_width / (float)scr_height, 0.1f, FAR_PLANE);
    //        glm::mat4 view = player.camera.get_view_matrix();
    //        glm::mat4 model = scene.entities[target_entity].get_model_matrix();

    //        ImGuizmo::OPERATION guizmo_op;
    //        if (editor_viewports.scene.gizmo_mode == gizmo_modes::TRANSLATE)
    //            guizmo_op = ImGuizmo::OPERATION::TRANSLATE;
    //        else if (editor_viewports.scene.gizmo_mode == gizmo_modes::ROTATE)
    //            guizmo_op = ImGuizmo::OPERATION::ROTATE;
    //        else if (editor_viewports.scene.gizmo_mode == gizmo_modes::SCALE)
    //            guizmo_op = ImGuizmo::OPERATION::SCALE;
    //        else
    //            assert(false);

    //        // no snap
    //        bool smooth = false;//glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    //        float snap_value = 0.5f;
    //        if (guizmo_op == ImGuizmo::OPERATION::ROTATE)
    //            snap_value = 15.0f;
    //        float snap_values[3] = { snap_value, snap_value, snap_value };

    //        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), guizmo_op, ImGuizmo::LOCAL,
    //            glm::value_ptr(model), nullptr, smooth ? nullptr : snap_values)) {

    //            glm::vec3 position, scale, rotation;
    //            Util::decompose(model, position, scale, rotation);

    //            // todo change
    //            Physics::set_body_position(scene.entities[target_entity].physics_id, position);
    //            Physics::set_body_rotation(scene.entities[target_entity].physics_id, glm::quat(rotation));
    //        }
    //        ImGui::End();
    //    }
    //}

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




    // editor_viewports_struct editor_viewports;
    shader_handle editor_shader;
    std::vector<size_t> selected_entites;
    float outline_scale = 0.1f;
};