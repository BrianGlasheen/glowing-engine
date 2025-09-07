// tokyo spliff
#include "audio.h"

#include <iostream>
#include <vector>
#include <unordered_map>

namespace Audio {
    // Global variables and FMOD system objects
#ifdef FMOD
    std::unordered_map<std::string, FMOD::Sound*> g_loaded_audio;
    constexpr int audio_channel_count = 512;
    FMOD::System* g_system = nullptr;
    std::vector<Audio_handle> g_playing_audio;  // Matches struct Audio_handle
#endif

    void init() {
#ifdef FMOD
        // Create the main system object.
        FMOD_RESULT result = FMOD::System_Create(&g_system);
        if (result != FMOD_OK) {
            std::cerr << "FMOD: Failed to create system object: "
                      << FMOD_ErrorString(result) << "\n";
        }

        // Initialize FMOD.
        result = g_system->init(audio_channel_count, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) {
            std::cerr << "FMOD: Failed to initialize system object: "
                      << FMOD_ErrorString(result) << "\n";
        }

        // Create a channel group (optional usage).
        FMOD::ChannelGroup* channel_group = nullptr;
        result = g_system->createChannelGroup("inGameSoundEffects", &channel_group);
        if (result != FMOD_OK) {
            std::cerr << "FMOD: Failed to create in-game sound effects channel group: "
                      << FMOD_ErrorString(result) << "\n";
        }
#endif
    }

    void load_audio(const std::string& filename) {
#ifdef FMOD
        // Only load if not already present
        if (g_loaded_audio.find(filename) == g_loaded_audio.end()) {
            FMOD::Sound* sound = nullptr;
            FMOD_MODE mode = FMOD_DEFAULT;

            FMOD_RESULT result = g_system->createSound(("../resources/sound/" + filename).c_str(),
                                                       mode, nullptr, &sound);
            if (result != FMOD_OK) {
                std::cerr << "FMOD: Failed to load sound: "
                          << FMOD_ErrorString(result) << "\n";
            }
            g_loaded_audio[filename] = sound;
        }
#endif
    }

    void update() {
#ifdef FMOD
        // Remove completed audio
        for (int i = 0; i < static_cast<int>(g_playing_audio.size()); ++i) {
            Audio_handle& handle = g_playing_audio[i];
            bool is_playing = false;

            // Check if channel is done playing
            FMOD_RESULT result = FMOD_OK;
            if (handle.channel) {
                result = handle.channel->isPlaying(&is_playing);
            }

            if (result != FMOD_OK || !is_playing) {
                // Erase this entry
                g_playing_audio.erase(g_playing_audio.begin() + i);
                i--;
            }
        }

        // Update FMOD
        g_system->update();
#endif
    }

    void play_audio(const std::string& filename, float volume, float frequency) {
#ifdef FMOD
        // Ensure the sound is loaded
        load_audio(filename);

        // Add a new handle to the playing list
        Audio_handle handle;
        handle.sound = g_loaded_audio[filename];
        handle.filename = filename;

        // Play the sound
        g_system->playSound(handle.sound, nullptr, false, &handle.channel);
        if (handle.channel) {
            handle.channel->setVolume(volume);

            float current_frequency = 0.0f;
            handle.channel->getFrequency(&current_frequency);
            handle.channel->setFrequency(current_frequency * frequency);
        }

        g_playing_audio.emplace_back(handle);
#endif
    }

    void loop_audio_if_not_playing(const std::string& filename, float volume) {
#ifdef FMOD
        // If already playing, do nothing
        for (auto& handle : g_playing_audio) {
            if (handle.filename == filename) {
                return;
            }
        }
        // Otherwise, start looping
        loop_audio(filename, volume);
#endif
    }

    void loop_audio(const std::string& filename, float volume) {
#ifdef FMOD
        // Ensure the sound is loaded
        load_audio(filename);

        // Add a new handle to the playing list
        Audio_handle handle;
        handle.sound = g_loaded_audio[filename];
        handle.filename = filename;

        // Set loop modes
        handle.sound->setMode(FMOD_LOOP_NORMAL);
        handle.sound->setLoopCount(-1);

        g_system->playSound(handle.sound, nullptr, false, &handle.channel);
        if (handle.channel) {
            handle.channel->setVolume(volume);
        }

        g_playing_audio.emplace_back(handle);
#endif
    }

    void stop_audio(const std::string& filename) {
#ifdef FMOD
        for (auto& handle : g_playing_audio) {
            if (handle.filename == filename && handle.channel) {
                FMOD_RESULT result = handle.channel->stop();
                if (result != FMOD_OK) {
                    std::cerr << "FMOD error when stopping sound: "
                              << FMOD_ErrorString(result) << "\n";
                }
            }
        }
#endif
    }

    void set_audio_volume(const std::string& filename, float volume) {
#ifdef FMOD
        for (auto& handle : g_playing_audio) {
            if (handle.filename == filename && handle.channel) {
                handle.channel->setVolume(volume);
            }
        }
#endif
    }

}
