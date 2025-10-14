#pragma once

// #include "core/ssbo.h"
#include "util/math.h"

#include <string>

typedef uint32_t particle_handle;

struct Particle_Paramaters {
	vec3 emitter_position;
	vec3 acceleration_direction;
	float acceleration_force;
	vec2 life_range;              // min/max particle lifetime

	vec4 color_start_base;        // base colors for variation
	vec4 color_end_base;

	vec3 velocity_base;           // base spawn velocity
	vec3 velocity_random_bias;
	float velocity_mag;
	
	float emission_rate;          // particles per second
	int max_particles;            // maximum active 
};

namespace Particle_Manager {
	void init();
	void add_effect(const std::string& name, Particle_Paramaters params, const float lifetime);
	void add_lifetime(const std::string& name);
	void step_particles(const float dt);
	void draw();
}
