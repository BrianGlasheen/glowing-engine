#ifndef PLAYER_H
#define PLAYER_H

#include <memory>
#include <unordered_map>

#include "controller.h"
#include "controller_fps.h"
#include "controller_thirdperson.h"
#include "controller_plane.h"

#include "core/camera.h"
#include "core/physics.h"
#include "core/scene.h"
#include "player/weapon.h"

enum class ControllerType { FPS, THIRDPERSON, PLANE };

class Player {
public:
    float PLAYER_HEIGHT = 1.8f;

    Camera camera;
    Camera debug_camera;
    bool out_of_body = false;

    std::unordered_map<ControllerType, std::unique_ptr<Controller>> controllers;
    Controller* controller;

    std::unordered_map<Weapon_id, std::unique_ptr<Weapon>> weapons;
    Weapon* active_weapon;

    // Hands hands; // (   ͡°   ͜ʖ    ͡°   )

    // std::unique_ptr<Controller> activeController
    //  std::make_unique<Controller_plane>
    
    // player model
    // player model model matrix
    // glm::vec3 forward;
    float model_yaw = 0.0f;

    bool crouched = false;
    bool dashing = false;
    bool key_toggles[256] = {false};
    bool f1_was_pressed = false;

    // Model_ass wep;

    Player() : camera(glm::vec3(0.0f, PLAYER_HEIGHT, 0.0f)), debug_camera(glm::vec3(0.0f, PLAYER_HEIGHT, 0.0f)), controller() {
        controllers[ControllerType::FPS] = std::make_unique<Controller_fps>();
        controllers[ControllerType::THIRDPERSON] = std::make_unique<Controller_thirdperson>();
        //controllers[ControllerType::PLANE] = std::make_unique<Controller_plane>();
        controller = controllers[ControllerType::FPS].get();

        weapons[Weapon_id::M4A1] = std::make_unique<Weapon>(Weapon::M4A1());
        weapons[Weapon_id::GLOCK] = std::make_unique<Weapon>(Weapon::GLOCK());
        // weapons[Weapon_id::NONE] = std::make_unique<Weapon>(Weapon::NONE());
        active_weapon = weapons[Weapon_id::M4A1].get();
    }

    void controller_step(GLFWwindow* window, float deltaTime, Scene& scene) {
        poll_player(window, scene);
        if (out_of_body) {
            controller->process_input(window, deltaTime, scene, debug_camera, model_yaw);
            //controller->update_camera(camera, crouched, PLAYER_HEIGHT);
        }
        else {
            controller->process_input(window, deltaTime, scene, camera, model_yaw);
        }

        // yanked from process input function of player controller, todo refactor ?
        Weapon* current_weapon = active_weapon;
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            // reset reload timer
            active_weapon = weapons[Weapon_id::M4A1].get();
        }
        else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            // reset reload timer of held
            active_weapon = weapons[Weapon_id::GLOCK].get();
        }

        bool ads_active = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        bool firing = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool reload_requested = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        bool is_sprinting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        current_weapon->update(deltaTime, ads_active, firing, reload_requested, is_sprinting, camera.position, camera.front);
    }

    /*static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
        Player* player = static_cast<Player*>(glfwGetWindowUserPointer(window));
        if (player) {
            player->mouse_callback_impl(window, xpos, ypos);
        }
    }

    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
        Player* player = static_cast<Player*>(glfwGetWindowUserPointer(window));
        if (player) {
            player->scroll_callback_impl(window, xoffset, yoffset);
        }
    }

    static void char_callback(GLFWwindow* window, unsigned int key) {
        Player* player = static_cast<Player*>(glfwGetWindowUserPointer(window));
        if (player) {
            player->char_callback_impl(window, key);
        }
    }*/

    glm::mat4 get_model_matrix() {
        glm::mat4 model = glm::mat4(1.0f);
        //model = glm::translate(model, position);
        //model = glm::rotate(model, -glm::radians(model_yaw - 90), glm::vec3(0, 1, 0));
        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        return model;
    }

    void debug_hud() {
        Weapon* current_weapon = active_weapon;

        ImGui::SetNextWindowPos(ImVec2(200, 200));
        ImGui::SetNextWindowSize(ImVec2(200, 50));
        ImGui::Begin("Weapon", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings);

        ImGui::Text("%s", current_weapon->name.c_str());
        ImGui::SameLine(120);
        ImGui::Text("%s", current_weapon->get_ammo_string().c_str());

        if (current_weapon->is_reloading) {
            float progress = current_weapon->get_reload_progress();
            ImGui::ProgressBar(progress, ImVec2(-1, 10), "");
        }

        ImGui::End();
    }


    void mouse_callback(GLFWwindow* window, double xpos, double ypos) { // need these guys to pass camera
        //if (!key_toggles[(unsigned)'q'])
        if (!out_of_body && !key_toggles[(unsigned)'q'])
            controller->mouse_callback(window, camera, xpos, ypos, model_yaw);
        else // out of body
            controller->mouse_callback(window, debug_camera, xpos, ypos, model_yaw);
    }
    
    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
        controller->scroll_callback(window, camera, xoffset, yoffset);
    }

    void char_callback(GLFWwindow* window, unsigned int key) {
        key_toggles[key] = !key_toggles[key]; // set this key in our player key toggles
        process_player_toggles(window); // run through local stuff based on these keytoggles
        controller->char_callback(window, key); // set controller key toggles

        if (key == 'b')
            out_of_body = !out_of_body;
    }

    float get_camera_zoom() {
        if (out_of_body)
            return debug_camera.zoom;
        else
            return camera.zoom;
    }

    glm::mat4 get_view_matrix() {
        if (out_of_body)
            return debug_camera.get_view_matrix();
        else
            return camera.get_view_matrix();
    }

    glm::vec3 get_view_position() {
        if (out_of_body)
            return camera.position;
        else
            return debug_camera.position;
    }

private:
    // key toggle state that is more specific to the idea of a player than a controller
    // think noclip mode vs plane throttle
    void process_player_toggles(GLFWwindow* window) { 
        // T
        // TOGGLE WIREFRAME
        if (key_toggles[(unsigned)'t'])
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        // R
        // TOGGLE MOUSE
        if (!key_toggles[(unsigned)'q'])
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        if (key_toggles[(unsigned)'i']) {
            printf("[CONTROLLER] setting fps \n");
            controller = controllers[ControllerType::FPS].get();
            key_toggles[(unsigned)'i'] = false;
        }
        if (key_toggles[(unsigned)'o']) {
            printf("[CONTROLLER] setting third person\n");
            controller = controllers[ControllerType::THIRDPERSON].get();
            key_toggles[(unsigned)'o'] = false;
        }
        if (key_toggles[(unsigned)'p']) {
            printf("[CONTROLLER] setting plane\n");
            controller = controllers[ControllerType::PLANE].get();
            key_toggles[(unsigned)'p'] = false;
        }
    }
    // stuff we're polling for every frame that wouldnt be captured by a keycallback
    // maybe can breakup into 'meta player' vs physics/game state?
    void poll_player(GLFWwindow* window, Scene& scene) {
        // CTRL + C
        // CLOSE WINDOW
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        // ^ example meta player control ^
        //            vs
        // v    game state control      v
        crouched = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

        /*  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
              player_physics.position.x += camera.front.x * 15.0f;
              player_physics.position.y += camera.front.y * 8.0f;
              player_physics.position.z += camera.front.z * 15.0f;
              dashing = true;
          } else {
              dashing = false;
          }*/

        bool f1_is_pressed = (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS);
        if (f1_is_pressed && !f1_was_pressed) {
            // spawn glock
            Audio::play_audio("beep.wav", 0.1f);
            Entity e("glock", camera.position + camera.front * 5.0f, true, glm::vec3(1.0f), 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            scene.include(e);
        }

        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) {
            Audio::play_audio("beep.wav", 0.1f);
            Entity e("deagle", camera.position + camera.front * 5.0f, true, glm::vec3(0.05f), 1.0f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            scene.include(e);
        }

        if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS) {
            Audio::play_audio("beep.wav", 0.1f);
            Entity e("sword", camera.position + camera.front * 5.0f, true, glm::vec3(1.0f), 0.1f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            scene.include(e);
        }

        if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) {
            Audio::play_audio("beep.wav", 0.1f);
            Entity e("sword_ice", camera.position + camera.front * 5.0f, true, glm::vec3(1.0f), 0.1f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            scene.include(e);
        }

        if (glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS) {
            Audio::play_audio("beep.wav", 0.1f);
            Entity e("link", camera.position + camera.front * 5.0f, true, glm::vec3(1.0f), 0.1f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            scene.include(e);
        }

        if (glfwGetKey(window, GLFW_KEY_F7) == GLFW_PRESS) {
            Audio::play_audio("beep.wav", 0.1f);
            Entity e("fuzziebox", camera.position + camera.front * 5.0f, true, glm::vec3(1.0f), 0.5f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            scene.include(e);
        }

        if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) {
            Physics::optimize_broad_phase();
        }
    }
};
#endif
