#pragma once

#include "asset/mesh.h"
#include "asset/material_manager.h"
#include "util/aabb.h"

#include "glm/glm.hpp"

#include <string>
#include <vector>
#include <cstdint>

class Model {
public:
	Model() = default;
	Model(std::string name, std::vector<Mesh> meshes) : 
		m_name(name), m_meshes(meshes)
	{
		calculate_aabb();
	}

	void add_mesh(Mesh& mesh) {
		glm::vec3 center = (mesh.aabb.min + mesh.aabb.max) * 0.5f;
		glm::vec3 extent = (mesh.aabb.max - mesh.aabb.min) * 0.5f;
		float radius = glm::length(extent);
		mesh.bounding_sphere = glm::vec4(center, radius);

		m_meshes.push_back(mesh);
	}

	void calculate_aabb() {
		m_aabb = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };

		for (const Mesh& mesh : m_meshes) {
			if (mesh.aabb.min.x < m_aabb.min.x) m_aabb.min.x = mesh.aabb.min.x;
			if (mesh.aabb.min.y < m_aabb.min.y) m_aabb.min.y = mesh.aabb.min.y;
			if (mesh.aabb.min.z < m_aabb.min.z) m_aabb.min.z = mesh.aabb.min.z;

			if (mesh.aabb.max.x > m_aabb.max.x) m_aabb.max.x = mesh.aabb.max.x;
			if (mesh.aabb.max.y > m_aabb.max.y) m_aabb.max.y = mesh.aabb.max.y;
			if (mesh.aabb.max.z > m_aabb.max.z) m_aabb.max.z = mesh.aabb.max.z;
		}

		//glm::vec3 center = (m_aabb.min + m_aabb.max) * 0.5f;
		//glm::vec3 extent = (m_aabb.max - m_aabb.min) * 0.5f;
		//float radius = glm::length(extent);
		//m_bounding_sphere = glm::vec4(center, radius);
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
	glm::vec4 m_bounding_sphere;
};