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

            gruvbox();
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
                    gruvbox();
                if (ImGui::MenuItem("purpleish"))
                    purpleish();
                if (ImGui::MenuItem("dark"))
                    dark();
                if (ImGui::MenuItem("green"))
                    green();

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();

        asset_browser();
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

    void gruvbox() {
        auto &style = ImGui::GetStyle();
        style.ChildRounding = 0;
        style.GrabRounding = 0;
        style.FrameRounding = 0;
        style.PopupRounding = 0;
        style.ScrollbarRounding = 0;
        style.TabRounding = 0;
        style.WindowRounding = 0;
        style.FramePadding = {4, 4};

        style.WindowTitleAlign = {0.5, 0.5};

        ImVec4 *colors = ImGui::GetStyle().Colors;
        // Updated to use IM_COL32 for more precise colors and to add table colors (1.80 feature)
        colors[ImGuiCol_Text] = ImColor{IM_COL32(0xeb, 0xdb, 0xb2, 0xFF)};
        colors[ImGuiCol_TextDisabled] = ImColor{IM_COL32(0x92, 0x83, 0x74, 0xFF)};
        colors[ImGuiCol_WindowBg] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0xF0)};
        colors[ImGuiCol_ChildBg] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0xFF)};
        colors[ImGuiCol_PopupBg] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0xF0)};
        colors[ImGuiCol_Border] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0xFF)};
        colors[ImGuiCol_BorderShadow] = ImColor{0};
        colors[ImGuiCol_FrameBg] = ImColor{IM_COL32(0x3c, 0x38, 0x36, 0x90)};
        colors[ImGuiCol_FrameBgHovered] = ImColor{IM_COL32(0x50, 0x49, 0x45, 0xFF)};
        colors[ImGuiCol_FrameBgActive] = ImColor{IM_COL32(0x66, 0x5c, 0x54, 0xA8)};
        colors[ImGuiCol_TitleBg] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0xFF)};
        colors[ImGuiCol_TitleBgActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_TitleBgCollapsed] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0x9C)};
        colors[ImGuiCol_MenuBarBg] = ImColor{IM_COL32(0x28, 0x28, 0x28, 0xF0)};
        colors[ImGuiCol_ScrollbarBg] = ImColor{IM_COL32(0x00, 0x00, 0x00, 0x28)};
        colors[ImGuiCol_ScrollbarGrab] = ImColor{IM_COL32(0x3c, 0x38, 0x36, 0xFF)};
        colors[ImGuiCol_ScrollbarGrabHovered] = ImColor{IM_COL32(0x50, 0x49, 0x45, 0xFF)};
        colors[ImGuiCol_ScrollbarGrabActive] = ImColor{IM_COL32(0x66, 0x5c, 0x54, 0xFF)};
        colors[ImGuiCol_CheckMark] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0x9E)};
        colors[ImGuiCol_SliderGrab] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0x70)};
        colors[ImGuiCol_SliderGrabActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_Button] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0x66)};
        colors[ImGuiCol_ButtonHovered] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0x9E)};
        colors[ImGuiCol_ButtonActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_Header] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0.4F)};
        colors[ImGuiCol_HeaderHovered] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xCC)};
        colors[ImGuiCol_HeaderActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_Separator] = ImColor{IM_COL32(0x66, 0x5c, 0x54, 0.50f)};
        colors[ImGuiCol_SeparatorHovered] = ImColor{IM_COL32(0x50, 0x49, 0x45, 0.78f)};
        colors[ImGuiCol_SeparatorActive] = ImColor{IM_COL32(0x66, 0x5c, 0x54, 0xFF)};
        colors[ImGuiCol_ResizeGrip] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0x40)};
        colors[ImGuiCol_ResizeGripHovered] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xAA)};
        colors[ImGuiCol_ResizeGripActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xF2)};
        colors[ImGuiCol_Tab] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0xD8)};
        colors[ImGuiCol_TabHovered] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xCC)};
        colors[ImGuiCol_TabActive] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_TabUnfocused] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0.97f)};
        colors[ImGuiCol_TabUnfocusedActive] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0xFF)};
        colors[ImGuiCol_PlotLines] = ImColor{IM_COL32(0xd6, 0x5d, 0x0e, 0xFF)};
        colors[ImGuiCol_PlotLinesHovered] = ImColor{IM_COL32(0xfe, 0x80, 0x19, 0xFF)};
        colors[ImGuiCol_PlotHistogram] = ImColor{IM_COL32(0x98, 0x97, 0x1a, 0xFF)};
        colors[ImGuiCol_PlotHistogramHovered] = ImColor{IM_COL32(0xb8, 0xbb, 0x26, 0xFF)};
        colors[ImGuiCol_TextSelectedBg] = ImColor{IM_COL32(0x45, 0x85, 0x88, 0x59)};
        colors[ImGuiCol_DragDropTarget] = ImColor{IM_COL32(0x98, 0x97, 0x1a, 0.90f)};
        colors[ImGuiCol_TableHeaderBg] = ImColor{IM_COL32(0x38, 0x3c, 0x36, 0xFF)};
        colors[ImGuiCol_TableBorderStrong] = ImColor{IM_COL32(0x28, 0x28, 0x28, 0xFF)};
        colors[ImGuiCol_TableBorderLight] = ImColor{IM_COL32(0x38, 0x3c, 0x36, 0xFF)};
        colors[ImGuiCol_TableRowBg] = ImColor {IM_COL32(0x1d, 0x20, 0x21, 0xFF)};
        colors[ImGuiCol_TableRowBgAlt] = ImColor{IM_COL32(0x28, 0x28, 0x28, 0xFF)};
        colors[ImGuiCol_TextSelectedBg] = ImColor { IM_COL32(0x45, 0x85, 0x88, 0xF0) };
        colors[ImGuiCol_NavHighlight] = ImColor{IM_COL32(0x83, 0xa5, 0x98, 0xFF)};
        colors[ImGuiCol_NavWindowingHighlight] = ImColor{IM_COL32(0xfb, 0xf1, 0xc7, 0xB2)};
        colors[ImGuiCol_NavWindowingDimBg] = ImColor{IM_COL32(0x7c, 0x6f, 0x64, 0x33)};
        colors[ImGuiCol_ModalWindowDimBg] = ImColor{IM_COL32(0x1d, 0x20, 0x21, 0x59)};
    }

    void purpleish() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.3f;
        style.FrameRounding = 2.3f;
        style.ScrollbarRounding = 0;

        style.Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
        style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
        // style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
        style.Colors[ImGuiCol_Border]                = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
        style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.00f, 0.00f, 0.01f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
        style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
        style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
        style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
        style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.00f, 0.00f, 0.00f, 0.87f);
        style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
        style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
        style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
        // style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.1f, 0.1f, 0.1f, 0.99f);
        style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
        style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
        style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
        style.Colors[ImGuiCol_Button]                = ImVec4(0.48f, 0.72f, 0.89f, 0.49f);
        style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.50f, 0.69f, 0.99f, 0.68f);
        style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_Header]                = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
        style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
        style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
        // style.Colors[ImGuiCol_Column]                = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        // style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
        // style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
        style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
        style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
        // style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.50f, 0.50f, 0.90f, 0.50f);
        // style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.70f, 0.70f, 0.90f, 0.60f);
        // style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        style.Colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
        // style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    }

    void green() {
        ImVec4 *colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.14f, 0.16f, 0.11f, 0.52f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.27f, 0.30f, 0.23f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.34f, 0.26f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.32f, 0.24f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.30f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.23f, 0.27f, 0.21f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
        colors[ImGuiCol_Button] = ImVec4(0.29f, 0.34f, 0.26f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
        colors[ImGuiCol_Header] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.42f, 0.31f, 0.6f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
        colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.16f, 0.11f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.19f, 0.23f, 0.18f, 0.00f); // grip invis
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.54f, 0.57f, 0.51f, 0.78f);
        colors[ImGuiCol_TabActive] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.78f, 0.28f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.73f, 0.67f, 0.24f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        ImGuiStyle &style = ImGui::GetStyle();
        style.FrameBorderSize = 1.0f;
        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;
    }

    void dark() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.04f, 0.04f, 0.04f, 0.99f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.51f, 0.51f, 0.80f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.03f, 0.03f, 0.04f, 0.67f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.17f, 0.17f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.60f, 0.10f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.60f, 0.10f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.43f, 0.90f, 0.11f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.21f, 0.22f, 0.23f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.51f, 0.51f, 0.80f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.54f, 0.55f, 0.55f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.38f, 0.51f, 0.51f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.16f, 0.16f, 0.16f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.23f, 0.23f, 0.24f, 0.80f);
        colors[ImGuiCol_Tab] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
        colors[ImGuiCol_TabSelected] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
        colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.13f, 0.78f, 0.07f, 1.00f);
        colors[ImGuiCol_TabDimmed] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
        colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
        colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.10f, 0.60f, 0.12f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.14f, 0.87f, 0.05f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.30f, 0.60f, 0.10f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.23f, 0.78f, 0.02f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.46f, 0.47f, 0.46f, 0.06f);
        colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.78f, 0.69f, 0.69f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        style.WindowRounding = 4.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 3.0f;
        style.PopupRounding = 4.0f;
        style.TabRounding = 4.0f;
        style.WindowMenuButtonPosition = ImGuiDir_Right;
        style.ScrollbarSize = 10.0f;
        style.GrabMinSize = 10.0f;
        style.DockingSeparatorSize = 1.0f;
        style.SeparatorTextBorderSize = 2.0f;
    }

    //void render() {
    //    //renderer.render_scene_editor(player, scene, delta_time);

    //    render_selected_outlined(player, scene);

    //}

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
            for (const Mesh& m : Model_Manager::get_model_ind(selected_entity.model_id).m_meshes) {
                shader->set_mat4("mvp", vp * selected_entity.get_model_matrix() * m.transform);
                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, m.index_count, GL_UNSIGNED_INT, (void*)(m.base_index * sizeof(unsigned int)), 1, m.base_vertex, 0);
            }

            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDisable(GL_CULL_FACE);

            shader->set_float("scale", outline_scale);
            for (const Mesh& m : Model_Manager::get_model_ind(selected_entity.model_id).m_meshes) {
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