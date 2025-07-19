#include "material_manager.h"

#include <vector>

namespace Material_Manager {
	std::vector<Material_Indirect> g_materials;

	uint32_t add_material(const Material_Indirect& mat) {
		uint32_t index = g_materials.size();
		g_materials.push_back(mat);
		return index;
	}
	
	const Material_Indirect& get_material(uint32_t index) {
		return g_materials[index];
	}

	size_t get_material_count() {
		return g_materials.size();
	}
}
