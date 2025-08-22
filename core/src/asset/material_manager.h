#pragma once

#include <cstdint>
#include "glm/glm.hpp"

enum Blend_Mode {
	disabled = 0, // opaque / clipped
	blend, // one minus alpha prob
	additive
};

struct Material {
	uint64_t albedo; // bindless handles
	glm::vec4 base_color;
	uint64_t normal;
	uint64_t met_rough;
	uint64_t emissive;
	uint64_t amb_occ;
	glm::vec4 emissive_factor; // r g b strength
	float metallic_factor;
	float roughness_factor;
	float alpha_cutoff;
	Blend_Mode blend_mode;
};

namespace Material_Manager {
	uint32_t add_material(const Material& mat);
	const Material& get_material(uint32_t index);
	size_t get_material_count();
}
