#include "asset/model_manager.h"

#include "glow_config.h"

#include "asset/animated_model.h"
#include "asset/texture_manager.h"
#include "asset/material_manager.h"
#include "asset/shader_manager.h"

#include "core/opengl.h"
#include "scene/scene.h"

#include "util/aabb.h"
#include "util/profiler.h"

#include <stb_image.h>
#include <assimp/GltfMaterial.h>
//#define CGLTF_IMPLEMENTATION
//#include <cgltf.h>

#include <cstdint>
#include <string>
#include <cassert>
#include <algorithm>
#include <set>
#include <memory>

struct Vertex {
    vec4 position; // normal x in w
    //vec3 normal;
    vec4 tangent; // normal y in w
    vec4 bitangent; // normal z in w
    vec2 tex_coords;
    uint32_t padding[2];
};

//struct Rigged_Vertex {
//    vec3 position;
//    vec3 normal;
//    vec2 tex_coords;
//    vec3 tangent;
//    vec3 bitangent;
//    uvec4 bone_ids; // index into global bone array, todo maybe more than 4 bones
//    vec4 bone_weights;
//};

struct Rigged_Vertex {
    vec4 position; // normal x in w
    vec4 tangent;  // normal y in w
    vec4 bitangent; // normal z in w
    uvec4 bone_ids; // index into global bone array, todo maybe more than 4 bones
    vec4 bone_weights;
    vec2 tex_coords;
    uint32_t padding[2];
};

struct Bone { // cpu bone
    std::string name;
    uint32_t parent_bone; // idx into bone array of parent
    mat4 inverse_bind;
};

struct GPU_Bone { // combine this bone with animation data to get skinned bone
    mat4 inverse_bind;
    uint32_t parent_bone;
    uint32_t padding[3];
};

struct Temp_Bone { // struct that will be allocated to map vertices to bones, then nuked
    uint32_t bone_index; // index into global bones
    float bone_weight;
};

struct Animation {
    uint32_t base_bone_animation;
    uint32_t bone_animation_count;
    float duration;
    //bool loop;
};

struct Bone_Animation {
    uint32_t bone_index;
    uint32_t base_position_keyframe;
    uint32_t position_keyframe_count;
    uint32_t base_rotation_keyframe;
    uint32_t rotation_keyframe_count;
    uint32_t base_scale_keyframe;
    uint32_t scale_keyframe_count;
    uint32_t padding;
};

struct Animation_Command {
    uint32_t base_bone;
    uint32_t bone_count;
    uint32_t bone_offset;
    uint32_t base_leaf;

    uint32_t leaf_count;
    uint32_t base_bone_animation; // first bone animation, fill from g_animations
    uint32_t bone_animation_count; // num of bone animations
    float duration;

    uint32_t leaf_thread_offset;
    bool loop;
    uint32_t padding[2];
};

struct Position_Keyframe { // todo compress?
    vec3 position;
    float time;
};

struct Rotation_Keyframe { // todo pack
    quat rotation;
    float time;
    uint32_t padding[3];
};

struct Scale_Keyframe { // todo compress?
    vec3 scale;
    float time;
};

// todo skeleton, maybe not!

namespace Model_Manager {

    static std::string base_path;

    mat4 assimp_to_glm(const aiMatrix4x4& ai_mat) {
        return mat4(
            ai_mat.a1, ai_mat.b1, ai_mat.c1, ai_mat.d1,
            ai_mat.a2, ai_mat.b2, ai_mat.c2, ai_mat.d2,
            ai_mat.a3, ai_mat.b3, ai_mat.c3, ai_mat.d3,
            ai_mat.a4, ai_mat.b4, ai_mat.c4, ai_mat.d4
        );
    }

    // todo change to some kind of block manager thing so i cna get rid of cpu verticies when uploaded but still track them
    static uint32_t num_meshes = 0;
    static uint32_t num_animated_meshes = 0;
    
    static uint32_t vao, vbo, ebo;
    static std::vector<Vertex> g_vertices(0);
    static std::vector<uint32_t> g_indices(0);

    static uint32_t g_rigged_vertices_ssbo;
    static std::vector<Rigged_Vertex> g_rigged_vertices(0);

    static std::vector<Model> m_models(0);
    static std::vector<std::string> m_model_names(0);
    static std::vector<Animated_Model> m_animated_models(0);
    static std::vector<std::string> m_animated_model_names(0); // todo think about how to store names

    // bone data
    static std::vector<Bone> g_rigged_bones(0); // base bones! todo rename
    static uint32_t num_skinned_bones = 0; // number of output bones we need. if duplicate models then num_skinned_bones != num base bones
    static std::vector<mat4> absolute_transforms(0);
    static std::vector<mat4> skinned_bones(0);

    // animation data
    static std::vector<std::string> g_animation_names(0);
    static std::vector<Animation> g_animations(0);
    static std::vector<Bone_Animation> g_bone_animations(0);
    static std::vector<Position_Keyframe> position_keyframes(0);
    static std::vector<Rotation_Keyframe> rotation_keyframes(0);
    static std::vector<Scale_Keyframe> scale_keyframes(0);

    // gpu animation
    static std::vector<uint32_t> g_leaf_bones(0); // index of bone that is a leaf for walking up bone hierarchy
    static uint32_t num_leafs = 0;
    static uint32_t n_cmds = 0;
    std::vector<Animation_Command> cmds(0);

    void init(std::string path) {
        base_path = path;
        // model_handle mh = load_model("teapot.obj");

        // Bone b = { "default", 0xFFFFFFFF, mat4(1.0f) };
        // g_rigged_bones.push_back(b);
    }

    void cleanup() {
        // todo Lol
    }

    bool indirect_model_loaded(const std::string& full_path, model_handle& model_index) {
        for (size_t i = 0; i < m_model_names.size(); i++) {
            if (full_path == m_model_names[i]) {
                model_index = i;
                return true;
            }
        }
        return false;
    }

    bool animated_model_loaded(const std::string& full_path, model_handle& model_index) {
        for (int i = m_animated_model_names.size() - 1; i >= 0; i--) {
            if (full_path == m_animated_model_names[i]) {
                model_index = i;
                return true;
            }
        }
        return false;
    }

    model_handle load_model(const std::string& path) {
        printf("num verts before model %llu\n", g_vertices.size());
        printf("num idx before model %llu\n", g_indices.size());

        const std::string full_path = base_path + path;

        model_handle model_index;
        if (indirect_model_loaded(path, model_index))
            return model_index;

        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(full_path, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_ValidateDataStructure | aiProcess_PopulateArmatureData);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            printf("ERROR::ASSIMP::%s\n", import.GetErrorString());
            return false;
        }

        const std::string path_without_filename = full_path.substr(0, full_path.find_last_of("/") + 1);

        Model model_ind;
        model_ind.m_name = path;
        process_node(scene->mRootNode, scene, model_ind, path_without_filename, mat4(1.0f));

        model_ind.calculate_aabb();
        
        model_index = m_models.size();

        m_models.push_back(model_ind);
        m_model_names.push_back(path);

        printf("num verts after model %llu\n", g_vertices.size());
        printf("num idx after model %llu\n", g_indices.size());

        return model_index;
    }

    model_handle load_animated_model(const std::string& path) {
        printf("num verts before model %llu\n", g_vertices.size());
        printf("num rigged verts before model %llu\n", g_rigged_vertices.size());
        printf("num rigged idx before model %llu\n", g_indices.size());
        printf("num base bones after model %llu\n", g_rigged_bones.size());
        printf("num total bones after model %du\n", num_skinned_bones);

        const std::string full_path = base_path + path;

        model_handle model_index;
        if (animated_model_loaded(path, model_index)) {
            printf("OYYOYOYOYOYOYOOY\n\n\n\n\nYOOO");
            Animated_Model loaded = m_animated_models[model_index];
            //num_skinned_bones += model.bone_count;

            Animated_Model copy {};

            // copy all draw vertices
            for (const Mesh& m : loaded.m_meshes) {
                // copy.m_meshes = loaded.m_meshes; // todo change when duplicating verts
                printf("copying mesh: %s\n", m.name.c_str());
                Mesh mesh = m;

                mesh.base_vertex = g_vertices.size();
                
                for (uint32_t i = 0; i < m.vertex_count; i++) {
                    g_vertices.push_back(g_vertices[m.base_vertex + i]);
                }
                
                copy.add_mesh(mesh);
                printf("size: %d\n", copy.m_meshes.size());
                // mesh.vertex_count = m.vertex_count;
                // mesh.base_index = m.base_index;
                // mesh.index_count = m.index_count;
                // struct Mesh {
                //     uint32_t base_vertex;
                //     uint32_t vertex_count;
                //     uint32_t base_index;
                //     uint32_t index_count;
                //     Util::AABB aabb;
                //     std::string name;

                //     // parent? 
                //     mat4 transform; // relative to parent

                //     Material material;

                //     vec4 bounding_sphere;
                // };
            }

            copy.m_aabb = loaded.m_aabb;
            copy.base_bone = loaded.base_bone;
            copy.bone_count = loaded.bone_count;
            copy.bone_offset = num_skinned_bones - copy.base_bone; assert(copy.bone_offset >= 0);
            num_skinned_bones += copy.bone_count;
            copy.base_leaf = loaded.base_leaf;
            copy.leaf_count = loaded.leaf_count;
            copy.base_animation = loaded.base_animation;
            copy.animation_count = loaded.animation_count;
            // todo copy base animation vertex
            copy.base_animation_vertex = loaded.base_animation_vertex;
            copy.animation_offset = copy.m_meshes[0].base_vertex - loaded.base_animation_vertex;

            model_index = m_animated_models.size();
            m_animated_models.push_back(copy);
            m_animated_model_names.push_back(path);

            //printf("offset: %d, tot: %d\n", copy.bone_offset, num_skinned_bones);
            printf("num verts after model %llu\n", g_vertices.size());
            printf("num rigged verts after model %llu\n", g_rigged_vertices.size());
            printf("num idx after model %llu\n", g_indices.size());
            printf("num base bones after model %llu\n", g_rigged_bones.size());
            printf("num total bones after model %du\n", num_skinned_bones);

            printf("original base vertex: %d, original base rigged vertex: %d, offset: %d\n", loaded.m_meshes[0].base_vertex, loaded.base_animation_vertex, loaded.animation_offset);
            printf("new base vertex: %d, new base rigged vertex: %d, offset: %d\n", copy.m_meshes[0].base_vertex, copy.base_animation_vertex, copy.animation_offset);
            // leaf bones
            // maybe kf's
            printf("here\n");
            printf("num animations loaded %llu\n", g_animations.size());
            printf("not here\n");

            return model_index;
        }

        Assimp::Importer import;
        import.SetPropertyBool(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);
        const aiScene* scene = import.ReadFile(full_path, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            printf("ERROR::ASSIMP::%s\n", import.GetErrorString());
            return false;
        }

        print_node_hierarchy(scene->mRootNode, 0);

        const std::string path_without_filename = full_path.substr(0, full_path.find_last_of("/") + 1);

        Animated_Model model{};
        model.animation_time = 0.0f;
        model.m_name = path;
        model.base_animation_vertex = g_rigged_vertices.size();

        model.base_bone = g_rigged_bones.size();

        process_animated_node(scene->mRootNode, scene, model, path_without_filename, mat4(1.0f), model.base_bone);

        create_fake_bones_for_animation_targets(scene, model.base_bone);

        model.bone_count = g_rigged_bones.size() - model.base_bone;
        model.bone_offset = num_skinned_bones - model.base_bone; assert(model.bone_offset >= 0);
        num_skinned_bones += model.bone_count;

        assert(model.bone_count != 0);

        update_bone_parents(scene, model.base_bone, g_rigged_bones.size());
        model.base_leaf = g_leaf_bones.size();
        add_leaf_bones(model.base_bone, g_rigged_bones.size());
        model.leaf_count = g_leaf_bones.size() - model.base_leaf;

        print_animated_model_info(model);
        printf("PRINTING BONE TREE FOR %s, bone: %d, to bone: %d\n", model.m_name.c_str(), model.base_bone, model.bone_count);
        print_bone_tree(model.base_bone, model.bone_count);

        if (scene->mNumAnimations > 0) {
            printf("LOADING ANIMATIONS\n");

            model.base_animation = g_animations.size();
            load_animations_from_scene(scene, model.base_bone);
            model.animation_count = g_animations.size() - model.base_animation;

            printf("base bone %d\n", model.base_bone);
            printf("bone count%d\n", model.bone_count);
            printf("bone offset%d\n", model.bone_offset);
            printf("base leaf%d\n", model.base_leaf);
            printf("leaf count %d\n", model.leaf_count);
            printf("base animation %d\n", model.base_animation);
            printf("animation count %d\n", model.animation_count);
        }


        model.calculate_aabb();

        model_index = m_animated_models.size();

        m_animated_models.push_back(model);
        m_animated_model_names.push_back(path);

        printf("num rigged verts after model %llu\n", g_rigged_vertices.size());
        printf("num idx after model %llu\n", g_indices.size());
        printf("num bones after model %llu\n", g_rigged_bones.size());
        // leaf bones
        // maybe kf's
        //printf("num animations loaded %d\n", g_animations.size());
        return model_index;
    }

    void compare_animation_data(uint32_t first, uint32_t second) {
        return;
        printf("%llu\n", m_animated_models.size());
        Animated_Model a = m_animated_models[0];
        Animated_Model b = m_animated_models[1];

        //static std::vector<Bone> g_rigged_bones(0); // one set of bones can produce many sets of skinned
        //static std::vector<uint32_t> g_leaf_bones(0);
        //static uint32_t num_skinned_bones = 0;

        //uint32_t base_bone;
        //uint32_t bone_count;
        //uint32_t bone_offset; // difference between base bones (1 set for all meshes) to skinned bones (1 set per mesh)
        //uint32_t base_leaf;
        //uint32_t leaf_count;
        //uint32_t base_animation;
        //uint32_t animation_count;

        assert(a.bone_count == b.bone_count);
        assert(a.leaf_count == b.leaf_count);
        assert(a.animation_count == b.animation_count);

        for (uint32_t i = 0; i < a.bone_count; i++) {
            printf("A: %s, B:%s\n", g_rigged_bones[a.base_bone + i].name.c_str(), g_rigged_bones[b.base_bone + i].name.c_str());
            assert(g_rigged_bones[a.base_bone + i].name == g_rigged_bones[b.base_bone + i].name);

            assert(g_rigged_bones[a.base_bone + i].inverse_bind == g_rigged_bones[b.base_bone + i].inverse_bind);

            printf("A: %d, B: %d\n", g_rigged_bones[a.base_bone + i].parent_bone, g_rigged_bones[b.base_bone + i].parent_bone);
            //assert(g_rigged_bones[a.base_bone + i].parent_bone == g_rigged_bones[b.base_bone + i].parent_bone);

            if (g_rigged_bones[b.base_bone + i].parent_bone != 0xFFFFFFFF) {

                printf("a bone [%d, %s], parent: [%d, %s]\n", a.base_bone + i, g_rigged_bones[a.base_bone + i].name.c_str(), g_rigged_bones[b.base_bone + i].parent_bone,
                                                            g_rigged_bones[g_rigged_bones[a.base_bone + i].parent_bone].name.c_str());

                printf("b bone [%d, %s], parent: [%d, %s]\n", b.base_bone + i, g_rigged_bones[b.base_bone + i].name.c_str(), g_rigged_bones[b.base_bone + i].parent_bone,
                g_rigged_bones[g_rigged_bones[b.base_bone + i].parent_bone].name.c_str());
            }

        }

        // for every animation
        //uint32_t base_animation;
        //uint32_t animation_count;

        assert(a.animation_count == b.animation_count);

        for (uint32_t i = 0; i < a.animation_count; i++) {
            Animation aa = g_animations[a.base_animation + i];
            Animation ab = g_animations[b.base_animation + i];
            std::string aan = g_animation_names[a.base_animation + i];
            std::string abn = g_animation_names[b.base_animation + i];

            //float duration;

            printf("A: %s, B: %s", aan.c_str(), abn.c_str());
            assert(aan == abn);

            uint32_t bba_a = aa.base_bone_animation;
            uint32_t bac_a = aa.bone_animation_count;

            uint32_t bba_b = ab.base_bone_animation;
            uint32_t bac_b = ab.bone_animation_count;

            //assert(bba_a == bba_b);
            printf("animation %d, A has %d, B has %d", i, bac_a, bac_b);
            assert(bac_a == bac_b);

            for (uint32_t j = 0; j < bac_a; j++) {

                Bone_Animation a_bone_anim = g_bone_animations[bba_a + j];
                Bone_Animation b_bone_anim = g_bone_animations[bba_b + j];

                std::string bone_a_n = g_rigged_bones[a_bone_anim.bone_index].name;
                std::string bone_a_b = g_rigged_bones[b_bone_anim.bone_index].name;
                
                assert(bone_a_n == bone_a_b);

                //uint32_t base_position_keyframe;
                //uint32_t position_keyframe_count;
                //uint32_t base_rotation_keyframe;
                //uint32_t rotation_keyframe_count;
                //uint32_t base_scale_keyframe;
                //uint32_t scale_keyframe_count;
            }
        }
    }

    void add_vertices(const std::vector<float> positions) {
        assert((positions.size() % 3) == 0);

        g_vertices.reserve(g_vertices.size() + positions.size());

        for (size_t i = 0; i < positions.size(); i += 3) {
            float v1 = positions[i];
            float v2 = positions[i + 1];
            float v3 = positions[i + 2];

            Vertex v = {
                .position = vec4(v1, v2, v3, 0.0f),
                .tangent = vec4(0.0f),
                .bitangent = vec4(0.0f),
                .tex_coords = vec2(0.0f)
            };

            g_vertices.push_back(v);
        }
    }

    void process_node(aiNode* node, const aiScene* scene, Model& model, const std::string& path, const mat4& parent_transform) {
        mat4 current_transform = parent_transform * assimp_to_glm(node->mTransformation);

        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];

            Mesh mesh = process_mesh(ai_mesh, scene, path);

            mesh.transform = current_transform;
            mesh.aabb = Util::transform_aabb(mesh.aabb, mesh.transform);

            model.add_mesh(mesh);
            num_meshes++;
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            process_node(node->mChildren[i], scene, model, path, current_transform);
        }
    }

    void process_animated_node(aiNode* node, const aiScene* scene, Animated_Model& model, const std::string& path, const mat4& parent_transform, uint32_t base_bone) {
        mat4 current_transform = parent_transform * assimp_to_glm(node->mTransformation);

        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];

            Mesh mesh = process_animated_mesh(ai_mesh, scene, path, base_bone);

            mesh.transform = current_transform;
            mesh.aabb = Util::transform_aabb(mesh.aabb, mesh.transform);

            model.add_mesh(mesh);
            num_animated_meshes++;
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            process_animated_node(node->mChildren[i], scene, model, path, current_transform, base_bone);
        }
    }

    Mesh process_mesh(const aiMesh* mesh, const aiScene* scene, const std::string& path) {
        Mesh mesh_ind = { 0 };
        mesh_ind.name = std::string(mesh->mName.C_Str());
        printf("loading mesh %s\n", mesh_ind.name.c_str());

        mesh_ind.aabb.min = vec3(FLT_MAX);
        mesh_ind.aabb.max = vec3(-FLT_MAX);

        mesh_ind.base_vertex = g_vertices.size();
        // uint32_t vertex_count = mesh->mNumVertices;
        mesh_ind.vertex_count = mesh->mNumVertices;

        g_vertices.reserve(g_vertices.size() + mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            
            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;

            mesh_ind.aabb.min = min(vec3(vertex.position), vec3(mesh_ind.aabb.min));
            mesh_ind.aabb.max = max(vec3(vertex.position), vec3(mesh_ind.aabb.max));

            if (mesh->HasNormals()) {
                vertex.position.w = mesh->mNormals[i].x;
                vertex.tangent.w = mesh->mNormals[i].y;
                vertex.bitangent.w = mesh->mNormals[i].z;
            }
            else {
                vertex.position.w = 0;
                vertex.tangent.w = 0;
                vertex.bitangent.w = 0;
            }

            if (mesh->mTextureCoords[0]) {
                vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.tex_coords = vec;

                const aiVector3D& pTangent = mesh->mTangents[i];
                vertex.tangent.x = pTangent.x;
                vertex.tangent.y = pTangent.y;
                vertex.tangent.z = pTangent.z;

                const aiVector3D& pBitangent = mesh->mBitangents[i];
                vertex.bitangent.x = pBitangent.x;
                vertex.bitangent.y = pBitangent.y;
                vertex.bitangent.z = pBitangent.z;
            }
            else {
                vertex.tex_coords = vec2(0.0f, 0.0f);
                vertex.tangent.x = 0;
                vertex.tangent.y = 0;
                vertex.tangent.z = 0;
                vertex.bitangent.x = 0;
                vertex.bitangent.y = 0;
                vertex.bitangent.z = 0;
            }

            g_vertices.push_back(vertex);
        }

        // process indices
        mesh_ind.base_index = g_indices.size();
        
        uint32_t num_idcs = 0;
        for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
                g_indices.push_back(face.mIndices[j]); // todo probably just reserver size and copy big chunk

            num_idcs += face.mNumIndices;
        }
        mesh_ind.index_count = num_idcs;

        // process material
        mesh_ind.material = load_material(mesh, scene, path);

        return mesh_ind;
    }

    Mesh process_animated_mesh(const aiMesh* mesh, const aiScene* scene, const std::string& path, uint32_t base_bone) {
        Mesh mesh_ind = { 0 };
        mesh_ind.name = std::string(mesh->mName.C_Str());
        printf("loading RIGGED mesh %s\n", mesh_ind.name.c_str());
        //mesh_ind.rigged = true;

        mesh_ind.aabb.min = vec3(FLT_MAX);
        mesh_ind.aabb.max = vec3(-FLT_MAX);

        uint32_t num_vertices = mesh->mNumVertices;
        mesh_ind.vertex_count = num_vertices;

        //// process bones
        // create mapping from verticies to bones
        // [v0 -> [b1, b2, b3, ...], v1 -> [b1, b2], v2 -> [b1]]
        std::vector<std::vector<Temp_Bone>> vertex_bone_influences(num_vertices);

        for (uint32_t bone_idx = 0; bone_idx < mesh->mNumBones; bone_idx++) {
            const aiBone* bone = mesh->mBones[bone_idx];

            // add to g_bones if doesnt exist, or return idx to it
            uint32_t global_bone_idx = find_or_create_global_bone(bone, scene, base_bone);

            for (uint32_t weight_idx = 0; weight_idx < bone->mNumWeights; weight_idx++) {
                uint32_t vertex_id = bone->mWeights[weight_idx].mVertexId;
                float weight = bone->mWeights[weight_idx].mWeight;

                //if (weight > 0.0f) {
                vertex_bone_influences[vertex_id].push_back({ global_bone_idx, weight });
                //}
            }
        }

        // base vertex is in the single buffer
        mesh_ind.base_vertex = g_vertices.size();
        // base vertex in the buffer to be used for animation

        // base animation vertex is in the buffer used to skin 
        // pre animation buffer

        g_rigged_vertices.reserve(g_rigged_vertices.size() + num_vertices);
        g_vertices.reserve(g_vertices.size() + num_vertices);

        for (uint32_t i = 0; i < num_vertices; i++) {
            Rigged_Vertex vertex = {};

            vertex.position = vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 0.0f);

            mesh_ind.aabb.min = min(vec3(vertex.position), vec3(mesh_ind.aabb.min));
            mesh_ind.aabb.max = max(vec3(vertex.position), vec3(mesh_ind.aabb.max));

            if (mesh->HasNormals()) {
                vertex.position.w = mesh->mNormals[i].x;
                vertex.tangent.w = mesh->mNormals[i].y;
                vertex.bitangent.w = mesh->mNormals[i].z;
            }
            else {
                vertex.position.w = 0;
                vertex.tangent.w = 0;
                vertex.bitangent.w = 0;
            }

            if (mesh->mTextureCoords[0]) {
                vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.tex_coords = vec;

                const aiVector3D& pTangent = mesh->mTangents[i];
                vertex.tangent.x = pTangent.x;
                vertex.tangent.y = pTangent.y;
                vertex.tangent.z = pTangent.z;

                const aiVector3D& pBitangent = mesh->mBitangents[i];
                vertex.bitangent.x = pBitangent.x;
                vertex.bitangent.y = pBitangent.y;
                vertex.bitangent.z = pBitangent.z;
            }
            else {
                vertex.tex_coords = vec2(0.0f, 0.0f);
                vertex.tangent.x = 0;
                vertex.tangent.y = 0;
                vertex.tangent.z = 0;
                vertex.bitangent.x = 0;
                vertex.bitangent.y = 0;
                vertex.bitangent.z = 0;
            }

            // process bones
            std::vector<Temp_Bone>& vertex_bones = vertex_bone_influences[i];
            if (vertex_bones.size() > 4) {
                // maybe do something
                printf("\n\n-----SUS MORE THAN 4 BONES [%llu]-----\n\n", vertex_bones.size());
            }

            uint32_t num_bones = vertex_bones.size() > 4 ? 4 : vertex_bones.size();
            std::sort(vertex_bones.begin(), vertex_bones.end(), [](Temp_Bone f1, Temp_Bone f2) { return f1.bone_weight > f2.bone_weight; });
            for (uint32_t i = 0; i < num_bones; i++) {
                vertex.bone_ids[i] = vertex_bones[i].bone_index; // this is a global bone index
                vertex.bone_weights[i] = vertex_bones[i].bone_weight;
            }
            float total_weight = vertex.bone_weights[0] + vertex.bone_weights[1] + vertex.bone_weights[2] + vertex.bone_weights[3];
            if (total_weight > 0.0f) {
                vertex.bone_weights[0] /= total_weight;
                vertex.bone_weights[1] /= total_weight;
                vertex.bone_weights[2] /= total_weight;
                vertex.bone_weights[3] /= total_weight;
            }

            g_rigged_vertices.push_back(vertex);

            Vertex v = { vertex.position, vertex.tangent, vertex.bitangent, vertex.tex_coords};
            g_vertices.push_back(v);
        }

        // process indices
        mesh_ind.base_index = g_indices.size();

        uint32_t num_idcs = 0;
        for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
                g_indices.push_back(face.mIndices[j]); // todo probably just reserver size and copy big chunk

            num_idcs += face.mNumIndices;
        }
        mesh_ind.index_count = num_idcs;

        //// process material
        mesh_ind.material = load_material(mesh, scene, path);

        return mesh_ind;
    }

    Material load_material(const aiMesh* mesh, const aiScene* scene, const std::string& path) {
        Material mesh_mat = { 0 };
        
        Defaults def = Texture_Manager::get_defaults();
        //mesh_mat.albedo = def.albedo;
        mesh_mat.normal = def.normal;
        mesh_mat.emissive = def.emissive;
        mesh_mat.met_rough = def.met_rough;
        mesh_mat.amb_occ = def.ao;

         printf("path is :%s\n", path.c_str());
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            //#define AI_MATKEY_BASE_COLOR "$clr.base", 0, 0

            if (material->GetTextureCount(aiTextureType_BASE_COLOR)) {
                aiString str;
                material->GetTexture(aiTextureType_BASE_COLOR, 0, &str);
                
                mesh_mat.albedo = Texture_Manager::load(path + str.C_Str());
            }

            if (material->GetTextureCount(aiTextureType_NORMALS)) {
                aiString str;
                material->GetTexture(aiTextureType_NORMALS, 0, &str);

                mesh_mat.normal = Texture_Manager::load(path + str.C_Str());
            }

            // todo why metallic 1?
            float metallic = 0.0f;
            float roughness = 1.0f;
            if (material->GetTextureCount(aiTextureType_GLTF_METALLIC_ROUGHNESS)) {
                aiString str;
                material->GetTexture(aiTextureType_GLTF_METALLIC_ROUGHNESS, 0, &str);
                // printf("MET ROUGHESNSENSENESNESNE %s\n", str.C_Str());
                mesh_mat.met_rough = Texture_Manager::load(path + str.C_Str());

                aiReturn metallicResult = material->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
                aiReturn roughnessResult = material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
            }
            mesh_mat.metallic_factor = metallic;
            mesh_mat.roughness_factor = roughness;

            // printf("MET: %f, ROG: %f\n", metallic, roughness);

            // aiTextureType_EMISSIVE
            if (material->GetTextureCount(aiTextureType_EMISSIVE)) {
                aiString str;
                material->GetTexture(aiTextureType_EMISSIVE, 0, &str);
                
                mesh_mat.emissive = Texture_Manager::load(path + str.C_Str());
            }

            aiColor3D emissiveColor;
            if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS) {
                float strength = 1.0f;
                aiReturn intensityResult = material->Get(AI_MATKEY_EMISSIVE_INTENSITY, strength);

                mesh_mat.emissive_factor = vec4(emissiveColor.r, emissiveColor.g, emissiveColor.b, strength);
                //printf("Emissive color: R=%.3f G=%.3f B=%.3f, w=%f\n", emissiveColor.r, emissiveColor.g, emissiveColor.b, strength);
            }
            else {
                mesh_mat.emissive_factor = vec4(1.0f);
            }

            aiColor4D pbrBaseColor(0.0f, 0.0f, 0.0f, 0.0f);
            if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &pbrBaseColor) == AI_SUCCESS) {
                mesh_mat.base_color = vec4(pbrBaseColor.r, pbrBaseColor.g, pbrBaseColor.b, pbrBaseColor.a);
            }
            else {
                mesh_mat.base_color = vec4(1.0f);
            }

            //  AO map
            if (material->GetTextureCount(aiTextureType_LIGHTMAP)) {
                aiString str;
                material->GetTexture(aiTextureType_LIGHTMAP, 0, &str);

                mesh_mat.amb_occ = Texture_Manager::load(path + str.C_Str());
            }

            float alpha_cutoff = 0.5f;
            float opacity;
            if (AI_SUCCESS == material->Get(AI_MATKEY_OPACITY, opacity)) {
                alpha_cutoff = opacity;
            }

            float transparency;
            if (AI_SUCCESS == material->Get(AI_MATKEY_TRANSPARENCYFACTOR, transparency)) {
                alpha_cutoff = 1.0f - transparency;
            }

            float gltf_ac;
            if (AI_SUCCESS == material->Get(AI_MATKEY_GLTF_ALPHACUTOFF, gltf_ac)) {
                alpha_cutoff = gltf_ac;
            }

            // todo grab this 
            //AI_MATKEY_BLEND_FUNC
            mesh_mat.blend_mode = Blend_Mode::disabled;

            aiString alpha_mode;
            if (AI_SUCCESS == material->Get(AI_MATKEY_GLTF_ALPHAMODE, alpha_mode)) {
                if (strcmp(alpha_mode.C_Str(), "OPAQUE") == 0) {
                    alpha_cutoff = 0.0f;
                }
                //else if (strcmp(alpha_mode.C_Str(), "MASK") == 0) {
                //    float gltf_ac;
                //    if (AI_SUCCESS == material->Get(AI_MATKEY_GLTF_ALPHACUTOFF, gltf_ac)) {
                //        alpha_cutoff = gltf_ac;
                //    }
                //}
                else if (strcmp(alpha_mode.C_Str(), "BLEND") == 0) {
                    alpha_cutoff = 0.01f;
                    mesh_mat.blend_mode = Blend_Mode::blend;
                    // printf("\n\n\nyesblend\n\n\n");
                }
            }

            mesh_mat.alpha_cutoff = alpha_cutoff;
        }

        return mesh_mat;
    }

    uint32_t find_or_create_global_bone(const aiBone* bone, const aiScene* scene, uint32_t base_bone) {
        std::string bone_name(bone->mName.C_Str());
        
        // base bone is g_bones.size() before loading a model
        for (uint32_t i = base_bone; i < g_rigged_bones.size(); i++) {
            if (g_rigged_bones[i].name == bone_name)
                return i;
        }

        // printf("[BONE] adding bone %s\n", bone_name.c_str());

        Bone new_bone;
        new_bone.name = bone_name;
        new_bone.inverse_bind = assimp_to_glm(bone->mOffsetMatrix);
        new_bone.parent_bone = UINT32_MAX;

        g_rigged_bones.push_back(new_bone);
        return g_rigged_bones.size() - 1;
    }

    void load_animations_from_scene(const aiScene* scene, uint32_t base_bone) {
        for (uint32_t anim_idx = 0; anim_idx < scene->mNumAnimations; anim_idx++) {
            aiAnimation* ai_anim = scene->mAnimations[anim_idx];

            Animation animation;
            animation.duration = static_cast<float>(ai_anim->mDuration / ai_anim->mTicksPerSecond);
            animation.base_bone_animation = g_bone_animations.size();

            printf("Loading animation: %s, duration: %.2f seconds\n", ai_anim->mName.C_Str(), animation.duration);

            for (uint32_t channel_idx = 0; channel_idx < ai_anim->mNumChannels; channel_idx++) {
                aiNodeAnim* ai_channel = ai_anim->mChannels[channel_idx];

                uint32_t bone_index = find_bone_index(ai_channel->mNodeName.C_Str(), base_bone);

                Bone_Animation bone_anim;
                bone_anim.bone_index = bone_index;
                bone_anim.base_position_keyframe = position_keyframes.size();
                bone_anim.base_rotation_keyframe = rotation_keyframes.size();
                bone_anim.base_scale_keyframe = scale_keyframes.size();

                load_keyframes_from_channel(ai_channel, ai_anim->mTicksPerSecond);

                bone_anim.position_keyframe_count = position_keyframes.size() - bone_anim.base_position_keyframe;
                bone_anim.rotation_keyframe_count = rotation_keyframes.size() - bone_anim.base_rotation_keyframe;
                bone_anim.scale_keyframe_count = scale_keyframes.size() - bone_anim.base_scale_keyframe;

                g_bone_animations.push_back(bone_anim);
            }

            animation.bone_animation_count = g_bone_animations.size() - animation.base_bone_animation;

            g_animations.push_back(animation);
            g_animation_names.push_back(std::string(ai_anim->mName.C_Str()));
        }
    }

    void load_keyframes_from_channel(aiNodeAnim* channel, double ticks_per_second) {
        for (uint32_t i = 0; i < channel->mNumPositionKeys; i++) {
            Position_Keyframe keyframe;
            aiVector3D pos = channel->mPositionKeys[i].mValue;
            keyframe.position = vec3(pos.x, pos.y, pos.z);
            keyframe.time = static_cast<float>(channel->mPositionKeys[i].mTime / ticks_per_second);
            position_keyframes.push_back(keyframe);
        }

        for (uint32_t i = 0; i < channel->mNumRotationKeys; i++) {
            Rotation_Keyframe keyframe;
            aiQuaternion rot = channel->mRotationKeys[i].mValue;
            keyframe.rotation = quat(rot.w, rot.x, rot.y, rot.z);
            keyframe.time = static_cast<float>(channel->mRotationKeys[i].mTime / ticks_per_second);
            rotation_keyframes.push_back(keyframe);
        }

        for (uint32_t i = 0; i < channel->mNumScalingKeys; i++) {
            Scale_Keyframe keyframe;
            aiVector3D scale = channel->mScalingKeys[i].mValue;
            keyframe.scale = vec3(scale.x, scale.y, scale.z);
            keyframe.time = static_cast<float>(channel->mScalingKeys[i].mTime / ticks_per_second);
            scale_keyframes.push_back(keyframe);
        }
    }

    uint32_t find_bone_index(const std::string& bone_name, uint32_t base_bone) {
        for (uint32_t i = base_bone; i < g_rigged_bones.size(); i++) {
            if (g_rigged_bones[i].name == bone_name) {
                // printf("MATCH %s | %s", bone_name.c_str(), g_rigged_bones[i].name.c_str());
                return i;
            }
        }
        printf("bone %s not found\n", bone_name.c_str());
        assert(false);
    }

    void create_fake_bones_for_animation_targets(const aiScene* scene, uint32_t base_bone) {
        if (!scene->mNumAnimations) 
            return;
        
        printf("[BONE] Scanning animations for non-bone targets...\n");
        
        std::set<std::string> animation_targets;
        
        for (uint32_t anim_idx = 0; anim_idx < scene->mNumAnimations; anim_idx++) {
            const aiAnimation* animation = scene->mAnimations[anim_idx];
            
            for (uint32_t channel_idx = 0; channel_idx < animation->mNumChannels; channel_idx++) {
                const aiNodeAnim* channel = animation->mChannels[channel_idx];
                std::string target_name(channel->mNodeName.C_Str());
                animation_targets.insert(target_name);
            }
        }
        
        for (const std::string& target : animation_targets) {
            bool is_existing_bone = false;
            
            for (uint32_t i = base_bone; i < g_rigged_bones.size(); i++) {
                if (g_rigged_bones[i].name == target) {
                    is_existing_bone = true;
                    break;
                }
            }
            
            if (!is_existing_bone) {
                uint32_t bone = find_or_create_fake_bone(target, scene, base_bone);
            }
        }
    }

    uint32_t find_or_create_fake_bone(const std::string& node_name, const aiScene* scene, uint32_t base_bone) {
        for (uint32_t i = base_bone; i < g_rigged_bones.size(); i++) {
            if (g_rigged_bones[i].name == node_name) {
                return i;
            }
        }
        
        aiNode* target_node = find_node_by_name(scene->mRootNode, node_name);
        if (!target_node) {
            printf("[BONE] Warning: Could not find animation target node '%s'\n", node_name.c_str());
            return UINT32_MAX;
        }
        
        printf("[BONE] Creating fake bone for animation target: %s\n", node_name.c_str());
        
        Bone fake_bone = {};
        fake_bone.name = node_name;
        
        // inverse bind should be inverse world transform
        aiMatrix4x4 world_transform;
        get_world_transform(target_node, scene->mRootNode, world_transform);
        aiMatrix4x4 inverse_world = world_transform.Inverse();
        fake_bone.inverse_bind = assimp_to_glm(inverse_world);
        
        fake_bone.parent_bone = UINT32_MAX;
        
        g_rigged_bones.push_back(fake_bone);
        return g_rigged_bones.size() - 1;
    }

    void get_world_transform(aiNode* node, aiNode* root, aiMatrix4x4& out_transform) {
        out_transform = aiMatrix4x4();
        
        aiNode* current = node;
        while (current && current != root) {
            out_transform = current->mTransformation * out_transform;
            current = current->mParent;
        }
    }

    void setup_buffers() {
        // todo move me!
        absolute_transforms.resize(num_skinned_bones);
        skinned_bones.resize(num_skinned_bones);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        //glBufferData(GL_ARRAY_BUFFER, g_vertices.size() * sizeof(Vertex), &g_vertices[0], GL_DYNAMIC_DRAW);
        glBufferStorage(GL_ARRAY_BUFFER, g_vertices.size() * sizeof(Vertex), &g_vertices[0], GL_DYNAMIC_STORAGE_BIT);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, g_indices.size() * sizeof(uint32_t), &g_indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));

        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            assert(false);
            printf("OpenGL Error after: 0x%x\n", error);
        }

        glBindVertexArray(0);

        printf("Uploaded %zu vertices, %zu indices\n", g_vertices.size(), g_indices.size());

        // todo do same for animated stuff
        // vertex data, + ssbo for all bone data prob
        glCreateBuffers(1, &g_rigged_vertices_ssbo);
        glNamedBufferStorage(g_rigged_vertices_ssbo, sizeof(Rigged_Vertex) * g_rigged_vertices.size(), g_rigged_vertices.data(), GL_DYNAMIC_STORAGE_BIT);

        printf("Uploaded [rigged] %zu vertices\n", g_rigged_vertices.size());
    }

    //void upload_data() {
    //    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    //    glBufferData(GL_ARRAY_BUFFER,
    //        g_vertices.size() * sizeof(Vertex),
    //        g_vertices.data(),
    //        GL_STATIC_DRAW);
    //    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    //    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
    //        g_indices.size() * sizeof(uint32_t),
    //        g_indices.data(),
    //        GL_STATIC_DRAW);
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //}

    bool is_bone_name(const std::string& name, uint32_t base_bone) {
        for (uint32_t i = base_bone; i < g_rigged_bones.size(); i++) {
            if (g_rigged_bones[i].name == name) {
                return true;
            }
        }
        return false;
    }

    aiNode* find_node_by_name(aiNode* node, const std::string& name) {
        if (node->mName.C_Str() == name) {
            return node;
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            aiNode* found = find_node_by_name(node->mChildren[i], name);
            if (found) return found;
        }

        return nullptr;
    }

    uint32_t find_parent_bone_index(const std::string& bone_name, const aiScene* scene, uint32_t base_bone) {
        aiNode* bone_node = find_node_by_name(scene->mRootNode, bone_name);
        if (!bone_node || !bone_node->mParent) {
            return UINT32_MAX; // No parent or root node
        }

        aiNode* current_parent = bone_node->mParent;
        while (current_parent) {
            std::string parent_name(current_parent->mName.C_Str());

            if (is_bone_name(parent_name, base_bone)) {
                for (uint32_t i = base_bone; i < g_rigged_bones.size(); i++) {
                    if (g_rigged_bones[i].name == parent_name) {
                        return i;
                    }
                }
            }

            current_parent = current_parent->mParent;
        }

        return UINT32_MAX; // bone is parent
    }

    void update_bone_parents(const aiScene* scene, uint32_t base_bone, uint32_t end_bone) {
        printf("[BONE] Fixing parent relationships for bones %d to %d\n", base_bone, end_bone - 1);

        for (uint32_t i = base_bone; i < end_bone; i++) {
            uint32_t parent_idx = find_parent_bone_index(g_rigged_bones[i].name, scene, base_bone);
            g_rigged_bones[i].parent_bone = parent_idx;

            if (parent_idx != UINT32_MAX) {
                // printf("[BONE] %s parent is %s (index %d)\n",
                //     g_rigged_bones[i].name.c_str(),
                //     g_rigged_bones[parent_idx].name.c_str(),
                //     parent_idx);
            }
            else {
                printf("[BONE] %s is a root bone\n", g_rigged_bones[i].name.c_str());
            }
        }
    }

    void add_leaf_bones(uint32_t base_bone, uint32_t end_bone) {
        for (uint32_t bone = base_bone; bone < end_bone; bone++) {
            bool leaf = true;

            for (uint32_t other_bone = base_bone; other_bone < end_bone; other_bone++) {
                if (g_rigged_bones[other_bone].parent_bone == bone) {
                    leaf = false;
                    break;
                }
            }

            if (leaf) {
                g_leaf_bones.push_back(bone);
                // printf("bone: %s is a leaf\n", g_rigged_bones[bone].name.c_str());
            }
        }
    }

    void print_animated_model_info(const Animated_Model& m) {
        printf("-----------------------\n");
        printf("MODEL: %s\n", m.m_name.c_str());
        printf("Number meshes: %d\n", m.m_meshes.size());
        
        for (const Mesh& mm : m.m_meshes) {
            std::string name;
            printf(" mesh: %s\n", mm.name.c_str());

            printf("%f %f %f %f\n", mm.transform[0][0], mm.transform[1][0], mm.transform[2][0], mm.transform[3][0]);
            printf("%f %f %f %f\n", mm.transform[0][1], mm.transform[1][1], mm.transform[2][1], mm.transform[3][1]);
            printf("%f %f %f %f\n", mm.transform[0][2], mm.transform[1][2], mm.transform[2][2], mm.transform[3][2]);
            printf("%f %f %f %f\n", mm.transform[0][3], mm.transform[1][3], mm.transform[2][3], mm.transform[3][3]);
        }
    }

    void print_bone_tree(uint32_t base_bone, uint32_t num_bones) {
        std::unordered_map<int, std::vector<int>> childrenMap;

        for (int i = base_bone; i < base_bone + num_bones; ++i) {
            int parent = g_rigged_bones[i].parent_bone;
            if (parent != 0xFFFFFFFF)
                childrenMap[parent].push_back(i);
        }

        std::vector<int> roots;
        for (int i = base_bone; i < base_bone + num_bones; ++i) {
            if (g_rigged_bones[i].parent_bone == 0xFFFFFFFF)
                roots.push_back(i);
        }

        std::vector<int> linear_bones;

        for (int root : roots) {
            tree(childrenMap, root);
            dfs(root, childrenMap, linear_bones);
        }

        for (int i : linear_bones)
            printf("%s, ", g_rigged_bones[i].name.c_str());
        
        assert(num_bones == linear_bones.size());

        for (int i = base_bone, j = 0; i < base_bone + num_bones; ++i, ++j) {
            g_rigged_bones[i] = g_rigged_bones[linear_bones[j]];
        }
    }

    void dfs(int bone, const std::unordered_map<int, std::vector<int>>& childrenMap, std::vector<int>& linear_bones) {
        linear_bones.push_back(bone);

        auto it = childrenMap.find(bone);
        if (it != childrenMap.end()) {
            const std::vector<int>& children = it->second;
            for (size_t i = 0; i < children.size(); ++i) {
                dfs(children[i], childrenMap, linear_bones);
            }
        }
    }

    void tree(const std::unordered_map<int, std::vector<int>>& childrenMap, int boneIndex, const std::string& indent, bool isLast) {
        const Bone& bone = g_rigged_bones[boneIndex];

        std::cout << indent;
        if (!indent.empty()) {
            std::cout << (isLast ? "└─" : "├─");
        }
        std::cout << bone.name << "\n";

        auto it = childrenMap.find(boneIndex);
        if (it != childrenMap.end()) {
            const std::vector<int>& children = it->second;
            for (size_t i = 0; i < children.size(); ++i) {
                bool lastChild = (i == children.size() - 1);
                tree(childrenMap, children[i],
                            indent + (isLast ? "   " : "│  "),
                            lastChild);
            }
        }
    }

    void print_node_hierarchy(const aiNode* node, int depth) {
        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }
        std::cout << "Node: " << node->mName.C_Str() << std::endl;

        if (node->mNumMeshes > 0) {
            for (int i = 0; i < depth; ++i) {
                std::cout << "  ";
            }
            std::cout << "  Meshes: ";
            for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
                std::cout << node->mMeshes[i] << (i == node->mNumMeshes - 1 ? "" : ", ");
            }
            std::cout << std::endl;
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            print_node_hierarchy(node->mChildren[i], depth + 1);
        }
    }

    uint32_t animation_commands, bone_ssbo, skinned_bone_ssbo, pos_keys_ssbo, rot_keys_ssbo, scale_keys_ssbo, bone_animation_ssbo, leaf_bones_ssbo, absolute_bone_transform_ssbo, transform_time_ssbo;

    void setup_ssbos() {
        Shader_Manager::load_compute("animate_skeleton");
        Shader_Manager::load_compute("skin");

        glGenBuffers(1, &skinned_bone_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, skinned_bone_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, num_skinned_bones * sizeof(mat4), nullptr, GL_DYNAMIC_DRAW);
        //printf("Created bone SSBO with %zu bytes\n", num_skinned_bones * sizeof(mat4));

#if GPU_ANIMATION || DEBUG_SKELETON
        std::vector<GPU_Bone> rigged_bones_temp;
        rigged_bones_temp.reserve(g_rigged_bones.size());
        for (const Bone& b : g_rigged_bones)
            rigged_bones_temp.push_back({ b.inverse_bind, b.parent_bone });

        glGenBuffers(1, &bone_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bone_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, rigged_bones_temp.size() * sizeof(GPU_Bone), rigged_bones_temp.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry

        glGenBuffers(1, &absolute_bone_transform_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, absolute_bone_transform_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, absolute_transforms.size() * sizeof(mat4), nullptr, GL_DYNAMIC_DRAW);
#endif

#if GPU_ANIMATION
        std::vector<float> absolute_transform_times(num_skinned_bones);
        for (size_t i = 0; i < num_skinned_bones; i++) {
            //g_skinned_bones[i].transform = mat4(1.0f);
            absolute_transform_times[i] = 0.0f;
        }

        glGenBuffers(1, &transform_time_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, transform_time_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, absolute_transform_times.size() * sizeof(float), absolute_transform_times.data(), GL_DYNAMIC_DRAW);

        glGenBuffers(1, &animation_commands);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, animation_commands);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            cmds.size() * sizeof(Animation_Command),
            nullptr,
            GL_DYNAMIC_DRAW);
        
        //bone_animation_ssbo
        glGenBuffers(1, &bone_animation_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bone_animation_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, g_bone_animations.size() * sizeof(Bone_Animation), g_bone_animations.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry

        glGenBuffers(1, &pos_keys_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, pos_keys_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, position_keyframes.size() * sizeof(Position_Keyframe), position_keyframes.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry

        glGenBuffers(1, &rot_keys_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, rot_keys_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, rotation_keyframes.size() * sizeof(Rotation_Keyframe), rotation_keyframes.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry

        glGenBuffers(1, &scale_keys_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, scale_keys_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, scale_keyframes.size() * sizeof(Scale_Keyframe), scale_keyframes.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry

        //leaf_bones_ssbo;
        glGenBuffers(1, &leaf_bones_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, leaf_bones_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, g_leaf_bones.size() * sizeof(uint32_t), g_leaf_bones.data(), GL_DYNAMIC_DRAW); // prob not dynamic dry
#endif

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL error: 0x" << std::hex << err << std::endl;
        }
    }

    void begin_animation_frame() {
        cmds.clear();
        n_cmds = 0;
        num_leafs = 0;
    }
    
    uint32_t get_num_animation_commands() {
        return n_cmds;
    }

    //void submit_animation_command(Animation_Command cmd) {
    //    cmds.push_back(cmd);
    //    n_cmds++;
    //    num_leafs += cmd.leaf_count;
    //}

    void submit_animation_command(uint32_t model_id) {
        Animated_Model& m = m_animated_models[model_id];
        Animation a = g_animations[m.current_animation + m.base_animation];

        Animation_Command cmd = { m.base_bone, m.bone_count, m.bone_offset, m.base_leaf, m.leaf_count, a.base_bone_animation, a.bone_animation_count, a.duration, num_leafs };

        printf("adding Animation command %s for model %s\n", g_animation_names[m.current_animation + m.base_animation].c_str(), m.m_name.c_str());
        printf("  base_bone: %u\n", cmd.base_bone);
        printf("  bone_count: %u\n", cmd.bone_count);
        printf("  bone_offset: %u\n", cmd.bone_offset);
        printf("  base_leaf: %u\n", cmd.base_leaf);
        printf("  leaf_count: %u\n", cmd.leaf_count);
        printf("  base_bone_animation: %u\n", cmd.base_bone_animation);
        printf("  bone_animation_count: %u\n", cmd.bone_animation_count);
        printf("  duration: %.3f\n", cmd.duration);
        printf("  leaf_thread_offset: %u\n", cmd.leaf_thread_offset);

        cmds.push_back(cmd);
        n_cmds++;
        num_leafs += cmd.leaf_count;
    }

    void upload_animation_commands() {
        glNamedBufferData(animation_commands, cmds.size() * sizeof(Animation_Command), cmds.data(), GL_DYNAMIC_DRAW);

        /*

        printf("num: %d", n_cmds);
        printf("size: %d", (uint32_t)cmds.size());

        glNamedBufferSubData(animation_commands, 0, cmds.size() * sizeof(Animation_Command), cmds.data());

        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            printf("OpenGL Error after: 0x%x\n", error);
            assert(false);

        }

        */
    }

    void update_bones_from_animation_compute(float time) {
        //const Animation& anim = g_animations[animation_index]
        // printf("animating for %d leafs\n", num_leafs);
        // printf("num_animation_cmds %du\n", n_cmds);

        Compute_Shader* skeleton = Shader_Manager::get_compute("animate_skeleton");
        skeleton->use();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, animation_commands);
        // can do all these once if doing gpu skeletal transforms
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, leaf_bones_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bone_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, skinned_bone_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, absolute_bone_transform_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, transform_time_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bone_animation_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, pos_keys_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, rot_keys_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, scale_keys_ssbo);

         //printf("Dispatching animation at time: %.3f\n", time);

        skeleton->set_float("dispatch_time", time);
        skeleton->set_uint("num_animation_cmds", n_cmds);
        //skeleton->set_uint("max_leaf", (uint32_t)g_leaf_bones.size());
        //skeleton->set_uint("animation_count", (uint32_t)g_animations[0].bone_animations.size());

        skeleton->dispatch_and_wait((num_leafs + 63) / 64, 1, 1, GL_ALL_BARRIER_BITS);
    }

    void update_animated_vertices(Scene& scene) {
        PROFILE_SCOPE_COLOR("compute skin", legit::Colors::amethyst);

        Compute_Shader* skin = Shader_Manager::get_compute("skin");
        skin->use();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_rigged_vertices_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, scene.animated_mesh_to_all_mesh_mapping_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, scene.gpu_mesh_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, skinned_bone_ssbo);

        skin->set_uint("animated_count", (uint32_t)scene.animated_mesh_to_all_mesh_mapping.size());

        skin->dispatch_and_wait((scene.animated_mesh_to_all_mesh_mapping.size() + 31) / 32, 1, 1, GL_ALL_BARRIER_BITS);

        // for (int i = 0; i < scene.animated_mesh_to_all_mesh_mapping.size(); i++) {
        //     uint32_t mesh_idx = scene.animated_mesh_to_all_mesh_mapping[i];
        //     printf("Animated mesh %d: mesh_idx=%u, base_vertex=%d, vertex_count=%u, offset=%u\n", 
        //         i, mesh_idx, 
        //         scene.gpu_meshes[mesh_idx].base_vertex,
        //         scene.gpu_meshes[mesh_idx].vertex_count,
        //         scene.gpu_meshes[mesh_idx].skinned_to_static_offset);
        // }

        // skin->dispatch_and_wait((g_vertices.size() + 31) / 32, 1, 1, GL_ALL_BARRIER_BITS);

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        // GLenum error = glGetError();
        // if (error != GL_NO_ERROR) {
        //     assert(false);
        //     printf("OpenGL Error after: 0x%x\n", error);
        // }
    }

    int32_t find_bone_animation(uint32_t bone_idx, uint32_t base_bone_animation, uint32_t bone_animation_count) {
        for (int32_t i = base_bone_animation; i < base_bone_animation + bone_animation_count; i++) {
            if (g_bone_animations[i].bone_index == bone_idx) {
                return i;
            }
        }
        return -1;
    }


    vec3 interpolate_position(uint32_t anim_idx, float time) {
        if (anim_idx == -1)
            return vec3(0.0);
        
        Bone_Animation anim = g_bone_animations[anim_idx];
        
        if (anim.position_keyframe_count == 0)
            return vec3(0.0);
        
        if (anim.position_keyframe_count == 1)
            return position_keyframes[anim.base_position_keyframe].position;
        
        for (uint32_t i = 0; i < anim.position_keyframe_count - 1; i++) {
            uint32_t idx1 = anim.base_position_keyframe + i;
            uint32_t idx2 = anim.base_position_keyframe + i + 1;
            
            float time1 = position_keyframes[idx1].time;
            float time2 = position_keyframes[idx2].time;
            
            if (time >= time1 && time <= time2) {
                float t = (time - time1) / (time2 - time1);
                return mix(position_keyframes[idx1].position, position_keyframes[idx2].position, t);
            }
        }
        
        return position_keyframes[anim.base_position_keyframe + anim.position_keyframe_count - 1].position;
    }

    quat interpolate_rotation(uint32_t anim_idx, float time) {
        if (anim_idx == -1)
            return quat(1.0f, 0.0f, 0.0f, 0.0f);
        
        Bone_Animation anim = g_bone_animations[anim_idx];
        
        if (anim.rotation_keyframe_count == 0)
            return quat(1.0f, 0.0f, 0.0f, 0.0f);
        
        if (anim.rotation_keyframe_count == 1)
            return rotation_keyframes[anim.base_rotation_keyframe].rotation;
        
        for (uint32_t i = 0; i < anim.rotation_keyframe_count - 1; i++) {
            uint32_t idx1 = anim.base_rotation_keyframe + i;
            uint32_t idx2 = anim.base_rotation_keyframe + i + 1;
            
            float time1 = rotation_keyframes[idx1].time;
            float time2 = rotation_keyframes[idx2].time;
            
            if (time >= time1 && time <= time2) {
                float t = (time - time1) / (time2 - time1);
                return slerp(rotation_keyframes[idx1].rotation, rotation_keyframes[idx2].rotation, t);
            }
        }
        
        return rotation_keyframes[anim.base_rotation_keyframe + anim.rotation_keyframe_count - 1].rotation;
    }

    vec3 interpolate_scale(uint32_t anim_idx, float time) {
        if (anim_idx == -1)
            return vec3(1.0);
        
        Bone_Animation anim = g_bone_animations[anim_idx];
        
        if (anim.scale_keyframe_count == 0)
            return vec3(1.0);
        
        if (anim.scale_keyframe_count == 1)
            return scale_keyframes[anim.base_scale_keyframe].scale;
        
        for (uint32_t i = 0; i < anim.scale_keyframe_count - 1; i++) {
            uint32_t idx1 = anim.base_scale_keyframe + i;
            uint32_t idx2 = anim.base_scale_keyframe + i + 1;
            
            float time1 = scale_keyframes[idx1].time;
            float time2 = scale_keyframes[idx2].time;
            
            if (time >= time1 && time <= time2) {
                float t = (time - time1) / (time2 - time1);
                return mix(scale_keyframes[idx1].scale, scale_keyframes[idx2].scale, t);
            }
        }
        
        return scale_keyframes[anim.base_scale_keyframe + anim.scale_keyframe_count - 1].scale;
    }

    void update_bones_from_animation(float time, model_handle model_id) {
        int i = 0;
        for (Animated_Model& am : m_animated_models) { // todo rm
            Animation& animation = g_animations[am.base_animation]; // todo change to model_id
            am.animation_time += time;
            if (am.animation_time >= animation.duration)
                am.animation_time = 0;

            // printf("i: %d, t: %f\n", i, am.animation_time);
            i++;

            for (uint32_t i = 0; i < am.bone_count; i++) {
                uint32_t bone_idx = am.base_bone + i;

                int bone_animation_idx = find_bone_animation(bone_idx, animation.base_bone_animation, animation.bone_animation_count);

                vec3 position = interpolate_position(bone_animation_idx, am.animation_time);
                quat rotation = interpolate_rotation(bone_animation_idx, am.animation_time);
                vec3 scale = interpolate_scale(bone_animation_idx, am.animation_time);

                mat4 translation = mat4(1.0);
                translation[3][0] = position.x;
                translation[3][1] = position.y;
                translation[3][2] = position.z;
                
                mat4 rotation_mat = mat4_cast(rotation);
                
                mat4 scale_mat = mat4(1.0);
                scale_mat[0][0] = scale.x;
                scale_mat[1][1] = scale.y;
                scale_mat[2][2] = scale.z;
    
                mat4 local_transform = translation * rotation_mat * scale_mat;

                mat4 absolute_transform;
                if (g_rigged_bones[bone_idx].parent_bone == 0xFFFFFFFF)
                    absolute_transform = local_transform;
                else
                    absolute_transform = absolute_transforms[g_rigged_bones[bone_idx].parent_bone + am.bone_offset] * local_transform;

                absolute_transforms[bone_idx + am.bone_offset] = absolute_transform;
                skinned_bones[bone_idx + am.bone_offset] = absolute_transform * g_rigged_bones[bone_idx].inverse_bind;
            }

            // upload skinned bones & absolute bones (debug)
            // skinned_bone_ssbo
            // absolute_bone_transform_ssbo
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, skinned_bone_ssbo);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, am.base_bone + am.bone_offset, sizeof(mat4) * am.bone_count, &skinned_bones[am.base_bone + am.bone_offset]);

#if DEBUG_SKELETON
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, absolute_bone_transform_ssbo);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, am.base_bone + am.bone_offset, sizeof(mat4) * am.bone_count, &absolute_transforms[am.base_bone + am.bone_offset]);
#endif
        }
    }

    void update_bones(float time) {
        PROFILE_SCOPE_COLOR("update bones", legit::Colors::wisteria);
#if GPU_ANIMATION
        // todo this should be absolute time rn I think 
        update_bones_from_animation_compute(time);
#else
        update_bones_from_animation(time, 0);
#endif
    }

    uint32_t get_bone_ssbo() {
        return bone_ssbo; 
    }

    uint32_t get_skinned_bone_ssbo() {
        return skinned_bone_ssbo; 
    }

    uint32_t get_absolute_bones() {
        return absolute_bone_transform_ssbo; 
    }

    uint32_t get_animation_command_ssbo() {
        return animation_commands;
    }

    uint32_t get_big_vao() {
        return vao;
    }

    uint32_t get_num_vertices() {
        return (uint32_t)g_vertices.size();
    }

    Model& get_model(uint32_t idx) {
        return m_models[idx];
    }

    Animated_Model& get_animated_model(uint32_t idx) {
        return m_animated_models[idx];
    }

    Util::AABB get_aabb_indirect(const model_handle& model_id) {
        return m_models[model_id].get_aabb();
    }

    uint32_t get_num_models() {
        return (uint32_t)m_models.size();
    }

    uint32_t get_num_animated_models() {
        return (uint32_t)m_animated_models.size();
    }

    std::string get_model_name(model_handle model_id, bool animated) {
        if (animated)
            return m_animated_model_names[model_id];
        else
            return m_model_names[model_id];
    }
}

//mat4 cgltf_to_glm(const cgltf_float* matrix) {
//    return mat4(
//        matrix[0], matrix[1], matrix[2], matrix[3],
//        matrix[4], matrix[5], matrix[6], matrix[7],
//        matrix[8], matrix[9], matrix[10], matrix[11],
//        matrix[12], matrix[13], matrix[14], matrix[15]
//    );
//}
//
//vec3 cgltf_to_vec3(const cgltf_float* data) {
//    return vec3(data[0], data[1], data[2]);
//}
//
//vec2 cgltf_to_vec2(const cgltf_float* data) {
//    return vec2(data[0], data[1]);
//}
//
//quat cgltf_to_quat(const cgltf_float* data) {
//    return quat(data[3], data[0], data[1], data[2]); // w, x, y, z
//}

//model_handle load_model_cgltf(const std::string& path) {
//    printf("num verts before model %llu\n", g_vertices.size());
//    printf("num idx before model %llu\n", g_indices.size());
//
//    const std::string full_path = base_path + path;
//
//    model_handle model_index;
//    if (indirect_model_loaded(full_path, model_index))
//        return model_index;
//
//    cgltf_options options = {};
//    cgltf_data* data = NULL;
//    cgltf_result res = cgltf_parse_file(&options, full_path.c_str(), &data);
//    if (res != cgltf_result_success)
//        assert(false);
//
//    std::unique_ptr<cgltf_data, void (*)(cgltf_data*)> dataPtr(data, &cgltf_free);
//
//    res = cgltf_load_buffers(&options, data, full_path.c_str());
//    if (res != cgltf_result_success)
//        assert(false);
//
//    res = cgltf_validate(data);
//    if (res != cgltf_result_success)
//        return false;
//
//    const std::string path_without_filename = full_path.substr(0, full_path.find_last_of("/") + 1);
//
//    Model model;
//    model.m_name = path;
//    for (cgltf_size i = 0; i < data->scene->nodes_count; i++) {
//        process_node_cgltf(data->scene->nodes[i], data, model, path_without_filename, mat4(1.0f));
//    }
//
//    model.calculate_aabb();
//
//    model_index = m_indirect_models.size();
//
//    m_indirect_models.push_back(model);
//    m_indirect_model_names.push_back(path);
//
//    printf("num verts after model %llu\n", g_vertices.size());
//    printf("num idx after model %llu\n", g_indices.size());
//
//    return model_index;
//}
//
//model_handle load_animated_model_cgltf(const std::string& path) {
//    printf("num rigged verts before model %llu\n", g_rigged_vertices.size());
//    printf("num idx before model %llu\n", g_indices.size());
//    printf("num bones before model %llu\n", g_rigged_bones.size());
//
//    const std::string full_path = base_path + path;
//
//    model_handle model_index;/*
//    if (animated_model_loaded(full_path, model_index)) {
//        printf("OYYOYOYOYOYOYOOY\n\n\n\n\nYOOO");
//        Animated_Model loaded = m_animated_models[model_index];
//        //num_skinned_bones += model.bone_count;
//
//        Animated_Model copy;
//        copy.m_meshes = loaded.m_meshes; // todo change when duplicating verts
//        copy.m_aabb = loaded.m_aabb;
//        copy.base_bone = loaded.base_bone;
//        copy.bone_count = loaded.bone_count;
//        copy.bone_offset = num_skinned_bones - loaded.base_bone; assert(copy.bone_offset >= 0);
//        num_skinned_bones += copy.bone_count;
//        copy.base_leaf = loaded.base_leaf;
//        copy.leaf_count = loaded.leaf_count;
//        copy.base_animation = loaded.base_animation;
//        copy.animation_count = loaded.animation_count;
//
//        model_index = m_animated_models.size();
//        m_animated_models.push_back(copy);
//        m_animated_model_names.push_back(full_path);
//
//        //printf("offset: %d, tot: %d\n", copy.bone_offset, num_skinned_bones);
//        printf("num rigged verts after model %llu\n", g_rigged_vertices.size());
//        printf("num rigged idx after model %llu\n", g_animated_indices.size());
//        printf("num bones after model %llu\n", g_rigged_bones.size());
//        // leaf bones
//        // maybe kf's
//        printf("here\n");
//        printf("num animations loaded %llu\n", g_animations.size());
//        printf("not here\n");
//
//        return model_index;
//    }*/
//
//    cgltf_options options = {};
//    cgltf_data* data = NULL;
//    cgltf_result res = cgltf_parse_file(&options, full_path.c_str(), &data);
//    if (res != cgltf_result_success)
//        assert(false);
//
//    std::unique_ptr<cgltf_data, void (*)(cgltf_data*)> dataPtr(data, &cgltf_free);
//
//    res = cgltf_load_buffers(&options, data, full_path.c_str());
//    if (res != cgltf_result_success)
//        assert(false);
//
//    res = cgltf_validate(data);
//    if (res != cgltf_result_success)
//        return false;
//
//    const std::string path_without_filename = full_path.substr(0, full_path.find_last_of("/") + 1);
//
//
//    Animated_Model model;
//    model.m_name = path;
//
//    uint32_t base_bone = g_rigged_bones.size();
//    for (cgltf_size i = 0; i < data->scene->nodes_count; i++) {
//        process_node_animated_cgltf(data->scene->nodes[i], data, model, path_without_filename, mat4(1.0f), base_bone);
//    }
//
//    model.base_bone = base_bone;
//    model.bone_count = g_rigged_bones.size() - base_bone;
//    model.bone_offset = num_skinned_bones - model.base_bone; assert(model.bone_offset >= 0);
//    num_skinned_bones += model.bone_count;
//
//    printf("calculating leaf bones\n");
//    model.base_leaf = g_leaf_bones.size();
//    add_leaf_bones(base_bone, g_rigged_bones.size());
//    model.leaf_count = g_leaf_bones.size() - model.base_leaf;
//
//    printf("LOADING ANIMATIONS\n");
//    model.base_animation = g_animations.size();
//    load_animations_from_scene_cgltf(data, base_bone);
//    model.animation_count = g_animations.size() - model.base_animation;
//
//    model.calculate_aabb();
//
//    model_index = m_animated_models.size();
//    m_animated_models.push_back(model);
//    m_animated_model_names.push_back(path);
//
//    printf("num rigged verts after model %llu\n", g_rigged_vertices.size());
//    printf("num idx after model %llu\n", g_indices.size());
//    printf("num bones after model %llu\n", g_rigged_bones.size());
//
//    return model_index;
//}
//
//void process_node_cgltf(cgltf_node* node, const cgltf_data* data, Model& model, const std::string& path, mat4 parent_transform) {
//    cgltf_float c_transform[16];
//    cgltf_node_transform_local(node, c_transform);
//
//    mat4 transform = parent_transform * cgltf_to_glm(c_transform);
//
//    if (node->mesh) {
//        const cgltf_mesh* mesh = node->mesh;
//        std::string mesh_name = mesh->name ? std::string(mesh->name) : "unnamed_mesh";
//
//        for (cgltf_size i = 0; i < mesh->primitives_count; i++) {
//            const cgltf_primitive* prim = &mesh->primitives[i];
//
//            if (prim->type != cgltf_primitive_type_triangles) { // maybe support more
//                continue;
//            }
//
//            Mesh mesh = process_mesh_cgltf(prim, data, i, path);
//
//            mesh.transform = transform;
//            // mesh.aabb.max = vec3(transform * vec4(mesh.aabb.max, 1.0f));
//            // mesh.aabb.min = vec3(transform * vec4(mesh.aabb.min, 1.0f));
//
//            model.add_mesh(mesh);
//
//            num_meshes++;
//        }
//    }
//    // todo add handling for light, maybe camera
//
//    for (cgltf_size i = 0; i < node->children_count; i++) {
//        process_node_cgltf(node->children[i], data, model, path, transform);
//    }
//}
//
//void process_node_animated_cgltf(cgltf_node* node, const cgltf_data* data, Animated_Model& model, const std::string& path, mat4 parent_transform, uint32_t base_bone) {
//    cgltf_float c_transform[16];
//    cgltf_node_transform_local(node, c_transform);
//    mat4 transform = parent_transform * cgltf_to_glm(c_transform);
//
//    if (node->mesh) {
//        const cgltf_mesh* mesh = node->mesh;
//
//        if (!node->skin) {
//            printf("Warning: mesh %s has no skin, skipping animated processing\n", mesh->name ? mesh->name : "unnamed");
//        }
//        else {
//            cgltf_skin* skin = node->skin;
//
//            std::unordered_map<const cgltf_node*, const cgltf_node*> node_to_parent;
//            //                          child             parent
//            for (cgltf_size i = 0; i < data->nodes_count; i++) {
//                cgltf_node* parent = &data->nodes[i];
//                for (cgltf_size j = 0; j < parent->children_count; j++) {
//                    node_to_parent[parent->children[j]] = parent;
//                }
//            }
//
//            std::unordered_map<const cgltf_node*, uint32_t> node_to_bone_index;
//            load_bones_from_skin_cgltf(skin, data, base_bone, node_to_parent, node_to_bone_index);
//
//            for (cgltf_size i = 0; i < mesh->primitives_count; i++) {
//                const cgltf_primitive* prim = &mesh->primitives[i];
//                if (prim->type != cgltf_primitive_type_triangles) continue; // todo maybe support more
//
//                //Animated_Mesh anim_mesh = process_animated_mesh_cgltf(prim, data, path, base_bone, skin, node_to_bone_index);
//
//                /*anim_mesh.transform = transform;
//                anim_mesh.aabb.max = vec3(transform * vec4(anim_mesh.aabb.max, 1.0f));
//                anim_mesh.aabb.min = vec3(transform * vec4(anim_mesh.aabb.min, 1.0f));*/
//
//                //model.add_mesh(anim_mesh);
//            }
//        }
//    }
//
//    for (cgltf_size i = 0; i < node->children_count; i++) {
//        process_node_animated_cgltf(node->children[i], data, model, path, transform, base_bone);
//    }
//}
//
//Mesh process_mesh_cgltf(const cgltf_primitive* prim, const cgltf_data* data, cgltf_size i, const std::string& path) {
//    Mesh mesh_ind = { 0 };
//    return mesh_ind;
//    //mesh_ind.name = mesh_name + "_primitive_" + std::to_string(primitive_index);
//    //printf("loading mesh %s\n", mesh_ind.name.c_str());
//
//    //mesh_ind.aabb.min = vec3(FLT_MAX);
//    //mesh_ind.aabb.max = vec3(-FLT_MAX);
//    //mesh_ind.base_vertex = g_vertices.size();
//
//    //uint32_t vertex_count = 0;
//    //cgltf_accessor* position_accessor = nullptr;
//
//    //for (cgltf_size i = 0; i < prim->attributes_count; i++) {
//    //    if (prim->attributes[i].type == cgltf_attribute_type_position) {
//    //        position_accessor = prim->attributes[i].data;
//    //        vertex_count = position_accessor->count;
//    //        break;
//    //    }
//    //}
//
//    //if (!position_accessor) {
//    //    printf("Warning: primitive has no position attribute\n");
//    //    assert(false);
//    //}
//
//    //g_vertices.reserve(g_vertices.size() + vertex_count);
//
//    //std::vector<Vertex> temp_vertices(vertex_count);
//    //for (cgltf_size i = 0; i < prim->attributes_count; i++) {
//    //    cgltf_attribute* attr = &prim->attributes[i];
//    //    cgltf_accessor* accessor = attr->data;
//
//    //    if (attr->type == cgltf_attribute_type_position) {
//    //        for (uint32_t v = 0; v < vertex_count; v++) {
//    //            float pos[3];
//    //            cgltf_accessor_read_float(accessor, v, pos, 3);
//    //            temp_vertices[v].position = vec3(pos[0], pos[1], pos[2]);
//
//    //            mesh_ind.aabb.min = min(temp_vertices[v].position, mesh_ind.aabb.min);
//    //            mesh_ind.aabb.max = max(temp_vertices[v].position, mesh_ind.aabb.max);
//    //        }
//    //    }
//    //    else if (attr->type == cgltf_attribute_type_normal) {
//    //        for (uint32_t v = 0; v < vertex_count; v++) {
//    //            float normal[3];
//    //            cgltf_accessor_read_float(accessor, v, normal, 3);
//    //            temp_vertices[v].normal = vec3(normal[0], normal[1], normal[2]);
//    //        }
//    //    }
//    //    else if (attr->type == cgltf_attribute_type_texcoord) {
//    //        for (uint32_t v = 0; v < vertex_count; v++) {
//    //            float texcoord[2];
//    //            cgltf_accessor_read_float(accessor, v, texcoord, 2);
//    //            temp_vertices[v].tex_coords = vec2(texcoord[0], texcoord[1]);
//    //        }
//    //    }
//    //    else if (attr->type == cgltf_attribute_type_tangent) {
//    //        for (uint32_t v = 0; v < vertex_count; v++) {
//    //            float tangent[4];
//    //            cgltf_accessor_read_float(accessor, v, tangent, 4);
//    //            temp_vertices[v].tangent = vec3(tangent[0], tangent[1], tangent[2]);
//
//    //            vec3 bitangent = cross(temp_vertices[v].normal, temp_vertices[v].tangent) * tangent[3];
//    //            temp_vertices[v].bitangent = bitangent;
//    //        }
//    //    }
//    //    // maybe add color
//    //}
//
//    //for (uint32_t v = 0; v < vertex_count; v++) {
//    //    if (length(temp_vertices[v].normal) == 0.0f) {
//    //        temp_vertices[v].normal = vec3(0.0f, 1.0f, 0.0f);
//    //    }
//    //    if (temp_vertices[v].tex_coords == vec2(0.0f) && !has_attribute(prim, cgltf_attribute_type_texcoord)) {
//    //        temp_vertices[v].tex_coords = vec2(0.0f, 0.0f);
//    //    }
//    //    if (length(temp_vertices[v].tangent) == 0.0f) {
//    //        temp_vertices[v].tangent = vec3(1.0f, 0.0f, 0.0f);
//    //    }
//    //    if (length(temp_vertices[v].bitangent) == 0.0f) {
//    //        temp_vertices[v].bitangent = vec3(0.0f, 0.0f, 1.0f);
//    //    }
//    //}
//
//    //for (const auto& vertex : temp_vertices) {
//    //    g_vertices.push_back(vertex);
//    //}
//
//    //mesh_ind.base_index = g_indices.size();
//
//    //if (prim->indices) {
//    //    uint32_t index_count = prim->indices->count;
//
//    //    std::vector<uint32_t> temp_indices(index_count);
//    //    cgltf_accessor_unpack_indices(prim->indices, temp_indices.data(), sizeof(uint32_t), index_count);
//
//    //    for (uint32_t idx : temp_indices)
//    //        g_indices.push_back(idx);
//
//    //    mesh_ind.index_count = index_count;
//    //}
//    //else {
//    //    for (uint32_t i = 0; i < vertex_count; i++)
//    //        g_indices.push_back(mesh_ind.base_vertex + i);
//
//    //    mesh_ind.index_count = vertex_count;
//    //}
//
//    //mesh_ind.material = load_material_cgltf(prim, data, path);
//
//    //return mesh_ind;
//}
//
//bool has_attribute(const cgltf_primitive* prim, cgltf_attribute_type type) {
//    for (cgltf_size i = 0; i < prim->attributes_count; i++) {
//        if (prim->attributes[i].type == type) {
//            return true;
//        }
//    }
//    return false;
//}
//
//// todo fix and dont use
//Mesh process_animated_mesh_cgltf(const cgltf_primitive* prim, const cgltf_data* data, const std::string& path, uint32_t base_bone, const cgltf_skin* skin, const std::unordered_map<const cgltf_node*, uint32_t>& node_to_bone_index) {
//    Mesh mesh_ind = { 0 };
//    return mesh_ind;
//    //// mesh_ind.name = ?
//    //printf("cgltf loading RIGGED mesh %s\n", mesh_ind.name.c_str());
//
//    //mesh_ind.aabb.min = vec3(FLT_MAX);
//    //mesh_ind.aabb.max = vec3(-FLT_MAX);
//
//    //// Get vertex count
//    //uint32_t vertex_count = 0;
//    //cgltf_accessor* position_accessor = nullptr;
//
//    //for (cgltf_size i = 0; i < prim->attributes_count; i++) {
//    //    if (prim->attributes[i].type == cgltf_attribute_type_position) {
//    //        position_accessor = prim->attributes[i].data;
//    //        vertex_count = position_accessor->count;
//    //        break;
//    //    }
//    //}
//
//    //if (!position_accessor) {
//    //    printf("no position attribute\n");
//    //    assert(false);
//    //}
//
//    //cgltf_accessor* joints_accessor = nullptr;
//    //cgltf_accessor* weights_accessor = nullptr;
//
//    //for (cgltf_size i = 0; i < prim->attributes_count; i++) {
//    //    if (prim->attributes[i].type == cgltf_attribute_type_joints) {
//    //        joints_accessor = prim->attributes[i].data;
//    //    }
//    //    else if (prim->attributes[i].type == cgltf_attribute_type_weights) {
//    //        weights_accessor = prim->attributes[i].data;
//    //    }
//    //}
//
//    //if (!joints_accessor || !weights_accessor) {
//    //    printf("missing joint/weight data\n");
//    //    assert(false);
//    //}
//
//    //mesh_ind.base_vertex = g_rigged_vertices.size();
//    //g_rigged_vertices.reserve(g_rigged_vertices.size() + vertex_count);
//
//    //std::vector<Rigged_Vertex> temp_vertices(vertex_count);
//    //for (cgltf_size i = 0; i < prim->attributes_count; i++) {
//    //    cgltf_attribute* attr = &prim->attributes[i];
//    //    cgltf_accessor* accessor = attr->data;
//
//    //    if (attr->type == cgltf_attribute_type_position) {
//    //        for (uint32_t v = 0; v < vertex_count; v++) {
//    //            float pos[3];
//    //            cgltf_accessor_read_float(accessor, v, pos, 3);
//    //            temp_vertices[v].position = vec3(pos[0], pos[1], pos[2]);
//
//    //            mesh_ind.aabb.min = min(temp_vertices[v].position, mesh_ind.aabb.min);
//    //            mesh_ind.aabb.max = max(temp_vertices[v].position, mesh_ind.aabb.max);
//    //        }
//    //    }
//    //    else if (attr->type == cgltf_attribute_type_normal) {
//    //        for (uint32_t v = 0; v < vertex_count; v++) {
//    //            float normal[3];
//    //            cgltf_accessor_read_float(accessor, v, normal, 3);
//    //            temp_vertices[v].normal = vec3(normal[0], normal[1], normal[2]);
//    //        }
//    //    }
//    //    else if (attr->type == cgltf_attribute_type_texcoord) {
//    //        for (uint32_t v = 0; v < vertex_count; v++) {
//    //            float texcoord[2];
//    //            cgltf_accessor_read_float(accessor, v, texcoord, 2);
//    //            temp_vertices[v].tex_coords = vec2(texcoord[0], texcoord[1]);
//    //        }
//    //    }
//    //    else if (attr->type == cgltf_attribute_type_tangent) {
//    //        for (uint32_t v = 0; v < vertex_count; v++) {
//    //            float tangent[4];
//    //            cgltf_accessor_read_float(accessor, v, tangent, 4);
//    //            temp_vertices[v].tangent = vec3(tangent[0], tangent[1], tangent[2]);
//
//    //            vec3 bitangent = cross(temp_vertices[v].normal, temp_vertices[v].tangent) * tangent[3];
//    //            temp_vertices[v].bitangent = bitangent;
//    //        }
//    //    }
//    //    else if (attr->type == cgltf_attribute_type_joints) {
//    //        for (uint32_t v = 0; v < vertex_count; v++) {
//    //            uint32_t joints[4];
//    //            if (cgltf_accessor_read_uint(accessor, v, joints, 4)) {
//    //                for (int j = 0; j < 4; j++) {
//    //                    if (joints[j] < skin->joints_count) {
//    //                        cgltf_node* joint_node = skin->joints[joints[j]];
//    //                        auto it = node_to_bone_index.find(joint_node);
//
//    //                        if (it != node_to_bone_index.end()) {
//    //                            temp_vertices[v].bone_ids[j] = it->second;
//    //                        }
//    //                        else {
//    //                            //printf("Joint node mapping failed for vertex %u, joint %d\n", v, j);
//    //                            temp_vertices[v].bone_ids[j] = 0;
//    //                        }
//    //                    }
//    //                    else {
//    //                        //printf("Joint index out of bounds: %u >= %llu\n", joints[j], skin->joints_count);
//    //                        temp_vertices[v].bone_ids[j] = 0;
//    //                    }
//    //                }
//    //                //printf("Vertex %u joints: [%u, %u, %u, %u] -> bones: [%u, %u, %u, %u]\n",
//    //                //    v, joints[0], joints[1], joints[2], joints[3],
//    //                //    temp_vertices[v].bone_ids[0], temp_vertices[v].bone_ids[1],
//    //                //    temp_vertices[v].bone_ids[2], temp_vertices[v].bone_ids[3]);
//    //            }
//    //            else {
//    //                printf("Failed to read joint data for vertex %u\n", v);
//    //                for (int j = 0; j < 4; j++) {
//    //                    temp_vertices[v].bone_ids[j] = 0;
//    //                }
//    //            }
//    //        }
//    //    }
//    //    else if (attr->type == cgltf_attribute_type_weights) {
//    //        printf("=== WEIGHT ACCESSOR DEBUG ===\n");
//    //        printf("Weight component_type: %d, type: %d, count: %llu\n",
//    //            accessor->component_type, accessor->type, accessor->count);
//
//    //        for (uint32_t v = 0; v < vertex_count; v++) {
//    //            float weights[4];
//    //            cgltf_accessor_read_float(accessor, v, weights, 4);
//    //            for (int j = 0; j < 4; j++) {
//    //                temp_vertices[v].bone_weights[j] = weights[j];
//    //            }
//    //        }
//    //    }
//    //}
//
//    //for (uint32_t v = 0; v < vertex_count; v++) {
//    //    if (length(temp_vertices[v].normal) == 0.0f) {
//    //        temp_vertices[v].normal = vec3(0.0f, 1.0f, 0.0f);
//    //    }
//    //    if (temp_vertices[v].tex_coords == vec2(0.0f) &&
//    //        !has_attribute(prim, cgltf_attribute_type_texcoord)) {
//    //        temp_vertices[v].tex_coords = vec2(0.0f, 0.0f);
//    //    }
//    //    if (length(temp_vertices[v].tangent) == 0.0f) {
//    //        temp_vertices[v].tangent = vec3(1.0f, 0.0f, 0.0f);
//    //    }
//    //    if (length(temp_vertices[v].bitangent) == 0.0f) {
//    //        temp_vertices[v].bitangent = vec3(0.0f, 0.0f, 1.0f);
//    //    }
//    //    // joint & weight?
//
//    //    for (uint32_t j = 0; j < 4; j++) {
//    //        //if (temp_vertices[v].bone_weights[j] == 0.0f)
//    //        //    temp_vertices[v].bone_ids[j] = 0;
//    //        //printf("  slot %d: bone %u, weight %.3f\n", v, temp_vertices[v].bone_ids[j], temp_vertices[v].bone_weights[j]);
//    //    }
//    //}
//
//    //for (const auto& vertex : temp_vertices) {
//    //    g_rigged_vertices.push_back(vertex);
//    //}
//
//    //// todo cahnge
//    ////mesh_ind.base_index = g_animated_indices.size();
//    //if (prim->indices) {
//    //    uint32_t index_count = prim->indices->count;
//    //    std::vector<uint32_t> temp_indices(index_count);
//    //    cgltf_accessor_unpack_indices(prim->indices, temp_indices.data(), sizeof(uint32_t), index_count);
//
//    //    for (uint32_t idx : temp_indices) {
//    //        //g_animated_indices.push_back(idx);
//    //    }
//    //    mesh_ind.index_count = index_count;
//    //}
//    //else {
//    //    for (uint32_t i = 0; i < vertex_count; i++) {
//    //        //g_animated_indices.push_back(i);
//    //    }
//    //    mesh_ind.index_count = vertex_count;
//    //}
//
//    //mesh_ind.material = load_material_cgltf(prim, data, path);
//
//    //return mesh_ind;
//}
//
//Material load_material_cgltf(const cgltf_primitive* prim, const cgltf_data* data, const std::string& path) {
//    Material mesh_mat{ 0 };
//
//    if (!prim->material) {
//        mesh_mat.base_color = vec4(1.0f, 0.0f, 1.0f, 1.0f);
//        mesh_mat.metallic_factor = 0.0f;
//        mesh_mat.roughness_factor = 1.0f;
//        mesh_mat.emissive_factor = vec4(1.0f);
//        return mesh_mat;
//    }
//
//    cgltf_material* material = prim->material;
//
//    // just met rough for now
//    if (material->has_pbr_metallic_roughness) {
//        cgltf_pbr_metallic_roughness* pbr = &material->pbr_metallic_roughness;
//
//        if (pbr->base_color_texture.texture && pbr->base_color_texture.texture->image &&
//            pbr->base_color_texture.texture->image->uri) {
//
//            std::string uri = pbr->base_color_texture.texture->image->uri;
//            uri.resize(cgltf_decode_uri(&uri[0]));
//
//            mesh_mat.albedo = Texture_Manager::load(path + uri);
//        }
//
//        if (pbr->metallic_roughness_texture.texture && pbr->metallic_roughness_texture.texture->image &&
//            pbr->metallic_roughness_texture.texture->image->uri) {
//
//            std::string uri = pbr->metallic_roughness_texture.texture->image->uri;
//            uri.resize(cgltf_decode_uri(&uri[0]));
//
//            mesh_mat.met_rough = Texture_Manager::load(path + uri);
//        }
//
//        mesh_mat.metallic_factor = pbr->metallic_factor;
//        mesh_mat.roughness_factor = pbr->roughness_factor;
//
//        mesh_mat.base_color = vec4(
//            pbr->base_color_factor[0],
//            pbr->base_color_factor[1],
//            pbr->base_color_factor[2],
//            pbr->base_color_factor[3]
//        );
//
//        // printf("MET: %f, ROG: %f\n", mesh_mat.metallic_factor, mesh_mat.roughness_factor);
//    }
//    else {
//        mesh_mat.base_color = vec4(1.0f);
//        mesh_mat.metallic_factor = 0.0f;
//        mesh_mat.roughness_factor = 1.0f;
//    }
//
//    if (material->normal_texture.texture && material->normal_texture.texture->image &&
//        material->normal_texture.texture->image->uri) {
//
//        std::string uri = material->normal_texture.texture->image->uri;
//        uri.resize(cgltf_decode_uri(&uri[0]));
//
//        mesh_mat.normal = Texture_Manager::load(path + uri);
//    }
//
//    if (material->emissive_texture.texture && material->emissive_texture.texture->image &&
//        material->emissive_texture.texture->image->uri) {
//
//        std::string uri = material->emissive_texture.texture->image->uri;
//        uri.resize(cgltf_decode_uri(&uri[0]));
//
//        mesh_mat.emissive = Texture_Manager::load(path + uri);
//    }
//
//    mesh_mat.emissive_factor = vec4(
//        material->emissive_factor[0],
//        material->emissive_factor[1],
//        material->emissive_factor[2],
//        1.0f // todo get emissive strength 
//    );
//
//    if (material->occlusion_texture.texture && material->occlusion_texture.texture->image &&
//        material->occlusion_texture.texture->image->uri) {
//
//        std::string uri = material->occlusion_texture.texture->image->uri;
//        uri.resize(cgltf_decode_uri(&uri[0]));
//
//        mesh_mat.amb_occ = Texture_Manager::load(path + uri);
//    }
//
//    // printf("path is :%s\n", path.c_str());
//
//    return mesh_mat;
//}
//
//void load_all_skins(const cgltf_data* data, uint32_t base_bone) {
//    //std::unordered_map<const cgltf_node*, const cgltf_node*> node_to_parent;
//    ////                          child             parent
//    //
//    //for (cgltf_size i = 0; i < data->nodes_count; i++) {
//    //    cgltf_node* parent = &data->nodes[i];
//    //    for (cgltf_size j = 0; j < parent->children_count; j++) {
//    //        node_to_parent[parent->children[j]] = parent;
//    //    }
//    //}
//
//    for (cgltf_size i = 0; i < data->skins_count; i++) {
//        //load_bones_from_skin_cgltf(&data->skins[i], data, base_bone, node_to_parent);
//    }
//}
//
//void load_bones_from_skin_cgltf(const cgltf_skin* skin, const cgltf_data* data, uint32_t base_bone, const std::unordered_map<const cgltf_node*, const cgltf_node*>& node_to_parent, std::unordered_map<const cgltf_node*, uint32_t>& node_to_bone_index) {
//    printf("[SKIN] Loading skin with %llu joints\n", skin->joints_count);
//
//    // g_rigged_bones.reserve(g_rigged_bones.size() + skin->joints_count);
//    for (cgltf_size i = 0; i < skin->joints_count; i++) {
//        cgltf_node* joint_node = skin->joints[i];
//        std::string bone_name = joint_node->name ? std::string(joint_node->name) : ("joint_" + std::to_string(i));
//
//        uint32_t bone_index = UINT32_MAX;
//        for (uint32_t j = base_bone; j < g_rigged_bones.size(); j++) {
//            if (g_rigged_bones[j].name == bone_name) {
//                bone_index = j;
//                break;
//            }
//        }
//
//        if (bone_index == UINT32_MAX) {
//            // printf("[BONE] adding bone %s\n", bone_name.c_str());
//            Bone new_bone;
//            new_bone.name = bone_name;
//
//            if (skin->inverse_bind_matrices) {
//                float matrix[16];
//                cgltf_accessor_read_float(skin->inverse_bind_matrices, i, matrix, 16);
//                new_bone.inverse_bind = cgltf_to_glm(matrix);
//                //new_bone.inverse_bind = mat4(1.0f);
//            }
//            else {
//                new_bone.inverse_bind = mat4(1.0f);
//            }
//
//            new_bone.parent_bone = UINT32_MAX;
//            bone_index = g_rigged_bones.size();
//            g_rigged_bones.push_back(new_bone);
//        }
//
//        node_to_bone_index[joint_node] = bone_index;
//    }
//
//
//    for (cgltf_size anim_idx = 0; anim_idx < data->animations_count; anim_idx++) {
//        const cgltf_animation* animation = &data->animations[anim_idx];
//        for (cgltf_size channel_idx = 0; channel_idx < animation->channels_count; channel_idx++) {
//            const cgltf_animation_channel* channel = &animation->channels[channel_idx];
//            cgltf_node* target_node = channel->target_node;
//
//            if (node_to_bone_index.find(target_node) != node_to_bone_index.end()) {
//                continue;
//            }
//
//            std::string bone_name = target_node->name ? std::string(target_node->name) : ("anim_target_" + std::to_string(channel_idx));
//
//            uint32_t bone_index = UINT32_MAX;
//            for (uint32_t j = base_bone; j < g_rigged_bones.size(); j++) {
//                if (g_rigged_bones[j].name == bone_name) {
//                    bone_index = j;
//                    break;
//                }
//            }
//
//            if (bone_index == UINT32_MAX) {
//                printf("[BONE] adding ANIMATION target bone %s\n", bone_name.c_str());
//                Bone new_bone;
//                new_bone.name = bone_name;
//                new_bone.inverse_bind = mat4(1.0f);
//                new_bone.parent_bone = UINT32_MAX;
//                bone_index = g_rigged_bones.size();
//                g_rigged_bones.push_back(new_bone);
//            }
//
//            node_to_bone_index[target_node] = bone_index;
//        }
//    }
//
//
//    // update parents
//    for (cgltf_size i = 0; i < skin->joints_count; i++) {
//        cgltf_node* joint_node = skin->joints[i];
//        uint32_t bone_index = node_to_bone_index[joint_node];
//
//
//        const cgltf_node* parent_node = joint_node->parent;
//        if (parent_node) {
//            auto itp = node_to_bone_index.find(parent_node);
//            if (itp != node_to_bone_index.end()) g_rigged_bones[bone_index].parent_bone = itp->second;
//            else g_rigged_bones[bone_index].parent_bone = UINT32_MAX;
//        }
//        else {
//            g_rigged_bones[bone_index].parent_bone = UINT32_MAX;
//        }
//
//        /*
//        const cgltf_node* parent_node = nullptr;
//        auto parent_it = node_to_parent.find(joint_node);
//        if (parent_it != node_to_parent.end()) {
//            parent_node = parent_it->second;
//        }
//
//        if (parent_node && node_to_bone_index.find(parent_node) != node_to_bone_index.end()) {
//            uint32_t parent_bone_index = node_to_bone_index[parent_node];
//            g_rigged_bones[bone_index].parent_bone = parent_bone_index;
//            printf("[BONE] %s -> parent: %s (index %u)\n",
//                g_rigged_bones[bone_index].name.c_str(),
//                g_rigged_bones[parent_bone_index].name.c_str(),
//                parent_bone_index);
//        }
//        else {
//            printf("[BONE] %s is a root bone (no parent or parent not in skin)\n", g_rigged_bones[bone_index].name.c_str());
//        }*/
//    }
//    //for (cgltf_size i = 0; i < skin->joints_count; i++) {
//    //    cgltf_node* joint_node = skin->joints[i];
//    //    uint32_t bone_index = node_to_bone_index[joint_node];
//
//    //    cgltf_node* parent_node = joint_node->parent;
//
//    //    if (parent_node) {
//    //        auto parent_it = node_to_bone_index.find(parent_node);
//    //        if (parent_it != node_to_bone_index.end()) {
//    //            uint32_t parent_bone_index = parent_it->second;
//    //            g_rigged_bones[bone_index].parent_bone = parent_bone_index;
//    //        }
//    //    }
//    //}
//
//    printf("\n[BONES] Final bone hierarchy:\n");
//    for (uint32_t i = base_bone; i < g_rigged_bones.size(); i++) {
//        const Bone& bone = g_rigged_bones[i];
//        if (bone.parent_bone == UINT32_MAX) {
//            printf("[BONE %u] %s (ROOT)\n", i, bone.name.c_str());
//        }
//        else {
//            printf("[BONE %u] %s -> parent: %s (index %u)\n",
//                i, bone.name.c_str(),
//                g_rigged_bones[bone.parent_bone].name.c_str(),
//                bone.parent_bone);
//        }
//    }
//
//    printf("[BONES] Total bones loaded: %zu\n", g_rigged_bones.size() - base_bone);
//}
//
//void load_animations_from_scene_cgltf(const cgltf_data* data, uint32_t base_bone) {
//    for (cgltf_size anim_idx = 0; anim_idx < data->animations_count; ++anim_idx) {
//        cgltf_animation* gltf_anim = &data->animations[anim_idx];
//
//        Animation animation;
//        animation.base_bone_animation = g_bone_animations.size();
//
//        std::string anim_name = gltf_anim->name ? std::string(gltf_anim->name) :
//            ("animation_" + std::to_string(anim_idx));
//
//        printf("Loading animation: %s\n", anim_name.c_str());
//
//        float max_time = 0.0f;
//        for (cgltf_size channel_idx = 0; channel_idx < gltf_anim->channels_count; channel_idx++) {
//            cgltf_animation_channel* channel = &gltf_anim->channels[channel_idx];
//            cgltf_animation_sampler* sampler = channel->sampler;
//
//            if (sampler->input->count > 0) {
//                float last_time;
//                cgltf_accessor_read_float(sampler->input, sampler->input->count - 1, &last_time, 1);
//                max_time = std::max(max_time, last_time);
//            }
//        }
//        animation.duration = max_time;
//
//        /*
//        for (cgltf_size channel_idx = 0; channel_idx < gltf_anim->channels_count; channel_idx++) {
//            cgltf_animation_channel* channel = &gltf_anim->channels[channel_idx];
//
//            if (!channel->target_node) continue;
//
//            // Find bone index for this node
//            std::string node_name = channel->target_node->name ? std::string(channel->target_node->name) : ("node_" + std::to_string(cgltf_node_index(data, channel->target_node)));
//
//            uint32_t bone_index = find_bone_index(node_name, base_bone);
//
//            Bone_Animation bone_anim;
//            bone_anim.bone_index = bone_index;
//            bone_anim.base_position_keyframe = position_keyframes.size();
//            bone_anim.base_rotation_keyframe = rotation_keyframes.size();
//            bone_anim.base_scale_keyframe = scale_keyframes.size();
//
//            load_keyframes_from_channel_cgltf(channel);
//
//            bone_anim.position_keyframe_count = position_keyframes.size() - bone_anim.base_position_keyframe;
//            bone_anim.rotation_keyframe_count = rotation_keyframes.size() - bone_anim.base_rotation_keyframe;
//            bone_anim.scale_keyframe_count = scale_keyframes.size() - bone_anim.base_scale_keyframe;
//
//            g_bone_animations.push_back(bone_anim);
//        }
//        */
//
//        std::unordered_map<uint32_t, Bone_Animation> bone_anim_map;
//        for (cgltf_size channel_idx = 0; channel_idx < gltf_anim->channels_count; channel_idx++) {
//            cgltf_animation_channel* channel = &gltf_anim->channels[channel_idx];
//            if (!channel->target_node) continue;
//
//            std::string node_name = channel->target_node->name ?
//                std::string(channel->target_node->name) :
//                ("node" + std::to_string(cgltf_node_index(data, channel->target_node)));
//
//            uint32_t bone_index = find_bone_index(node_name, base_bone);
//            if (bone_index == 0xFFFFFFFF) continue;
//
//            auto& bone_anim = bone_anim_map[bone_index];
//            if (bone_anim.bone_index == 0 && bone_anim.base_position_keyframe == 0) {
//                bone_anim.bone_index = bone_index;
//                bone_anim.base_position_keyframe = position_keyframes.size();
//                bone_anim.base_rotation_keyframe = rotation_keyframes.size();
//                bone_anim.base_scale_keyframe = scale_keyframes.size();
//            }
//
//            load_keyframes_from_channel_cgltf(channel);
//
//            if (channel->target_path == cgltf_animation_path_type_translation)
//                bone_anim.position_keyframe_count = position_keyframes.size() - bone_anim.base_position_keyframe;
//            else if (channel->target_path == cgltf_animation_path_type_rotation)
//                bone_anim.rotation_keyframe_count = rotation_keyframes.size() - bone_anim.base_rotation_keyframe;
//            else if (channel->target_path == cgltf_animation_path_type_scale)
//                bone_anim.scale_keyframe_count = scale_keyframes.size() - bone_anim.base_scale_keyframe;
//        }
//
//        for (auto& [bone_idx, anim] : bone_anim_map)
//            g_bone_animations.push_back(anim);
//
//        animation.bone_animation_count = g_bone_animations.size() - animation.base_bone_animation;
//        g_animations.push_back(animation);
//        g_animation_names.push_back(anim_name);
//
//        printf("Animation duration: %.2f seconds\n", animation.duration);
//    }
//}
//
//void load_keyframes_from_channel_cgltf(cgltf_animation_channel* channel) {
//    cgltf_animation_sampler* sampler = channel->sampler;
//
//    if (channel->target_path == cgltf_animation_path_type_translation) {
//        for (cgltf_size i = 0; i < sampler->input->count; i++) {
//            Position_Keyframe keyframe;
//
//            float time;
//            cgltf_accessor_read_float(sampler->input, i, &time, 1);
//            keyframe.time = time;
//
//            float pos[3];
//            cgltf_accessor_read_float(sampler->output, i, pos, 3);
//            keyframe.position = vec3(pos[0], pos[1], pos[2]);
//
//            position_keyframes.push_back(keyframe);
//        }
//    }
//    else if (channel->target_path == cgltf_animation_path_type_rotation) {
//        for (cgltf_size i = 0; i < sampler->input->count; i++) {
//            Rotation_Keyframe keyframe;
//
//            float time;
//            cgltf_accessor_read_float(sampler->input, i, &time, 1);
//            keyframe.time = time;
//
//            float rot[4];
//            cgltf_accessor_read_float(sampler->output, i, rot, 4);
//            keyframe.rotation = quat(rot[3], rot[0], rot[1], rot[2]);
//            rotation_keyframes.push_back(keyframe);
//        }
//    }
//    else if (channel->target_path == cgltf_animation_path_type_scale) {
//        for (cgltf_size i = 0; i < sampler->input->count; i++) {
//            Scale_Keyframe keyframe;
//
//            float time;
//            cgltf_accessor_read_float(sampler->input, i, &time, 1);
//            keyframe.time = time;
//
//            float scale[3];
//            cgltf_accessor_read_float(sampler->output, i, scale, 3);
//            keyframe.scale = vec3(scale[0], scale[1], scale[2]);
//
//            scale_keyframes.push_back(keyframe);
//        }
//    }
//}