#pragma once

#include "glow.h"

#include <glm/glm.hpp>

struct Material_Indirect {
	uint64_t albedo; // bindless handles
	glm::vec4 base_color;
	uint64_t normal;
	uint64_t met_rough;
	uint64_t emissive;
	uint64_t amb_occ;
	glm::vec4 emissive_factor; // r g b strength
	float metallic_factor;
	float roughness_factor;
};

namespace Material_Manager {
	uint32_t add_material(const Material_Indirect& mat);
	const Material_Indirect& get_material(uint32_t index);
	size_t get_material_count();
}
