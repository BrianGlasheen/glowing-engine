#include "renderer_debug.h"

// #include <glad/glad.h>
#include "core/opengl.h"

#include "glm/glm.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

void Renderer_Debug::init() {
    // line VAO/VBO 
    glGenVertexArrays(1, &line_vao);
    glGenBuffers(1, &line_vbo);
    glBindVertexArray(line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    // We won't set any data here yet. We'll do it in `render()`.
    // Just allocate some space or use a 0-size buffer with dynamic usage.
    // thanks chat gpt
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // Layout: position (3 floats) + color (3 floats) => total 6 floats
    // positions go to location=0, colors go to location=1
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    //build_sphere_geometry();
    //build_cube_geometry();
    sphere_vertices.clear();
    
    const int segments = 32;
    const float step = 2.0f * 3.1415 / segments;
    
    // xy
    for (int i = 0; i <= segments; ++i) {
        float angle = i * step;
        sphere_vertices.push_back(glm::vec3(cos(angle), sin(angle), 0.0f));
    }
    
    // xz
    for (int i = 0; i <= segments; ++i) {
        float angle = i * step;
        sphere_vertices.push_back(glm::vec3(cos(angle), 0.0f, sin(angle)));
    }
    
    // yz
    for (int i = 0; i <= segments; ++i) {
        float angle = i * step;
        sphere_vertices.push_back(glm::vec3(0.0f, cos(angle), sin(angle)));
    }
    
    sphere_vertex_count = sphere_vertices.size();
        
    glGenVertexArrays(1, &sphere_vao);
    glGenBuffers(1, &sphere_vbo);
    
    glBindVertexArray(sphere_vao);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
    
    glBufferData(GL_ARRAY_BUFFER, 
                    sphere_vertices.size() * sizeof(glm::vec3), 
                    sphere_vertices.data(), 
                    GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void Renderer_Debug::shutdown() {
    // todo
}

void Renderer_Debug::add_line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
    line_vertices.push_back(start.x);
    line_vertices.push_back(start.y);
    line_vertices.push_back(start.z);
    line_vertices.push_back(color.r);
    line_vertices.push_back(color.g);
    line_vertices.push_back(color.b);

    line_vertices.push_back(end.x);
    line_vertices.push_back(end.y);
    line_vertices.push_back(end.z);
    line_vertices.push_back(color.r);
    line_vertices.push_back(color.g);
    line_vertices.push_back(color.b);
}

//void Renderer_Debug::add_sphere(const glm::vec3& center, float radius, const glm::vec3& color) {
//    Debug_sphere s;
//    s.center = center;
//    s.radius = radius;
//    s.color  = color;
//    spheres.push_back(s);
//}
//
//void Renderer_Debug::add_cube(const glm::vec3& center, float size, const glm::vec3& color) {
//    Debug_cube c;
//    c.center = center;
//    c.size = size;
//    c.color = color;
//    cubes.push_back(c);
//}

//void Renderer_Debug::add_axes(const glm::vec3& position, const glm::quat& orientation, float length) {
//    glm::vec3 xEnd = position + (orientation * glm::vec3(length, 0.0f, 0.0f)); // x
//    add_line(position, xEnd, glm::vec3(1.0f, 0.0f, 0.0f));
//
//    glm::vec3 yEnd = position + (orientation * glm::vec3(0.0f, length, 0.0f)); // y
//    add_line(position, yEnd, glm::vec3(0.0f, 1.0f, 0.0f));
//
//    glm::vec3 zEnd = position + (orientation * glm::vec3(0.0f, 0.0f, length)); // z (zed)
//    add_line(position, zEnd, glm::vec3(0.0f, 0.0f, 1.0f));
//}

void Renderer_Debug::add_bbox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color) {
    glm::vec3 corners[8] = {
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(min.x, max.y, max.z)
    };

    // z = min.z
    add_line(corners[0], corners[1], color);
    add_line(corners[1], corners[2], color);
    add_line(corners[2], corners[3], color);
    add_line(corners[3], corners[0], color);

    // z = max.z
    add_line(corners[4], corners[5], color);
    add_line(corners[5], corners[6], color);
    add_line(corners[6], corners[7], color);
    add_line(corners[7], corners[4], color);

    // vert lines
    add_line(corners[0], corners[4], color); 
    add_line(corners[1], corners[5], color);
    add_line(corners[2], corners[6], color);
    add_line(corners[3], corners[7], color);
}

void Renderer_Debug::draw_frustum(const glm::vec3& cameraPos, const glm::vec3& cameraDir, const glm::vec3& cameraUp, const float& fov, const float& aspect, const float& near, const float& far, const glm::vec3& color) {
    glm::vec3 right = glm::normalize(glm::cross(cameraDir, cameraUp));
    glm::vec3 up = glm::normalize(glm::cross(right, cameraDir));

    // Calculate plane dimensions
    float nearHeight = 2.0f * tan(fov / 2.0f) * near;
    float nearWidth = nearHeight * aspect;
    float farHeight = 2.0f * tan(fov / 2.0f) * far;
    float farWidth = farHeight * aspect;

    glm::vec3 nearCenter = cameraPos + cameraDir * near;
    glm::vec3 farCenter = cameraPos + cameraDir * far;

    glm::vec3 corners[8];

    // near
    corners[0] = nearCenter - right * (nearWidth * 0.5f) - up * (nearHeight * 0.5f); // bottom-left
    corners[1] = nearCenter + right * (nearWidth * 0.5f) - up * (nearHeight * 0.5f); // bottom-right
    corners[2] = nearCenter + right * (nearWidth * 0.5f) + up * (nearHeight * 0.5f); // top-right
    corners[3] = nearCenter - right * (nearWidth * 0.5f) + up * (nearHeight * 0.5f); // top-left

    // far
    corners[4] = farCenter - right * (farWidth * 0.5f) - up * (farHeight * 0.5f); // bottom-left
    corners[5] = farCenter + right * (farWidth * 0.5f) - up * (farHeight * 0.5f); // bottom-right
    corners[6] = farCenter + right * (farWidth * 0.5f) + up * (farHeight * 0.5f); // top-right
    corners[7] = farCenter - right * (farWidth * 0.5f) + up * (farHeight * 0.5f); // top-left

    // near plane
    add_line(corners[0], corners[1], color); // bottom
    add_line(corners[1], corners[2], color); // right
    add_line(corners[2], corners[3], color); // top
    add_line(corners[3], corners[0], color); // left

    // far plane
    add_line(corners[4], corners[5], color); // bottom
    add_line(corners[5], corners[6], color); // right
    add_line(corners[6], corners[7], color); // top
    add_line(corners[7], corners[4], color); // left

    // connecting lines
    add_line(corners[0], corners[4], color); // bottom-left
    add_line(corners[1], corners[5], color); // bottom-right
    add_line(corners[2], corners[6], color); // top-right
    add_line(corners[3], corners[7], color); // top-left
}

void Renderer_Debug::render(Shader* debug_shader, const glm::mat4& projection, const glm::mat4& view, uint32_t num_cubes) {

    debug_shader->use();

    if (!line_vertices.empty()) {
        glBindVertexArray(line_vao);
        glBindBuffer(GL_ARRAY_BUFFER, line_vbo);

        glBufferData(GL_ARRAY_BUFFER, line_vertices.size() * sizeof(float), line_vertices.data(), GL_DYNAMIC_DRAW);

        debug_shader->set_mat4("mvp", projection * view);

        glDisable(GL_DEPTH_TEST);

        glDrawArrays(GL_LINES, 0, (GLsizei)(line_vertices.size() / 6));
        glBindVertexArray(0);
    }

    line_vertices.clear();
}

void Renderer_Debug::draw_bounding_sphere(Shader* debug_shader, const glm::vec3& center, float radius, const glm::vec3& color, const glm::mat4& vp) {
    if (sphere_vertex_count == 0) return;
    
    glm::mat4 model = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
    
    // Set uniforms (adjust these based on your shader)
    debug_shader->set_mat4("mvp", vp * model);
    debug_shader->set_vec3("color", color);
    
    // Draw the three circles
    glBindVertexArray(sphere_vao);
    
    const int segments = 32;
    const int vertices_per_circle = segments + 1;
    
    // xy
    glDrawArrays(GL_LINE_STRIP, 0, vertices_per_circle);
    // xz
    glDrawArrays(GL_LINE_STRIP, vertices_per_circle, vertices_per_circle);
    // yz
    glDrawArrays(GL_LINE_STRIP, vertices_per_circle * 2, vertices_per_circle);
    
    glBindVertexArray(0);
}

void Renderer_Debug::draw_scene_bounding_spheres(Shader* debug_shader, const Scene& scene, const glm::mat4& view_proj) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(1.0f);

    for (size_t i = 0; i < scene.gpu_meshes.size(); ++i) {
        const GPU_Mesh& mesh = scene.gpu_meshes[i];
        
        glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);
        
        // if (i < scene.animated_mesh_to_all_mesh_mapping.size()) {
        //     color = glm::vec3(1.0f, 0.0f, 1.0f);
        // }
        
        // if (mesh.entity_index < scene.gpu_entities.size() && scene.gpu_entities[mesh.entity_index].is_dirty) {
        //     color = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow for dirty
        // }
        
        glm::mat4 entity_transform = scene.gpu_entities[mesh.entity_index].transform;
        glm::vec3 center = glm::vec3(entity_transform * glm::vec4(glm::vec3(mesh.bounding_sphere), 1.0));
        // bounding sphere radius has entity scale baked in
        draw_bounding_sphere(debug_shader, center, mesh.bounding_sphere.w, color, view_proj);
    }
    
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glUseProgram(0);
}

//void Renderer_Debug::build_sphere_geometry() {
//    const int latSegments = 8;
//    const int lonSegments = 8;
//    const float PI = 3.14159f;
//
//    std::vector<glm::vec3> positions;
//    std::vector<uint32_t> indices;
//
//    for (int y = 0; y <= latSegments; y++) {
//        for (int x = 0; x <= lonSegments; x++) {
//            float u = (float)x / (float)lonSegments;
//            float v = (float)y / (float)latSegments;
//
//            float theta = u * 2.0f * PI;
//            float phi = v * PI;
//
//            float sinPhi = sin(phi);
//            float cosPhi = cos(phi);
//
//            float sinTheta = sin(theta);
//            float cosTheta = cos(theta);
//
//            float px = cosTheta * sinPhi;
//            float py = cosPhi;
//            float pz = sinTheta * sinPhi;
//
//            positions.push_back(glm::vec3(px, py, pz));
//        }
//    }
//
//    for (int y = 0; y < latSegments; y++) {
//        for (int x = 0; x < lonSegments; x++) {
//            int i0 = y * (lonSegments + 1) + x;
//            int i1 = y * (lonSegments + 1) + x + 1;
//            int i2 = (y + 1) * (lonSegments + 1) + x;
//            int i3 = (y + 1) * (lonSegments + 1) + x + 1;
//
//            indices.push_back(i0);
//            indices.push_back(i2);
//            indices.push_back(i1);
//
//            indices.push_back(i1);
//            indices.push_back(i2);
//            indices.push_back(i3);
//        }
//    }
//    sphere_index_count = (int)indices.size();
//
//    std::vector<float> vertexData;
//    vertexData.reserve(positions.size() * 3);
//
//    for (size_t i = 0; i < positions.size(); i++) {
//        vertexData.push_back(positions[i].x);
//        vertexData.push_back(positions[i].y);
//        vertexData.push_back(positions[i].z);
//    }
//
//    glGenVertexArrays(1, &sphere_vao);
//    glGenBuffers(1, &sphere_vbo);
//    glGenBuffers(1, &sphere_ebo);
//
//    glBindVertexArray(sphere_vao);
//
//    glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
//    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
//
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_ebo);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
//
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
//
//    glEnableVertexAttribArray(1); // dummy color
//    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
//
//    glBindVertexArray(0);
//}
//
//void Renderer_Debug::build_cube_geometry() {
//    float vertices[] = {
//        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
//         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
//         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
//         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
//        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
//        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
//
//        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
//         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
//         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
//         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
//        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
//        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
//
//        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
//        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
//        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
//        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
//        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
//        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
//
//         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
//         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
//         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
//         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
//         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
//         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
//
//         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
//          0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
//          0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
//          0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
//         -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
//         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
//
//         -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
//          0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
//          0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
//          0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
//         -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
//         -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f
//    };
//    cube_vertex_count = 36;
//
//    glGenVertexArrays(1, &cube_vao);
//    glGenBuffers(1, &cube_vbo);
//
//    glBindVertexArray(cube_vao);
//
//    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);
//
//    glBindVertexArray(0);
//}