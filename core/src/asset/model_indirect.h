#include <string>
#include <vector>

#include "glow.h"

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
	
	// parent? 
	// glm::mat4 local_transform; relative to parent
};

class Model_Indirect {
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
		// go through mesh aabbs
		// max min
	}

	Util::AABB get_aabb() {
		return m_aabb;
	}

private:
	std::string m_name;
	std::vector<Mesh_Indirect> m_meshes;
	// todo maybe huge buffer of all meshes or something
	// store index into it? instead of whole mesh
	Util::AABB m_aabb;

};