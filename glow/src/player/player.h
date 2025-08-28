#pragma once

#include <unordered_map>

#include "core/camera.h"
#include "core/physics.h"
#include "core/scene.h"
#include "player/weapon.h"
#include "core/renderer.h"

#include <Jolt/Physics/Character/Character.h>

enum class ControllerType { FPS, THIRDPERSON, PLANE, NUM_CONTROLLER };

class Player {
public:
    float player_height = 1.8f;

    Camera camera;
    Camera debug_camera;
    bool out_of_body = false;

    ControllerType active_controller;

    std::unordered_map<Weapon_Id, std::unique_ptr<Weapon>> weapons;
    Weapon* active_weapon;

    JPH::BodyID physics_id;

    //Character			mAnimatedCharacter;

    // Player input
    //Vec3					mControlInput = Vec3::sZero();
    //bool					mJump = false;
    //bool					mWasJump = false;
    //bool					mSwitchStance = false;
    //bool					mWasSwitchStance = false;

    // Hands hands; // (   ͡°   ͜ʖ    ͡°   )

    // player model
    // glm::vec3 forward; player fwd vs camera
    float model_yaw = 0.0f;

    bool crouched = false;
    bool dashing = false;
    bool key_toggles[256] = {false};
    bool f1_was_pressed = false;

    // Model wep;

    Player();
    void init();

    void controller_step(GLFWwindow* window, float deltaTime, Scene& scene);

    void submit_animation_items();
    void submit_render_items(Renderer& renderer);

    void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    void char_callback(GLFWwindow* window, uint32_t key);

    glm::mat4 get_model_matrix();
    float get_camera_zoom() const;
    glm::mat4 get_body_view_matrix() const;
    glm::mat4 get_debug_view_matrix() const;
    glm::mat4 get_view_matrix() const;
    glm::vec3 get_view_position() const;
    
    void debug_hud();

private:
    // key toggle state that is more specific to the idea of a player than a controller
    // think noclip mode vs plane throttle
    void process_player_toggles(GLFWwindow* window);
    // stuff we're polling for every frame that wouldnt be captured by a keycallback
    // maybe can breakup into 'meta player' vs physics/game state?
    void poll_player(GLFWwindow* window, Scene& scene);

    void move_debug_camera(GLFWwindow* window, Camera& camera);
};
