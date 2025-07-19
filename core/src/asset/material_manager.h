#pragma once

#include "glow.h"

struct Material_Indirect {
	uint64_t albedo; // bindless handles
	uint64_t normal;
	//uint64_t metallic_roughness;
};

namespace Material_Manager {
	uint32_t add_material(const Material_Indirect& mat);
	const Material_Indirect& get_material(uint32_t index);
	size_t get_material_count();
}
