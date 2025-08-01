#pragma once

#include "glow.h"
#include "util/aabb.h"

#include <glm/glm.hpp>

#include <string>

struct Animated_Mesh {
	uint32_t base_vertex;
	uint32_t vertex_count;
	uint32_t base_index;
	uint32_t index_count;
	Util::AABB aabb;
	std::string name;

	uint32_t material_index;
	// parent? 
	glm::mat4 transform; // relative to parent
};

class Animated_Model {
public:
	Animated_Model() = default;
	Animated_Model(std::string name, std::vector<Animated_Mesh> meshes) :
		m_name(name), m_meshes(meshes)
	{
		calculate_aabb();
	}

	void add_mesh(const Animated_Mesh& mesh) {
		m_meshes.push_back(mesh);
	}

	void calculate_aabb() {
		// todo change to calculate some kind of max or something
		m_aabb = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };

		for (const Animated_Mesh& mesh : m_meshes) {
			if (mesh.aabb.min.x < m_aabb.min.x) m_aabb.min.x = mesh.aabb.min.x;
			if (mesh.aabb.min.y < m_aabb.min.y) m_aabb.min.y = mesh.aabb.min.y;
			if (mesh.aabb.min.z < m_aabb.min.z) m_aabb.min.z = mesh.aabb.min.z;

			if (mesh.aabb.max.x > m_aabb.max.x) m_aabb.max.x = mesh.aabb.max.x;
			if (mesh.aabb.max.y > m_aabb.max.y) m_aabb.max.y = mesh.aabb.max.y;
			if (mesh.aabb.max.z > m_aabb.max.z) m_aabb.max.z = mesh.aabb.max.z;
		}
	}

	Util::AABB get_aabb() {
		return m_aabb;
	}

	//private:
	std::string m_name;
	std::vector<Animated_Mesh> m_meshes;
	// todo maybe huge buffer of all meshes or something
	// store index into it? instead of whole mesh
	Util::AABB m_aabb;

	// todo below
	uint32_t base_bone;
	uint32_t bone_count;
	uint32_t bone_offset; // difference between base bones (1 set for all meshes) to skinned bones (1 set per mesh)
	uint32_t base_leaf;
	uint32_t leaf_count;
	uint32_t base_animation;
	uint32_t animation_count;
};