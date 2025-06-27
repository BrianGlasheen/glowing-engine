#ifndef CONTROLLER_FPS_H
#define CONTROLLER_FPS_H

#include "GLFW/glfw3.h"
#include <glm/glm.hpp>

#include "core/camera.h"
#include "core/scene.h"

class Controller_fps : public Controller {
public:
    float lastX = 0.0f;
    float lastY = 0.0f;
         
    Controller_fps() = default;

    // glm::vec3 wep_pos = glm::vec3(0.6f, -0.5f, -1.6f);  M4A1
    // glm::vec3 min_pos = wep_pos;
    // glm::vec3 ads_pos = glm::vec3(-0.0001f, -.45f, -1.2f);
    // glm::vec3 wep_rot = glm::vec3(0.0f);
    // float ads_speed = 20.0f;
    // int holding = 0;
    // int rounds_per_min = 700;
    // float weapon_sound_cooldown = 0.07f; 
    // float last_shot_time = 0.0f; 
    
    bool key_toggles[256] = {false};

    virtual void mouse_callback(GLFWwindow* window, Camera& camera, double xpos, double ypos, float& model_yaw) override {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
        lastX = xpos;
        lastY = ypos;
        camera.process_mouse_movement(xoffset, yoffset); // todo change to controller processess
    }

    virtual void scroll_callback(GLFWwindow* window, Camera& camera, double xoffset, double yoffset) override {
        camera.process_mouse_scroll(static_cast<float>(yoffset));
    }

    virtual void char_callback(GLFWwindow* window, uint32_t key) override{
        key_toggles[key] = !key_toggles[key];
    }

    virtual void process_input(GLFWwindow* window, float deltaTime, Scene& scene, Camera& camera, float& model_yaw) override {
        //// Check for sprinting
        glm::vec3 movement(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            movement.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            movement.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            movement.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            movement.x += 1.0f;

        //// Normalize movement vector if the player is moving diagonally
        //if (glm::length(movement) > 0.0f)
        //    movement = glm::normalize(movement);
        //    
        //bool is_sprinting = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && movement.z > 0.0f;
        //
        //// Update the weapon with all inputs
        //current_weapon->update(deltaTime, ads_active, firing, reload_requested, is_sprinting);
        
        // Handle jumping
        //if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && player_physics.isOnGround) {
        //    player_physics.velocity.y = JUMP_FORCE;
        //    player_physics.isOnGround = false;
        //}

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera.position += glm::vec3(0.0f, 1.0f, 0.0f);
            /*player_physics.velocity.y = JUMP_FORCE;
            player_physics.isOnGround = false;*/
        }

        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            camera.position -= glm::vec3(0.0f, 1.0f, 0.0f);
            /*player_physics.velocity.y = JUMP_FORCE;
            player_physics.isOnGround = false;*/
        }
        

        // Convert camera-relative movement to world space
        glm::vec3 forward = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
        glm::vec3 right = glm::normalize(glm::cross(forward, camera.world_up));
        glm::vec3 acceleration = forward * movement.z + right * movement.x;

        camera.position += acceleration;
        
        // Apply sprint boost if sprinting
        //float speed_multiplier = is_sprinting ? 1.5f : 1.0f;
        //acceleration *= ACCELERATION * deltaTime * speed_multiplier;
        
        // Apply acceleration
        //player_physics.velocity.x += acceleration.x;
        //player_physics.velocity.z += acceleration.z;
        //
        //// Apply friction when on ground
        //if (player_physics.isOnGround) {
        //    player_physics.velocity.x *= FRICTION;
        //    player_physics.velocity.z *= FRICTION;
        //}
        //
        //// Limit horizontal velocity
        //float horizontal_speed = glm::length(glm::vec2(player_physics.velocity.x, player_physics.velocity.z));
        //if (horizontal_speed > MAX_VELOCITY * speed_multiplier) {
        //    float scale = MAX_VELOCITY * speed_multiplier / horizontal_speed;
        //    player_physics.velocity.x *= scale;
        //    player_physics.velocity.z *= scale;
        //}
        
        model_yaw = camera.yaw;
    }
    
    virtual void update_camera(Camera& camera, bool crouched, float player_height) override {
    }

    virtual void draw_hud(Shader& shader) const override {
        //if (!active_weapon) return;
        
        //active_weapon->model.draw(shader);
        
        // if (hands_model) {
        // }
    }

    virtual glm::vec3 get_weapon_position() const override {
        //return active_weapon->wep_pos;
        return glm::vec3(0.0f);
    }
    virtual glm::vec3 get_weapon_rotation() const {
        //return active_weapon->wep_rot;
        return glm::vec3(0.0f);
    }
    
    virtual void debug_hud(ImGuiIO& io) override {
        //Weapon* current_weapon = active_weapon;

        //ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 210, io.DisplaySize.y - 60));
        //ImGui::SetNextWindowSize(ImVec2(200, 50));
        //ImGui::Begin("Weapon", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        //    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        //    ImGuiWindowFlags_NoSavedSettings);

        //ImGui::Text("%s", current_weapon->name.c_str());
        //ImGui::SameLine(120);
        //ImGui::Text("%s", current_weapon->get_ammo_string().c_str());

        //if (current_weapon->is_reloading) {
        //    float progress = current_weapon->get_reload_progress();
        //    ImGui::ProgressBar(progress, ImVec2(-1, 10), "");
        //}

        //ImGui::End();
    }
};
#endif
