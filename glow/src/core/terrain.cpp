#include "core/terrain.h"

#include "core/opengl.h"

#include <vector>

void Terrain::init(float width, float height, uint32_t num_patches_x, uint32_t num_patches_z, const std::string& path) {

	// load hightmap
	heightmap = Texture_Manager::load_heightmap(path);
	heightmap_texture = Texture_Manager::load_from_path("../resources/textures/terrain/atxcolor.png");

	std::vector<float> vertices;
	for (uint32_t i = 0; i < num_patches_x; i++) {
		for (uint32_t j = 0; j < num_patches_z; j++) {
			float fi = (float)i;
			float fj = (float)j;

			vertices.push_back(-width / 2.0f + width * fi / (float)num_patches_x); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-height / 2.0f + height * fj / (float)num_patches_z); // v.z
			vertices.push_back(fi / (float)num_patches_x); // u
			vertices.push_back(fj / (float)num_patches_z); // v

			vertices.push_back(-width / 2.0f + width * (fi + 1) / (float)num_patches_x); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-height / 2.0f + height * fj / (float)num_patches_z); // v.z
			vertices.push_back((fi + 1) / (float)num_patches_x); // u
			vertices.push_back(fj / (float)num_patches_z); // v

			vertices.push_back(-width / 2.0f + width * fi / (float)num_patches_x); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-height / 2.0f + height * (fj + 1) / (float)num_patches_z); // v.z
			vertices.push_back(fi / (float)num_patches_x); // u
			vertices.push_back((fj + 1) / (float)num_patches_z); // v

			vertices.push_back(-width / 2.0f + width * (fi + 1) / (float)num_patches_x); // v.x
			vertices.push_back(0.0f); // v.y
			vertices.push_back(-height / 2.0f + height * (fj + 1) / (float)num_patches_z); // v.z
			vertices.push_back((fi + 1) / (float)num_patches_x); // u
			vertices.push_back((fj + 1) / (float)num_patches_z); // v
		}
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER,
		vertices.size() * sizeof(float),       // size of vertices buffer
		&vertices[0],                          // pointer to first element
		GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	// todo memory = vertices.size() * sizeof(float)
	vertex_count = num_patches_x * num_patches_z * 4;
}

void Terrain::cleanup() {
	glDeleteBuffers(1, &vbo);
	vbo = 0;
	glDeleteVertexArrays(1, &vao);
	vao = 0;
}

//uint32_t Terrain::get_vao() const {
//	return vao;
//}
