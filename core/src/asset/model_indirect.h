#include <string>
#include <vector>

#include "glow.h"

#include "asset/material_manager.h"
#include "util/aabb.h"

struct Mesh_Info {
	std::string mesh_name;
	size_t index;
};

struct Mesh_Indirect {
	uint32_t base_vertex;
	uint32_t vertex_count;
	uint32_t base_index;
	uint32_t index_count;
	// material info? idx to material buffer?
	Util::AABB aabb;
	std::string name;
	
	uint32_t material_index;
	// parent? 
	// glm::mat4 local_transform; relative to parent
};

class Model_Indirect {
public:
	Model_Indirect() = default;
	Model_Indirect(std::string name, std::vector<Mesh_Indirect> meshes) : 
		m_name(name), m_meshes(meshes)
	{
		calculate_aabb();
	}

	void add_mesh(const Mesh_Indirect& mesh) {
		m_meshes.push_back(mesh);
	}

	void calculate_aabb() {
		m_aabb = { glm::vec3(FLT_MAX), glm::vec3(FLT_MIN) };

		for (const Mesh_Indirect& mesh : m_meshes) {
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
	std::vector<Mesh_Indirect> m_meshes;
	// todo maybe huge buffer of all meshes or something
	// store index into it? instead of whole mesh
	Util::AABB m_aabb;

};