#pragma once

#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

#include "mesh.h"
#include "shader.h"
#include "util/aabb.h"

class Model {
    public:
        Model() = default;
        Model(const std::string& meshName);
        
        int load_model(const std::string &meshName);
        void draw(const Shader* shader, bool shadow_pass);	
        Util::AABB get_aabb();

    private:
        std::vector<Mesh> meshes;
        Util::AABB aabb;

        void process_node(aiNode *node, const aiScene *scene, const std::string& path);
        Mesh process_mesh(aiMesh *mesh, const aiScene *scene, const std::string& path);
        void calculate_aabb();
};
