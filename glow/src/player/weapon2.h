//#pragma once
//
//#include <string>
//#include <vector>
//#include <unordered_map>
//#include "glm/glm.hpp"
//#include <core/audio.h>
//#include <core/physics.h>
//
//// Plain data structures - no methods, just data
//struct WeaponStats {
//    float damage;
//    float cooldown;
//    float reload_time;
//    float ads_speed;
//    int magazine_size;
//    int reserve_ammo;
//    float shake_intensity;
//    float shake_decay;
//    bool is_automatic;
//
//    // Audio
//    const char* fire_sound;
//    const char* dry_fire_sound;
//    const char* reload_sound;
//    float sound_volume;
//
//    // Visual positioning
//    glm::vec3 hip_pos;
//    glm::vec3 ads_pos;
//    glm::vec3 sprint_pos;
//    glm::vec3 base_rotation;
//
//    // Model/animation data
//    const char* model_path;
//    const char* animation_idle;
//    const char* animation_fire;
//    const char* animation_reload;
//};
//
//// Runtime weapon state - separate from static data
//struct WeaponState {
//    int current_ammo;
//    float last_shot_time;
//    float reload_timer;
//
//    // Current visual state
//    glm::vec3 current_pos;
//    glm::vec3 current_rot;
//    glm::vec3 shake_offset;
//
//    // State flags packed into bitfield for cache efficiency
//    union {
//        struct {
//            uint8_t is_reloading : 1;
//            uint8_t is_shaking : 1;
//            uint8_t prev_firing : 1;
//            uint8_t is_ads : 1;
//            uint8_t is_sprinting : 1;
//            uint8_t _padding : 3;
//        };
//        uint8_t flags;
//    };
//
//    // Animation state
//    float current_anim_time;
//    const char* current_animation;
//};
//
//// Weapon IDs - used as indices into arrays
//enum WeaponId : uint8_t {
//    WEAPON_M4A1 = 0,
//    WEAPON_GLOCK,
//    WEAPON_AK47,
//    WEAPON_SHOTGUN,
//    WEAPON_SNIPER,
//    WEAPON_COUNT
//};
//
//// Global weapon database - loaded once, never modified at runtime
//extern WeaponStats g_weapon_stats[WEAPON_COUNT];
//
//// Player weapon data - multiple weapons can share the same stats
//struct PlayerWeapon {
//    WeaponId weapon_id;        // Index into g_weapon_stats
//    WeaponState state;         // Runtime state
//    uint32_t entity_id;        // For animated mesh (if using entity system)
//};
//
//// Weapon system - handles all weapons for all players
//class WeaponSystem {
//private:
//    // Array of all active weapons (more cache friendly than map)
//    std::vector<PlayerWeapon> weapons;
//    std::vector<uint32_t> free_slots;  // Reuse deleted weapon slots
//
//public:
//    // Create a new weapon instance
//    uint32_t create_weapon(WeaponId weapon_id) {
//        uint32_t slot;
//
//        if (!free_slots.empty()) {
//            slot = free_slots.back();
//            free_slots.pop_back();
//        }
//        else {
//            slot = weapons.size();
//            weapons.resize(slot + 1);
//        }
//
//        PlayerWeapon& weapon = weapons[slot];
//        weapon.weapon_id = weapon_id;
//
//        // Initialize state from stats
//        const WeaponStats& stats = g_weapon_stats[weapon_id];
//        weapon.state.current_ammo = stats.magazine_size;
//        weapon.state.last_shot_time = 0.0f;
//        weapon.state.reload_timer = 0.0f;
//        weapon.state.current_pos = stats.hip_pos;
//        weapon.state.current_rot = stats.base_rotation;
//        weapon.state.shake_offset = glm::vec3(0.0f);
//        weapon.state.flags = 0;  // Clear all flags
//        weapon.state.current_anim_time = 0.0f;
//        weapon.state.current_animation = stats.animation_idle;
//
//        return slot;
//    }
//
//    void destroy_weapon(uint32_t weapon_handle) {
//        if (weapon_handle < weapons.size()) {
//            free_slots.push_back(weapon_handle);
//        }
//    }
//
//    // Update a specific weapon
//    void update_weapon(uint32_t weapon_handle, float delta_time,
//        bool ads_requested, bool firing, bool reload_requested,
//        bool sprinting, glm::vec3 player_pos, glm::vec3 facing) {
//
//        if (weapon_handle >= weapons.size()) return;
//
//        PlayerWeapon& weapon = weapons[weapon_handle];
//        const WeaponStats& stats = g_weapon_stats[weapon.weapon_id];
//        WeaponState& state = weapon.state;
//
//        // Update timers
//        state.last_shot_time += delta_time;
//        state.current_anim_time += delta_time;
//
//        // Update state flags
//        state.is_sprinting = sprinting;
//        state.is_ads = ads_requested && !sprinting;
//
//        // Handle reloading
//        if (state.is_reloading) {
//            state.reload_timer += delta_time;
//            if (state.reload_timer >= stats.reload_time) {
//                finish_reload(weapon);
//            }
//        }
//        else if (reload_requested && state.current_ammo < stats.magazine_size) {
//            start_reload(weapon);
//        }
//
//        // Handle firing
//        handle_firing(weapon, firing, player_pos, facing, delta_time);
//
//        // Update visual position
//        update_weapon_position(weapon, delta_time);
//
//        // Update shake
//        if (state.is_shaking) {
//            state.shake_offset *= (1.0f - delta_time * stats.shake_decay);
//            if (glm::length(state.shake_offset) < 0.001f) {
//                state.is_shaking = false;
//                state.shake_offset = glm::vec3(0.0f);
//            }
//        }
//    }
//
//    // Get weapon for rendering/querying
//    const PlayerWeapon* get_weapon(uint32_t weapon_handle) const {
//        return (weapon_handle < weapons.size()) ? &weapons[weapon_handle] : nullptr;
//    }
//
//    PlayerWeapon* get_weapon_mutable(uint32_t weapon_handle) {
//        return (weapon_handle < weapons.size()) ? &weapons[weapon_handle] : nullptr;
//    }
//
//private:
//    void handle_firing(PlayerWeapon& weapon, bool firing, glm::vec3 pos, glm::vec3 facing, float delta_time) {
//        const WeaponStats& stats = g_weapon_stats[weapon.weapon_id];
//        WeaponState& state = weapon.state;
//
//        if (firing) {
//            bool can_fire = !state.is_reloading && state.last_shot_time >= stats.cooldown;
//            bool should_fire = stats.is_automatic || !state.prev_firing;
//
//            if (can_fire && should_fire) {
//                if (state.current_ammo > 0) {
//                    fire_weapon(weapon, pos, facing);
//                }
//                else {
//                    // Dry fire
//                    if (stats.dry_fire_sound) {
//                        Audio::play_audio(stats.dry_fire_sound, stats.sound_volume);
//                    }
//                    state.last_shot_time = 0.0f;
//                }
//            }
//        }
//
//        state.prev_firing = firing;
//    }
//
//    void fire_weapon(PlayerWeapon& weapon, glm::vec3 pos, glm::vec3 facing) {
//        const WeaponStats& stats = g_weapon_stats[weapon.weapon_id];
//        WeaponState& state = weapon.state;
//
//        // Play sound
//        Audio::play_audio(stats.fire_sound, stats.sound_volume);
//
//        // Physics/damage
//        if (Physics::shoot(pos, facing, 999999999.f, stats.damage)) {
//            // Hit something
//        }
//
//        // Update state
//        state.last_shot_time = 0.0f;
//        state.current_ammo--;
//
//        // Apply shake
//        if (stats.shake_intensity > 0.0f) {
//            apply_shake(weapon);
//        }
//
//        // Set fire animation
//        state.current_animation = stats.animation_fire;
//        state.current_anim_time = 0.0f;
//    }
//
//    void apply_shake(PlayerWeapon& weapon) {
//        const WeaponStats& stats = g_weapon_stats[weapon.weapon_id];
//        WeaponState& state = weapon.state;
//
//        state.is_shaking = true;
//        float x = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * stats.shake_intensity;
//        float y = ((float)rand() / RAND_MAX * 0.8f) * stats.shake_intensity;
//        state.shake_offset = glm::vec3(x, y, 0.0f);
//    }
//
//    void start_reload(PlayerWeapon& weapon) {
//        WeaponState& state = weapon.state;
//        const WeaponStats& stats = g_weapon_stats[weapon.weapon_id];
//
//        state.is_reloading = true;
//        state.reload_timer = 0.0f;
//        state.current_animation = stats.animation_reload;
//        state.current_anim_time = 0.0f;
//
//        if (stats.reload_sound) {
//            Audio::play_audio(stats.reload_sound, stats.sound_volume);
//        }
//    }
//
//    void finish_reload(PlayerWeapon& weapon) {
//        const WeaponStats& stats = g_weapon_stats[weapon.weapon_id];
//        WeaponState& state = weapon.state;
//
//        // Simple reload - fill magazine (could be made more complex)
//        state.current_ammo = stats.magazine_size;
//        state.is_reloading = false;
//        state.reload_timer = 0.0f;
//        state.current_animation = stats.animation_idle;
//        state.current_anim_time = 0.0f;
//    }
//
//    void update_weapon_position(PlayerWeapon& weapon, float delta_time) {
//        const WeaponStats& stats = g_weapon_stats[weapon.weapon_id];
//        WeaponState& state = weapon.state;
//
//        // Choose target position
//        glm::vec3 target_pos;
//        if (state.is_sprinting) {
//            target_pos = stats.sprint_pos;
//        }
//        else if (state.is_ads) {
//            target_pos = stats.ads_pos;
//        }
//        else {
//            target_pos = stats.hip_pos;
//        }
//
//        // Smooth interpolation
//        state.current_pos = glm::mix(state.current_pos, target_pos, delta_time * stats.ads_speed);
//
//        // Apply shake if active
//        if (state.is_shaking) {
//            state.current_pos += state.shake_offset;
//        }
//    }
//};
//
//// Global weapon database definition (would be in .cpp file)
//WeaponStats g_weapon_stats[WEAPON_COUNT] = {
//    // M4A1
//    {
//        .damage = 33.0f,
//        .cooldown = 0.07f,
//        .reload_time = 2.5f,
//        .ads_speed = 20.0f,
//        .magazine_size = 30,
//        .reserve_ammo = 90,
//        .shake_intensity = 0.05f,
//        .shake_decay = 10.0f,
//        .is_automatic = true,
//        .fire_sound = "gun1.wav",
//        .dry_fire_sound = "dry_fire.wav",
//        .reload_sound = "m4_reload.wav",
//        .sound_volume = 0.1f,
//        .hip_pos = glm::vec3(0.6f, -0.5f, -1.6f),
//        .ads_pos = glm::vec3(0.0f, -0.45f, -1.2f),
//        .sprint_pos = glm::vec3(0.8f, -0.3f, -1.5f),
//        .base_rotation = glm::vec3(0.0f),
//        .model_path = "models/m4a1.fbx",
//        .animation_idle = "m4_idle",
//        .animation_fire = "m4_fire",
//        .animation_reload = "m4_reload"
//    },
//    {
//        .damage = 28.0f,
//        .cooldown = 0.13f,
//        .reload_time = 1.8f,
//        .ads_speed = 25.0f,
//        .magazine_size = 17,
//        .reserve_ammo = 51,
//        .shake_intensity = 0.03f,
//        .shake_decay = 12.0f,
//        .is_automatic = false,
//        .fire_sound = "glock.wav",
//        .dry_fire_sound = "glock_dry.wav",
//        .reload_sound = "glock_reload.wav",
//        .sound_volume = 0.07f,
//        .hip_pos = glm::vec3(0.4f, -0.4f, -1.3f),
//        .ads_pos = glm::vec3(-0.0001f, -0.4f, -1.0f),
//        .sprint_pos = glm::vec3(0.6f, -0.2f, -1.2f),
//        .base_rotation = glm::vec3(0.0f),
//        .model_path = "models/glock.fbx",
//        .animation_idle = "glock_idle",
//        .animation_fire = "glock_fire",
//        .animation_reload = "glock_reload"
//    }
//};