#include "model.h"

#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "texture_manager.h"

Model::Model(const std::string& path)
{
    load_model(path);
}


int Model::load_model(const std::string &path) 
{
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(path, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);
	
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return -1;
    }

    process_node(scene->mRootNode, scene, path);
    calculate_aabb();
    shift_mesh_up();
    calculate_aabb();
    return 0;
}

void Model::draw(const Shader* shader, bool shadow_pass)
{
    for (size_t i = 0; i < meshes.size(); i++)
        meshes[i].draw(shader, shadow_pass);
}

Util::AABB Model::get_aabb()
{
    return aabb;
}

void Model::process_node(aiNode *node, const aiScene *scene, const std::string& path) 
{
    // process all the node's meshes (if any)
    for(uint32_t i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes.push_back(process_mesh(mesh, scene, path));			
    }

    // then do the same for each of its children
    for(uint32_t i = 0; i < node->mNumChildren; i++) {
        process_node(node->mChildren[i], scene, path);
    }
} 

Mesh Model::process_mesh(aiMesh *mesh, const aiScene *scene, const std::string& path) 
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for(uint32_t i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        // process vertex positions, normals and texture coordinates
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.normal   = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

        if(mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x; 
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.tex_coords = vec;

            const aiVector3D& pTangent = mesh->mTangents[i];
            vertex.tangent = glm::vec3(pTangent.x, pTangent.y, pTangent.z);

            const aiVector3D& pBitangent = mesh->mBitangents[i];
            vertex.bitangent = glm::vec3(pBitangent.x, pBitangent.y, pBitangent.z);
        }
        else {
            vertex.tex_coords = glm::vec2(0.0f, 0.0f);
            vertex.tangent = glm::vec3(0.0f);
            vertex.bitangent = glm::vec3(0.0f);
        }

        vertices.push_back(vertex);
    }
    // process indices
    for(uint32_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(uint32_t j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // process material
    texture_handle albedo = 0, normal = 0, metrough = 0, occ = 0, emis = 0;
    if(mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        if (material->GetTextureCount(aiTextureType_BASE_COLOR)) {
            aiString str;
            material->GetTexture(aiTextureType_BASE_COLOR, 0, &str);
            albedo = Texture_Manager::load_from_path(path.substr(0, path.size() - 10) + str.C_Str());
        }        
        
        if (material->GetTextureCount(aiTextureType_NORMALS)) {
            aiString str;
            material->GetTexture(aiTextureType_NORMALS, 0, &str);
            normal = Texture_Manager::load_from_path(path.substr(0, path.size() - 10) + str.C_Str());
        }

        if (material->GetTextureCount(aiTextureType_UNKNOWN)) {
            // Try to find metallic-roughness texture by name patterns
            for (uint32_t i = 0; i < material->GetTextureCount(aiTextureType_UNKNOWN); i++) {
                aiString str;
                material->GetTexture(aiTextureType_UNKNOWN, i, &str);
                std::string texName = str.C_Str();
                
                if (texName.find("metallic") != std::string::npos ||
                    texName.find("roughness") != std::string::npos ||
                    texName.find("orm") != std::string::npos) { // ORM = Occlusion/Roughness/Metallic
                    metrough = Texture_Manager::load_from_path(path.substr(0, path.size() - 10) + str.C_Str());
                    break;
                }
            }
        }
   
    }

    Material material(albedo, normal, metrough, occ, emis);
    return Mesh(vertices, indices, material);
}  

void Model::calculate_aabb() 
{
    aabb.min = glm::vec3(FLT_MAX);
    aabb.max = glm::vec3(-FLT_MAX);

    for (auto &m : meshes) {
        // todo when mesh has aabb just use that instead of all verts
        for (auto &v : m.vertices) {
            aabb.min.x = std::min(aabb.min.x, v.position.x);
            aabb.min.y = std::min(aabb.min.y, v.position.y);
            aabb.min.z = std::min(aabb.min.z, v.position.z);

            aabb.max.x = std::max(aabb.max.x, v.position.x);
            aabb.max.y = std::max(aabb.max.y, v.position.y);
            aabb.max.z = std::max(aabb.max.z, v.position.z);
        }
    }

}

void Model::shift_mesh_up()
{
    glm::vec3 center = 0.5f * (aabb.min + aabb.max);

    //glm::vec3 diff   = aabb.max - aabb.min;
    //float maxDim     = std::max(diff.x, std::max(diff.y, diff.z));
    //if (maxDim < 1e-8f) {
    //    maxDim = 1.0f;
    //}
    ////float scale_f = scale / maxDim;  // so the largest dimension goes from -1 to +1
    //float scale_f = 1.0f; // dont scale, just center

    //for (auto& m : meshes) {
    //    for (auto& v : m.vertices) {
    //        v.position = v.position - center;
    //        // Then shift Y so bottom is at y=0
    //        //v.Position.y += (center.y - aabb_min.y) * scale_f; // why ?>?????? todo figure out bruh
    //    }
    //    m.update_vertex_buffer();
    //}
}
