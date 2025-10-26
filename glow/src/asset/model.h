#pragma once

#include "asset/mesh.h"
#include "util/aabb.h"

#include "util/math.h"

#include <string>
#include <vector>

class Model {
public:
	Model() = default;

	Model(std::string name, std::vector<Mesh> meshes) : 
		m_name(name), m_meshes(meshes)
	{
		calculate_aabb();
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

		for (const Mesh& mesh : m_meshes) {
			m_aabb.min = min(mesh.aabb.min, m_aabb.min);
			m_aabb.max = max(mesh.aabb.max, m_aabb.max);
		}

		//vec3 center = (m_aabb.min + m_aabb.max) * 0.5f;
		//vec3 extent = (m_aabb.max - m_aabb.min) * 0.5f;
		//float radius = length(extent);
		//m_bounding_sphere = vec4(center, radius);
	}

	Util::AABB get_aabb() {
		return m_aabb;
	}

//private:
	std::string m_name;
	std::vector<Mesh> m_meshes;
	Util::AABB m_aabb;
	vec4 m_bounding_sphere;
};