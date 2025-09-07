#pragma once

#include "core/camera.h"
#include "core/scene.h"

#include "glm/glm.hpp"

namespace FPS_Controller {

    void mouse_callback(GLFWwindow* window, Camera& camera, double xpos, double ypos, float& model_yaw) {
        camera.process_mouse_movement(xpos, ypos);
        // add weapon logic?
    }

    void scroll_callback(GLFWwindow* window, Camera& camera, double xoffset, double yoffset) {
        camera.process_mouse_scroll(static_cast<float>(yoffset));
    }

    void char_callback(GLFWwindow* window, uint32_t key) {
        //key_toggles[key] = !key_toggles[key];
    }

    void process_input(GLFWwindow* window, float deltaTime, Scene& scene, Camera& camera, float& model_yaw, JPH::BodyID physics_id) {
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

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            movement *= 10.0f;
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
        glm::vec3 currentVelocity = Physics::get_body_velocity(physics_id);
        glm::vec3 newVelocity(movement.x, currentVelocity.y, movement.z);

        Physics::set_body_velocity(physics_id, newVelocity);

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            //camera.position += glm::vec3(0.0f, 1.0f, 0.0f);

            JPH::Vec3 jumpVelocity(0, 150.0f, 0); // jump impulse
            Physics::add_impulse(physics_id, jumpVelocity);
            /*player_physics.velocity.y = JUMP_FORCE;
            player_physics.isOnGround = false;*/
        }

        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            //camera.position -= glm::vec3(0.0f, 1.0f, 0.0f);
            /*player_physics.velocity.y = JUMP_FORCE;
            player_physics.isOnGround = false;*/
        }
        

        // Convert camera-relative movement to world space
        glm::vec3 forward = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
        glm::vec3 right = glm::normalize(glm::cross(forward, camera.world_up));
        glm::vec3 acceleration = forward * movement.z + right * movement.x;

        //camera.position += acceleration;
        camera.position = Physics::get_body_position(physics_id) + glm::vec3(0.0f, 1.5f, 0.0f);

        //printf("%f, %f, %f\n", camera.position.x, camera.position.y, camera.position.z);
        
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
    
    void update_camera(Camera& camera, bool crouched, float player_height) {
    }

    void draw_hud(Shader& shader) {
        //if (!active_weapon) return;
        
        //active_weapon->model.draw(shader);
        
        // if (hands_model) {
        // }
    }

    glm::vec3 get_weapon_position() {
        //return active_weapon->wep_pos;
        return glm::vec3(0.0f);
    }
    glm::vec3 get_weapon_rotation() {
        //return active_weapon->wep_rot;
        return glm::vec3(0.0f);
    }

    void deactivate() { // do something to physics state when switching to another controller

    }

    void activate() { // same ^

    }
    
    //void debug_hud(ImGuiIO& io) {
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
    //}
};
