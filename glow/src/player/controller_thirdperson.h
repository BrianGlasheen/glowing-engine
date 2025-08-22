#ifndef CONTROLLER_THIRDPERSON_H
#define CONTROLLER_THIRDPERSON_H

#include "glm/glm.hpp"

#include <core/camera.h>
#include <core/scene.h>

class Controller_thirdperson : public Controller {
public:
    float lastX = 0.0f;
    float lastY = 0.0f;
    // Character controls / state
    glm::vec3 character_rotation = glm::vec3(0.0f);
    
    // WoW-style camera states
    bool right_mouse_pressed = false;
    bool left_mouse_pressed = false;
    float character_yaw = 0.0f;    // Character's facing direction
    float model_target_yaw = 0.0f; // Target rotation for the model when moving
    float movement_angle = 0.0f;   // Direction of movement
    
    // Third-person camera constants
    float camera_distance = 5.0f;  // Modified to be variable for zooming
    const float CAMERA_HEIGHT = 1.5f;
    const float CAMERA_OFFSET = 0.0f;  // Horizontal offset (0 for centered)
    const float MIN_CAMERA_DISTANCE = 2.0f;
    const float MAX_CAMERA_DISTANCE = 1000.0f;
    const float CAMERA_SMOOTHING = 5.0f;
    const float CHARACTER_ROTATION_SPEED = 15.0f;
    const float MODEL_ROTATION_SPEED = 10.0f;  // How fast the model turns to face movement direction
    
    // Camera collision detection
    bool enable_camera_collision = true;
    float current_camera_distance = camera_distance;
    float target_camera_distance = camera_distance;
    
    bool key_toggles[256] = {false};
    
    virtual void mouse_callback(GLFWwindow* window, Camera& camera, double xpos, double ypos, float& model_yaw) override {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
        lastX = xpos;
        lastY = ypos;
        
        // Update current states
        right_mouse_pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        left_mouse_pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        
        if (right_mouse_pressed || left_mouse_pressed) {
            camera.process_mouse_movement(xoffset, yoffset);
            
            // Only when right mouse is pressed, update character yaw to match camera
            if (right_mouse_pressed) {
                character_yaw = -camera.yaw - 90;
                model_yaw = -character_yaw - 90; // Update model to face same direction as character
            }
        }
    }
    
    virtual void scroll_callback(GLFWwindow* window, Camera& camera, double xoffset, double yoffset) override {
        // Adjust camera distance with scroll (WoW-style zoom)
        target_camera_distance -= static_cast<float>(yoffset) * 2.0f;
        target_camera_distance = glm::clamp(target_camera_distance, MIN_CAMERA_DISTANCE, MAX_CAMERA_DISTANCE);
        
        // We'll still use the camera's zoom for FOV adjustments if needed
        // camera.process_mouse_scroll(yoffset);
    }
    
    virtual void char_callback(GLFWwindow* window, uint32_t key) override {
        key_toggles[key] = !key_toggles[key];
    }
    
    virtual void process_input(GLFWwindow* window, float deltaTime, Scene& scene, Camera& camera, float& model_yaw) override {
        //glm::vec3 movement(0.0f);
        //if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        //    movement.z -= 1.0f;
        //if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        //    movement.z += 0.5f; // Half speed backward
        //if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        //    movement.x -= 1.0f;  // Strafe left
        //if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        //    movement.x += 1.0f;  // Strafe right

        //// Normalize movement vector for diagonal movement
        //bool is_moving = glm::length(movement) > 0.0f;
        //if (is_moving)
        //    movement = glm::normalize(movement);

        //// Handle jumping
        //if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        //    //player_physics.velocity.y = JUMP_FORCE;
        //    //player_physics.isOnGround = false;
        //}

        //// Convert movement direction to be relative to character facing direction (for WoW style)
        //float character_yaw_radians = glm::radians(character_yaw);
        //glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), character_yaw_radians, glm::vec3(0.0f, 1.0f, 0.0f));
        //// Forward vector is based on character orientation
        //glm::vec3 forward = glm::vec3(rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)); // Note: negative Z is forward
        //glm::vec3 right = glm::vec3(rotationMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
        //// Calculate world space movement vector
        //glm::vec3 moveDir = forward * -movement.z + right * movement.x; // Note the negative for z
        //
        //// If moving, calculate the angle of movement for model orientation
        //if (is_moving) {
        //    // Calculate the movement angle in world space
        //    movement_angle = glm::degrees(atan2(moveDir.x, moveDir.z));
        //    // When right-click is held, character and model should face camera direction
        //    if (!right_mouse_pressed) {
        //        model_target_yaw = -movement_angle + 90;
        //        // Smoothly interpolate model yaw towards target
        //        float yaw_diff = model_target_yaw - model_yaw;
        //        // Handle angle wrap-around
        //        if (yaw_diff > 180.0f) yaw_diff -= 360.0f;
        //        if (yaw_diff < -180.0f) yaw_diff += 360.0f;
        //        // Apply smooth rotation to model_yaw
        //        model_yaw += yaw_diff * MODEL_ROTATION_SPEED * deltaTime;
        //        // Keep model_yaw in range [0, 360)
        //        if (model_yaw < 0.0f) model_yaw += 360.0f;
        //        if (model_yaw >= 360.0f) model_yaw -= 360.0f;
        //    }
        //}
        // Apply acceleration
        //glm::vec3 acceleration = moveDir * ACCELERATION * deltaTime;
        //player_physics.velocity.x += acceleration.x;
        //player_physics.velocity.z += acceleration.z;
        
        // Apply friction when on ground
        //if (player_physics.isOnGround) {
            //player_physics.velocity.x *= FRICTION;
            //player_physics.velocity.z *= FRICTION;
        //}
        
        // Limit horizontal velocity
        //float horizontal_speed = glm::length(glm::vec2(player_physics.velocity.x, player_physics.velocity.z));
        //if (horizontal_speed > MAX_VELOCITY) {
            //float scale = MAX_VELOCITY / horizontal_speed;
            //player_physics.velocity.x *= scale;
            //player_physics.velocity.z *= scale;
        //}
        
        // Smoothly interpolate camera distance for zoom
        //current_camera_distance = glm::mix(current_camera_distance, target_camera_distance, deltaTime * CAMERA_SMOOTHING);
    }
    
    virtual void update_camera(Camera& camera, bool crouched, float player_height) override {
        // Use camera's existing front vector (already normalized and calculated from yaw/pitch)
        //glm::vec3 camera_dir = -camera.front; // Negate front to get direction from player to camera
        
        // Position camera at the desired distance behind the player
        //glm::vec3 target_pos = player_physics.position;
        //target_pos.y += player_height; // Adjust for character height
        //
        //glm::vec3 camera_pos = target_pos;
        //camera_pos -= camera.front * current_camera_distance; // Use camera front for distance calculation
        //
        //// Camera collision detection
        //if (enable_camera_collision) {
        //    // Simple example: prevent camera from going below ground
        //    if (camera_pos.y < FLOOR_Y + 0.5f) {
        //        camera_pos.y = FLOOR_Y + 0.5f;
        //    }
        //}
        //
        //// Update camera position
        //camera.position = camera_pos;
        //
        //// No need to update front vector here as it's determined by yaw/pitch
        //// which are already being updated in mouse_callback
        //
        //// Update right and up vectors
        //camera.update_camera_vectors();
    }
    
    virtual glm::vec3 get_weapon_position() const override {
        // return glm::vec3(-0.2f, -0.1f, 0.5f);
        return glm::vec3(-0.2f, -99999.1f, 0.5f);
    }
};
#endif