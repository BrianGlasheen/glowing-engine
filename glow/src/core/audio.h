// tokyo spliff
#pragma once

#include <string>

#ifdef FMOD
#include "fmod.hpp"
#include <fmod_errors.h>
#endif

struct Audio_handle {
    #ifdef FMOD
    FMOD::Sound* sound = nullptr;
    FMOD::Channel* channel = nullptr;
    std::string filename;
    #endif
};

struct Audio_effect_info {
    #ifdef FMOD
    std::string filename = "";
    float volume = 0.0f;
    #endif
};

namespace Audio {
    void init();
    void update();
    void load_audio(const std::string& filename);
    void stop_audio(const std::string& filename);
    void loop_audio(const std::string& filename, float volume);
    void loop_audio_if_not_playing(const std::string& filename, float volume);
    void play_audio(const std::string& filename, float volume, float frequency = 1.0f);
    void set_audio_volume(const std::string& filename, float volume);
}
