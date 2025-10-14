#pragma once

#include "asset/material_manager.h"
#include "util/aabb.h"
#include "util/math.h"

#include <string>
#include <cstdint>

struct Mesh {
	uint32_t base_vertex;
	uint32_t vertex_count;
	uint32_t base_index;
	uint32_t index_count;
	Util::AABB aabb;
	std::string name;

	// parent? 
	mat4 transform; // relative to parent

	Material material;

	vec4 bounding_sphere;
};
