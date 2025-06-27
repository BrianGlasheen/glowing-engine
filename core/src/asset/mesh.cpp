#include "mesh.h"
#include "texture_manager.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, Material material) : material(material) 
{
    this->vertices = vertices;
    this->indices = indices;

    setup_mesh();
    // maybe calc aabb?
}

void Mesh::setup_mesh() 
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

    glBindVertexArray(0);
}

// todo gonna be way different
void Mesh::draw(const Shader* shader, bool shadow_pass) const 
{
    if (!shadow_pass) {

        Texture_Manager::bind(material.albedo_map, 0);
        shader->set_int("diffuse", 0);
        //printf("bound diffuse: %s\n", Texture_Manager::get_name(material.albedo_map).c_str());

        shader->set_bool("has_normal", material.has_normal);
        if (material.has_normal) {
            Texture_Manager::bind(material.normal_map, 1);
            shader->set_int("normal", 1);
            //printf("bound normal: %s\n", Texture_Manager::get_name(material.normal_map).c_str());
        }

        shader->set_bool("has_metallic_roughness", material.metallic_roughness_map != 0);
        if (material.metallic_roughness_map != 0) {
            Texture_Manager::bind(material.metallic_roughness_map, 2);
            shader->set_int("metallic_roughness", 2);
        }
    }

    // draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}  

void Mesh::update_vertex_buffer() 
{
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}