#pragma once

// #include <glad/glad.h>
// #include <GLFW/glfw3.h>
#include "core/opengl.h"

#include <glm/glm.hpp>

#include "renderer.h"
#include "player/player.h"
#include "editor.h"

class Window {
public:

	Window() = default;
    ~Window();

    int init(int w, int h, const char* title);
    void shutdown();

    void present();

    float get_time();
    bool open();
    glm::vec2 get_window_size();
    GLFWwindow* get_window();

    void sync_callbacks(Player& p, Renderer& r, Editor& e, bool& editor);

private:

    GLFWwindow* window;
    int width, height;
    int target_width, target_height;

    Player* player;
    Renderer* renderer;
    Editor* editor;
    bool* editor_mode;

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    //static void window_refresh_callback(GLFWwindow* window);
    static void static_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void static_mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void static_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void static_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void static_char_callback(GLFWwindow* window, uint32_t key);
};