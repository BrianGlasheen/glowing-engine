#include "asset/particle_manager.h"

#include "core/ssbo.h"
#include "core/opengl.h"
#include "asset/shader_manager.h"

#include <unordered_map>

struct GPU_Particle {
	glm::vec3 position;
	float ttl;
	glm::vec3 velocity;
	float max_ttl;
	glm::vec4 color_start;
	glm::vec4 color_end;
	float size_start;
	float size_end;
	glm::vec2 padding;
};

struct Effect {
	Particle_Paramaters parameters;
	SSBO buffer;
	float remaining_lifetime;
};

namespace Particle_Manager {
	static std::unordered_map<std::string, Effect> effects;

	void init() {
		Shader_Manager::load_compute("particle2");

		Particle_Paramaters a = {
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),  // Up
			2.0f,
			glm::vec2(1.0f, 3.0f),
			glm::vec4(1.0f, 0.8f, 0.2f, 1.0f),     // Bright orange
			glm::vec4(0.8f, 0.1f, 0.0f, 0.0f),       // Dark red, fade out
			glm::vec3(0.0f, 2.0f, 0.0f),
			glm::vec3(1.5f, 0.5f, 1.5f),       // Spread outward
			3.0f,
			150.0f,
			5000
		};

		Particle_Paramaters b = {
			glm::vec3(-250.0f, 2.0f, 0.0f),
			glm::vec3(0.0f, -1.0f, 0.0f),  // Gentle fall
			0.5f,
			glm::vec2(2.0f, 5.0f),
			glm::vec4(0.9f, 0.7f, 1.0f, 1.0f),     // Light purple
			glm::vec4(1.0f, 1.0f, 0.8f, 0.0f),       // Golden fade
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(2.0f, 1.0f, 2.0f),       // Wide spread
			1.5f,
			75.0f,
			3000
		};

		Particle_Paramaters c = {
			glm::vec3(250.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, -1.0f, 0.0f),  // Gravity down
			8.0f,
			glm::vec2(0.5f, 2.0f),
			glm::vec4(1.0f, 1.0f, 0.9f, 1.0f),     // Bright white
			glm::vec4(0.3f, 0.3f, 0.3f, 0.0f),       // Dark smoke
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(8.0f, 6.0f, 8.0f),       // Explosive spread
			12.0f,
			500.0f,  // High burst rate
			8000
		};

		add_effect("a", a, 10.0f);
		add_effect("b", b, 10.0f);
		add_effect("c", c, 10.0f);
	}

	//struct Particle_Paramaters {
	//	glm::vec3 emitter_position;
	//	glm::vec3 acceleration_direction;
	//	float acceleration_force;
	//	glm::vec2 life_range;              // min/max particle lifetime

	//	glm::vec4 color_start_base;        // base colors for variation
	//	glm::vec4 color_end_base;

	//	glm::vec3 velocity_base;           // base spawn velocity
	//	glm::vec3 velocity_random_bias;
	//	float velocity_mag;

	//	float emission_rate;          // particles per second
	//	int max_particles;            // maximum active 
	//};

	void add_effect(const std::string& name, Particle_Paramaters params, const float lifetime) {
		std::vector<GPU_Particle> particles(params.max_particles);

		auto& effect = effects[name];
		effect.parameters = params;
		effect.remaining_lifetime = lifetime;
		effect.buffer.init();
		effect.buffer.set_data(sizeof(GPU_Particle) * params.max_particles, particles.data(), GL_DYNAMIC_DRAW);

		//SSBO ssbo;
		//ssbo.init();
		//ssbo.set_data(sizeof(GPU_Particle) * params.max_particles, particles.data(), GL_DYNAMIC_DRAW);

		//Effect e = { params, ssbo, lifetime };
		//effects[name] = e;
	}

	void add_lifetime(const std::string& name) {
		// get particle effect
		// add lifetime to it
	}

	void step_particles(const float dt) {
		// bind shader
		// for every effect
		// bind buffer

		Compute_Shader* particle = Shader_Manager::get_compute("particle2");
		particle->use();
		particle->set_float("dt", dt);

		for (auto it = effects.begin(); it != effects.end();) {
			Effect& e = it->second;
			e.remaining_lifetime -= dt;
			if (e.remaining_lifetime < 0) {
				it = effects.erase(it);
				continue;
			}
			else
				it++;

			const Particle_Paramaters p = e.parameters;

			particle->set_vec3("emitter_position", p.emitter_position);
			particle->set_vec3("acceleration_direction", p.acceleration_direction);
			particle->set_float("acceleration_force", p.acceleration_force);
			particle->set_vec2("life_range", p.life_range);
			particle->set_vec4("color_start_base", p.color_start_base);
			particle->set_vec4("color_end_base", p.color_end_base);
			particle->set_vec3("velocity_base", p.velocity_base);
			particle->set_vec3("velocity_random_bias", p.velocity_random_bias);
			particle->set_float("velocity_mag", p.velocity_mag);
			particle->set_float("emission_rate", p.emission_rate);
			particle->set_int("max_particle", p.max_particles);
			
			e.buffer.bind(0);
			glDispatchCompute((p.max_particles + 127) / 128, 1, 1);
		}

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // todo rm move to render
	}

	void draw() {
		for (auto it = effects.begin(); it != effects.end(); it++) {
			const Effect& e = it->second;
			e.buffer.bind(0);
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, e.parameters.max_particles);
		}
	}
}
