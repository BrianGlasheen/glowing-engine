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

	// uint32_t parent id? maybe later if needed prob not 
	glm::mat4 transform; // relative to parent
	
	Material material;
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
		m_aabb = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };

		for (const Animated_Mesh& mesh : m_meshes) {
			m_aabb.min = glm::min(mesh.aabb.min, m_aabb.min);
			m_aabb.max = glm::max(mesh.aabb.max, m_aabb.max);
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

	uint32_t base_bone;
	uint32_t bone_count;
	uint32_t bone_offset; // difference between base bones (1 set for all meshes) to skinned bones (1 set per mesh)
	uint32_t base_leaf;
	uint32_t leaf_count;
	uint32_t base_animation;
	uint32_t animation_count;

	float animation_time = 0.0; // local time
	bool animation_direction = true; // true forward false back? maybe add loop / bounce flags
	uint32_t current_animation = 0;
};