#pragma once

#include "asset/mesh.h"
#include "util/aabb.h"
#include "util/math.h"

#include <string>
#include <cstdint>
#include <vector>

class Animated_Model {
public:
	Animated_Model() = default;
	Animated_Model(std::string name, std::vector<Mesh> meshes) :
		m_name(name), m_meshes(meshes)
	{
		calculate_aabb();
		// set animation offset
	
		//animation_offset = m_meshes[0].base_vertex - base_animation_vertex;
		//printf("\n\n\nbase vertex: %du, base anim vert: %du, offset: %du\n", m_meshes[0].base_vertex, base_animation_vertex, animation_offset);
	}

	void add_mesh(Mesh& mesh) {
		vec3 center = (mesh.aabb.min + mesh.aabb.max) * 0.5f;
		vec3 extent = (mesh.aabb.max - mesh.aabb.min) * 0.5f;
		float radius = length(extent);
		mesh.bounding_sphere = vec4(center, radius);

		m_meshes.push_back(mesh);
	}

	void calculate_aabb() {
		m_aabb = { vec3(FLT_MAX), vec3(-FLT_MAX) };

		for (Mesh& mesh : m_meshes) {
			m_aabb.min = min(mesh.aabb.min, m_aabb.min);
			m_aabb.max = max(mesh.aabb.max, m_aabb.max);
			// mesh.transform = mat4
		}

		animation_offset = m_meshes[0].base_vertex - base_animation_vertex;
		printf("base vertex: %du, base anim vert: %du, offset: %du\n", m_meshes[0].base_vertex, base_animation_vertex, animation_offset);
	}

	Util::AABB get_aabb() {
		return m_aabb;
	}

	//private:
	std::string m_name;
	std::vector<Mesh> m_meshes;
	// todo maybe huge buffer of all meshes or something
	// store index into it? instead of whole mesh
	Util::AABB m_aabb;

	// base vertex in pre-animated buffer
	// use this vertex + vertex_count to animate the
	// actual verticies in the main geom buffer which
	// are indexed by ^^^^ that stuff

	// probably need to find difference between this and first mesh base vertex to use as offset when indexing for each mesh

	// offset = mesh0 - base_animation
	// when writing write at mesh_base_vertex + offset
	uint32_t base_animation_vertex;
	uint32_t animation_offset;

	uint32_t base_bone;
	uint32_t bone_count;
	uint32_t bone_offset; // difference between base bones (1 set for all meshes) to skinned bones (1 set per mesh)
	uint32_t base_leaf;
	uint32_t leaf_count;
	uint32_t base_animation;
	uint32_t animation_count;

	float animation_time; // local time
	bool animation_direction; // true forward false back? maybe add loop / bounce flags
	uint32_t current_animation;
};