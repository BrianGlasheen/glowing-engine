#include "window.h"

#include "asset/shader_manager.h"

Window::~Window() {
    shutdown();
}

int Window::init(int w, int h, const char* title) {
    width = w;
    height = h;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_STENCIL_BITS, 8);

    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }
    else {
        printf("~~~ cool quote here ~~~\n");
    }

    glfwMakeContextCurrent(window); // idk
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // window can own its resize callback, all the others will affect renderer/player state in some way

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSwapInterval(1);
    //glfwSwapInterval(0);
    
    return 0;
}

void Window::shutdown() {
    glfwTerminate();
}


void Window::present() {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

// 
// get info
//
float Window::get_time() {
    return static_cast<float>(glfwGetTime());
}

bool Window::open() {
    return !glfwWindowShouldClose(window);
}

glm::vec2 Window::get_window_size() {
    return glm::vec2(width, height);
}

GLFWwindow* Window::get_window() {
    return window;
}

//
// low use functions
//
void Window::sync_callbacks(Player& p, Renderer& r, Editor& e, bool& editor) {
    player = &p;
    renderer = &r;
    editor = &e;
    editor_mode = &editor;

    //glfwSetWindowUserPointer(window, &player);
    glfwSetCursorPosCallback(window, Window::static_mouse_callback);
    glfwSetMouseButtonCallback(window, Window::static_mouse_button_callback);
    glfwSetScrollCallback(window, Window::static_scroll_callback);
    glfwSetKeyCallback(window, Window::static_key_callback);
    glfwSetCharCallback(window, Window::static_char_callback);
    //glfwSetWindowRefreshCallback(window, Window::window_refresh_callback);
}

//
// private functions
//
void Window::framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height) {
    Window* this_window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    //this_window->renderer->resize(width, height);
    //this_window->target_width = width;
    //this_window->target_height = height;

    this_window->width = width;
    this_window->height = height;

    printf("w: %d, h: %d\n", width, height);
    this_window->renderer->resize(this_window->width, this_window->height);

    //glViewport(0, 0, width, height);
    //renderer->scr_width = width;
    //renderer->scr_height = height;
}

void Window::static_mouse_button_callback(GLFWwindow* glfw_window, int button, int action, int mods) {
    Window* this_window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    //if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && (this_window->target_width != this_window->width || this_window->target_height != this_window->target_width)) {
    //    this_window->width = this_window->target_width;
    //    this_window->height = this_window->target_height;
    //    this_window->renderer->resize(this_window->width, this_window->height);
    //}
    // 
    /*if (button == GLFW_MOUSE_BUTTON_LEFT)
        this_window->renderer->bone++;
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
        this_window->renderer->bone--;*/
// 
    //if (this_window->renderer->editor_mode) {
    //    // Handle editor mode mouse button input
    //    if (button == GLFW_MOUSE_BUTTON_MIDDLE ||
    //        (button == GLFW_MOUSE_BUTTON_LEFT && (mods & GLFW_MOD_SHIFT))) {

    //        double xpos, ypos;
    //        glfwGetCursorPos(glfw_window, &xpos, &ypos);

    //        view_type_data* active_viewport = this_window->renderer->get_viewport_at_mouse(xpos, ypos);
    //        if (active_viewport && active_viewport->type != view_type::SCENE) {
    //            if (action == GLFW_PRESS) {
    //                active_viewport->start_pan(glm::vec2(xpos, ypos));
    //            }
    //            else if (action == GLFW_RELEASE) {
    //                active_viewport->stop_pan();
    //            }
    //        }
    //    }
    //}
    //else {
    //    
    //}
}

//void Window::window_refresh_callback(GLFWwindow* glfw_window) {
//    Window* this_window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
//
//    if (this_window->target_width != this_window->width || this_window->target_height != this_window->height) {
//
//        this_window->width = this_window->target_width;
//        this_window->height = this_window->target_height;
//        this_window->renderer->resize(this_window->width, this_window->height);
//    }
//}

void Window::static_mouse_callback(GLFWwindow* glfw_window, double xpos, double ypos) {
    Window* this_window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    //if (renderer && renderer->current_player) {
        //if (this_window->renderer->editor_mode) {

        //    view_type_data* active_viewport = this_window->renderer->get_viewport_at_mouse(xpos, ypos);
        //    if (active_viewport && active_viewport->is_panning) {
        //        glm::vec2 current_mouse(xpos, ypos);
        //        glm::vec2 mouse_delta = current_mouse - active_viewport->last_mouse_pos;
        //        active_viewport->handle_pan(mouse_delta);
        //        active_viewport->last_mouse_pos = current_mouse;
        //    }

    //        // Editor mode input handling
    //        double half_width = renderer->scr_width / 2.0;
    //        double half_height = renderer->scr_height / 2.0;

    //        if (xpos < half_width && ypos < half_height) {
    //            // std::cout << "Top-Left: (" << xpos << ", " << ypos << ")" << std::endl;
    //        }
    //        else if (xpos >= half_width && ypos < half_height) {
    //            // std::cout << "Top-Right: (" << xpos << ", " << ypos << ")" << std::endl;
    //        }
    //        else if (xpos < half_width && ypos >= half_height) {
    //            // std::cout << "Bottom-Left: (" << xpos << ", " << ypos << ")" << std::endl;
    //        }
    //        else {
    //            // std::cout << "Bottom-Right: (" << xpos << ", " << ypos << ")" << std::endl;
    //        }
        //}
        //else {
            this_window->player->mouse_callback(glfw_window, xpos, ypos);
        //}
}

void Window::static_scroll_callback(GLFWwindow* glfw_window, double xoffset, double yoffset) {
    Window* this_window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    //if (this_window->renderer->editor_mode) {

    //    double xpos, ypos;
    //    glfwGetCursorPos(glfw_window, &xpos, &ypos);

    //    view_type_data* active_viewport = this_window->renderer->get_viewport_at_mouse(xpos, ypos);
    //    if (active_viewport) {
    //        active_viewport->handle_zoom(static_cast<float>(-yoffset));
    //    }
    //}
    //else {
        this_window->player->scroll_callback(glfw_window, xoffset, yoffset);
    //}
}

void Window::static_key_callback(GLFWwindow* glfw_window, int key, int scancode, int action, int mods) {
    Window* this_window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    // exit
    if (key == GLFW_KEY_C && (mods & GLFW_MOD_CONTROL))
        glfwSetWindowShouldClose(glfw_window, true);

    Renderer* renderer = this_window->renderer;
    if (renderer) {
        //if (renderer->editor_mode) {

            //if (key == GLFW_KEY_RIGHT)
            //    renderer->target_entity = (renderer->target_entity + 1);

            //if (key == GLFW_KEY_LEFT)
            //    renderer->target_entity -= (renderer->target_entity > 0 ? 1 : 0);
        //}
        //else {
            // Game mode character input handling (e.g., for console, chat)
            // You might have a process_char method in Player or a separateUIhandler
            //renderer->current_player->char_callback(window, key); /Placeholder assuming this method exists
        //}
    }
}

void Window::static_char_callback(GLFWwindow* glfw_window, uint32_t key) {
    Window* this_window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

    if (key == 'm') {
        *this_window->editor_mode = !(*this_window->editor_mode);

        if (*this_window->editor_mode) 
            glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
            glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (key == 't')
        this_window->renderer->terrain_draw_type = (this_window->renderer->terrain_draw_type + 1) % 3;

    if (key == '\'')
        Shader_Manager::hot_reload_all();

    if (*this_window->editor_mode)
        this_window->editor->char_callback(glfw_window, key);
    else
        this_window->player->char_callback(glfw_window, key);
}