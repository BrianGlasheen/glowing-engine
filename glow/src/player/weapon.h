#pragma once

#include <string>

#include "glm/glm.hpp"

#include <core/audio.h>
#include <core/physics.h>
#include "asset/model_manager.h"

enum class Weapon_Id {
    GLOCK,
    //KNIFE,
    //SHOTGUN,
    //SNIPER,
    BOW,
    NONE
};

class Weapon {
public:
    float wep_scale;
    glm::vec3 wep_pos;      // Current position
    glm::vec3 wep_rot;      // Rotation
    
    //glm::vec3 min_pos;      // Hip fire position
    //glm::vec3 ads_pos;      // Aim down sight position
    //glm::vec3 sprint_pos;   // Sprinting position
    //float ads_speed;        // ADS transition speed
    
    // Gameplay properties
    Weapon_Id id;           // Weapon identifier
    std::string name;       // Display name
    std::string sound_file; // Sound file to play when firing
    std::string sound_dry;
    float sound_volume;     // Volume to play sound at
    float cooldown;         // Time between shots
    float last_shot_time;   // Tracks time since last shot
    
    // Ammo properties
    int magazine_size;      // Max ammo in a magazine
    int current_ammo;       // Current ammo in magazine
    int reserve_ammo;       // Ammo in reserve
    float reload_time;      // Time to reload
    bool is_reloading;      // Currently reloading?
    float reload_timer;     // Current reload progress
    
    // Visual effects
    float shake_intensity;  // How much the weapon shakes when firing
    float shake_decay;      // How quickly shake fades
    glm::vec3 shake_offset; // Current shake offset
    bool is_shaking;        // Is the weapon currently shaking
    
    // States
    bool is_sprinting;      // Is the player sprinting
    bool is_ads;            // Is aiming down sights

    bool is_automatic;
    bool prev_firing;

    model_handle model_id;
    
    // Constructor for default empty weapon
    Weapon() 
        : wep_pos(glm::vec3(0.0f)),
          //min_pos(glm::vec3(0.0f)),
          //ads_pos(glm::vec3(0.0f)),
          //sprint_pos(glm::vec3(0.0f)),
          wep_rot(glm::vec3(0.0f)),
          //ads_speed(0.0f),
          id(Weapon_Id::NONE),
          name("none"),
          sound_file(""),
          sound_dry(""),
          sound_volume(0.0f),
          cooldown(0.0f),
          last_shot_time(0.0f),
          magazine_size(0),
          current_ammo(0),
          reserve_ammo(0),
          reload_time(0.0f),
          is_reloading(false),
          reload_timer(0.0f),
          shake_intensity(0.0f),
          shake_decay(0.0f),
          shake_offset(glm::vec3(0.0f)),
          is_shaking(false),
          is_sprinting(false),
          is_ads(false) {}
    
    static Weapon GLOCK() {
        Weapon weapon;
        //weapon.model.load_model("../resources/models/glock/glock.gltf");
        weapon.id = Weapon_Id::GLOCK;
        weapon.name = "glock";
        //weapon.min_pos = glm::vec3(0.4f, -0.4f, -1.3f);
        //weapon.wep_pos = weapon.min_pos;
        weapon.wep_pos = glm::vec3(0.0f, -1.72, -1.7);
        weapon.wep_rot = glm::vec3(113.0f);
        weapon.wep_scale = 1.0f;
        //weapon.ads_pos = glm::vec3(-0.0001f, -0.4f, -1.0f);
        //weapon.sprint_pos = glm::vec3(0.6f, -0.2f, -1.2f);
        //weapon.ads_speed = 25.0f;
        weapon.sound_file = "glock.wav";
        weapon.sound_dry = "glock_dry.wav";
        weapon.sound_volume = 0.07f;
        weapon.cooldown = 0.13f; // ~300 rounds per minute
        weapon.magazine_size = 17;
        weapon.current_ammo = 17;
        weapon.reserve_ammo = 51;
        weapon.reload_time = 1.8f;
        weapon.shake_intensity = 0.03f;
        weapon.shake_decay = 12.0f;
        weapon.is_automatic = false;
        weapon.prev_firing = false;
        //weapon.model_id = Model_Manager::load_animated_model("pistol4/scene.gltf");
        weapon.model_id = Model_Manager::load_animated_model_cgltf("pistol5/rig.gltf");
        return weapon;
    }

    static Weapon BOW() {
        Weapon weapon;
        //weapon.model.load_model("../resources/models/glock/glock.gltf");
        weapon.id = Weapon_Id::BOW;
        weapon.name = "bow";
        //weapon.min_pos = glm::vec3(0.4f, -0.4f, -1.3f);
        weapon.wep_pos = glm::vec3(-1.72f, -7.182f, -4.36f);
        weapon.wep_rot = glm::vec3(-171.0f);
        weapon.wep_scale = .097f;
        //weapon.ads_pos = glm::vec3(-0.0001f, -0.4f, -1.0f);
        //weapon.sprint_pos = glm::vec3(0.6f, -0.2f, -1.2f);
        //weapon.ads_speed = 25.0f;
        weapon.sound_file = "bow_shoot.mp3 ";
        weapon.sound_dry = "glock_dry.wav";
        weapon.sound_volume = 0.7f;
        weapon.cooldown = 2.0f;
        weapon.magazine_size = 1;
        weapon.current_ammo = 25;
        weapon.reserve_ammo = 51;
        weapon.reload_time = 1.8f;
        weapon.shake_intensity = 0.03f;
        weapon.shake_decay = 12.0f;
        weapon.is_automatic = false;
        weapon.prev_firing = false;
        //weapon.model_id = Model_Manager::load_animated_model("pistol4/scene.gltf");
        weapon.model_id = Model_Manager::load_animated_model_cgltf("bow/scene.gltf");
        return weapon;
    }

    // QBZZZZZZZZZ
    // fps controls / state?
    // glm::vec3 wep_pos = glm::vec3(-0.2f, -0.1f, 0.5f); QBZ pos
    // glm::vec3 min_pos = glm::vec3(-0.2f, -0.1f, 0.5f);
    // glm::vec3 ads_pos = glm::vec3(-0.0001f, -.07f, 0.25f);
    // glm::vec3 wep_rot = glm::vec3(0.0f);
    // float ads_speed = 20.0f;
    // int holding = 0;
    // int rounds_per_min = 700;
    // float weapon_sound_cooldown = 0.07f; 
    // float last_shot_time = 0.0f; 
    
    // Update weapon position and state
    void update(float deltaTime, bool firing, bool reload_requested, bool sprinting, glm::vec3 pos, glm::vec3 facing) {

        // pullout
        // fire
        // run
        // etc
        
        // Update cooldown timer
        last_shot_time += deltaTime;
        
        // Track state
        is_sprinting = sprinting;
        //is_ads = ads_requested && !is_sprinting; // Can't ADS while sprinting
        
        // Handle reloading state
        if (is_reloading) {
            reload_timer += deltaTime;
            if (reload_timer >= reload_time) {
                finish_reload();
            }
        } else if (reload_requested && current_ammo < magazine_size && reserve_ammo > 0) {
            start_reload();
        }
        
        // Determine target position based on state
        //glm::vec3 target_pos;
        //if (is_sprinting) {
        //    target_pos = sprint_pos;
        //} else if (is_ads) {
        //    target_pos = ads_pos;
        //} else {
        //    target_pos = min_pos;
        //}
        
        // Firing logic: check automatic vs semi-automatic
        if (firing) {
            if (is_automatic) {
                // Automatic: fire continuously as long as button is held and cooldown is met.
                if (!is_reloading && last_shot_time >= cooldown && current_ammo > 0) {
                    fire(pos, facing);
                } else if (!is_reloading && last_shot_time >= cooldown && current_ammo == 0) {
                    Audio::play_audio(sound_dry.c_str(), sound_volume);
                    last_shot_time = 0.0f;
                }
            } else {
                // Semi-automatic: fire only on the rising edge (button was not held previously)
                if (!prev_firing && !is_reloading && last_shot_time >= cooldown && current_ammo > 0) {
                    fire(pos, facing);
                } else if (!prev_firing && !is_reloading && last_shot_time >= cooldown && current_ammo == 0) {
                    Audio::play_audio(sound_dry.c_str(), sound_volume);
                    last_shot_time = 0.0f;
                }
            }
        }
        // Store current state for next frame
        prev_firing = firing;
        
        // Apply shake effect if active
        if (is_shaking) {
            // Apply shake to position
            wep_pos += shake_offset;
            
            // Decay shake over time
            shake_offset *= (1.0f - deltaTime * shake_decay);
            
            // Stop shaking if it's very small
            if (glm::length(shake_offset) < 0.001f) {
                is_shaking = false;
                shake_offset = glm::vec3(0.0f);
            }
        }
        
        // Smooth transition to target position
        //wep_pos = glm::mix(wep_pos, target_pos, deltaTime * ads_speed);
        
        // Gradually reset rotation (if there was recoil)
        if (wep_rot.x > 0.0f) {
            wep_rot.x = std::max(0.0f, wep_rot.x - deltaTime * 1.0f);
        }
    }
    
    // Fire the weapon
    void fire(glm::vec3 pos, glm::vec3 facing) {
        // Play weapon sound
        Audio::play_audio(sound_file.c_str(), sound_volume);


        // Reset cooldown timer and decrement ammo
        last_shot_time = 0.0f;
        current_ammo--;
        
        if (Physics::shoot(pos, facing, 10000.0f, 1000.0f)) {
            //Audio::play_audio("hitmarker.wav", 0.15f);
        }
        //apply_shake();
        
        //wep_rot.x += 0.03f;
    }
    
    void apply_shake() {
        is_shaking = true;
        
        float x_offset = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * shake_intensity;
        float y_offset = ((float)rand() / RAND_MAX * 0.8f) * shake_intensity;
        shake_offset = glm::vec3(x_offset, y_offset, 0.0f);
    }
    
    void start_reload() {
        is_reloading = true;
        reload_timer = 0.0f;
        // Audio::play_audio("reload.wav", 0.2f);
    }
    
    void finish_reload() {
        int ammo_needed = magazine_size - current_ammo;
        int ammo_to_use = std::min(reserve_ammo, ammo_needed);
        
        current_ammo += ammo_to_use;
        reserve_ammo -= ammo_to_use;
        
        is_reloading = false;
        reload_timer = 0.0f;
    }

    glm::mat4 get_model_matrix() const {
        glm::mat4 model_matrix(1.0f);

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), wep_pos);
        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(wep_rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 scaling = glm::scale(glm::mat4(1.0f), glm::vec3(wep_scale));

        return translation * rot * scaling;
    }
    
    // Get current reload progress (0.0 to 1.0)
    float get_reload_progress() const {
        if (!is_reloading) return 0.0f;
        return reload_timer / reload_time;
    }
    
    // Get ammo as string for HUD ("current/reserve")
    std::string get_ammo_string() const {
        return std::to_string(current_ammo) + " / " + std::to_string(reserve_ammo);
    }
};