#include "renderer_debug.h"

// #include <glad/glad.h>
#include "core/opengl.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    printf("s[pjere\n");

    build_sphere_geometry();
}

void Renderer_Debug::build_sphere_geometry() {
    // A quick way to generate a sphere is to do a “UV sphere” or “icosphere.” 
    // For simplicity, let’s do a small UV sphere. 
    // You can generate as many segments as you want. Here is just a minimal example.

    const int latSegments = 8;
    const int lonSegments = 8;
    const float PI = 3.14159f;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

    // Generate vertices
    for (int y = 0; y <= latSegments; y++) {
        for (int x = 0; x <= lonSegments; x++) {
            float u = (float)x / (float)lonSegments;
            float v = (float)y / (float)latSegments;

            float theta = u * 2.0f * PI; // around the Y-axis
            float phi   = v * PI;       // from top to bottom

            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            float px = cosTheta * sinPhi;
            float py = cosPhi;
            float pz = sinTheta * sinPhi;

            positions.push_back(glm::vec3(px, py, pz));
            normals.push_back(glm::vec3(px, py, pz));
        }
    }

    // generate indices (two triangles per quad)
    for (int y = 0; y < latSegments; y++) {
        for (int x = 0; x < lonSegments; x++) {
            int i0 = y       * (lonSegments + 1) + x;
            int i1 = y       * (lonSegments + 1) + x + 1;
            int i2 = (y + 1) * (lonSegments + 1) + x;
            int i3 = (y + 1) * (lonSegments + 1) + x + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
    sphere_index_count = (int)indices.size();

    // positions and normals are both vectors<glm::vec3>
    // assume you've already populated these with the sphere geometry data
    std::vector<float> vertexData;
    vertexData.reserve(positions.size() * 6); // 3 for position + 3 for normal

    for (size_t i = 0; i < positions.size(); i++) {
        // positions
        vertexData.push_back(positions[i].x);
        vertexData.push_back(positions[i].y);
        vertexData.push_back(positions[i].z);
        // normals
        vertexData.push_back(normals[i].x);
        vertexData.push_back(normals[i].y);
        vertexData.push_back(normals[i].z);
    }

    // Create VBO/VAO/EBO for the sphere
    glGenVertexArrays(1, &sphere_vao);
    glGenBuffers(1, &sphere_vbo);
    glGenBuffers(1, &sphere_ebo);

    // 1. Bind VAO
    glBindVertexArray(sphere_vao);

    // 2. VBO: upload vertex data (positions + normals)
    glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                vertexData.size() * sizeof(float),
                vertexData.data(),
                GL_STATIC_DRAW);

    // 3. EBO: upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // 4. Set up vertex attributes
    //    Stride = 6 floats (3 for pos, 3 for normal) => 6 * sizeof(float)
    int stride = 6 * sizeof(float);

    // Position attribute => location=0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, // 3 floats for position
                        GL_FLOAT, GL_FALSE, stride, (void*)0);

    // Normal attribute => location=1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, // 3 floats for normal
                        GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer_Debug::add_line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
    Debug_Line l;
    l.start = start;
    l.end   = end;
    l.color = color;
    lines.push_back(l);
}

void Renderer_Debug::add_sphere(const glm::vec3& center, float radius, const glm::vec3& color) {
    Debug_sphere s;
    s.center = center;
    s.radius = radius;
    s.color  = color;
    spheres.push_back(s);
}

void Renderer_Debug::add_axes(const glm::vec3& position, const glm::quat& orientation, float length) {
    glm::vec3 xEnd = position + (orientation * glm::vec3(length, 0.0f, 0.0f)); // x
    add_line(position, xEnd, glm::vec3(1.0f, 0.0f, 0.0f));

    glm::vec3 yEnd = position + (orientation * glm::vec3(0.0f, length, 0.0f)); // y
    add_line(position, yEnd, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 zEnd = position + (orientation * glm::vec3(0.0f, 0.0f, length)); // z (zed)
    add_line(position, zEnd, glm::vec3(0.0f, 0.0f, 1.0f));
}

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

void Renderer_Debug::add_obb(const Util::OBB obb, const glm::vec3& color) {
    add_line(obb.corners[0], obb.corners[1], color); // min to +x
    add_line(obb.corners[1], obb.corners[3], color); // +x to +x+y
    add_line(obb.corners[3], obb.corners[2], color); // +x+y to +y
    add_line(obb.corners[2], obb.corners[0], color); // +y to min

    // Top face (z = max)
    add_line(obb.corners[4], obb.corners[5], color); // +z to +x+z
    add_line(obb.corners[5], obb.corners[7], color); // +x+z to max
    add_line(obb.corners[7], obb.corners[6], color); // max to +y+z
    add_line(obb.corners[6], obb.corners[4], color); // +y+z to +z

    // Vertical edges connecting bottom and top faces
    add_line(obb.corners[0], obb.corners[4], color); // min to +z
    add_line(obb.corners[1], obb.corners[5], color); // +x to +x+z
    add_line(obb.corners[2], obb.corners[6], color); // +y to +y+z
    add_line(obb.corners[3], obb.corners[7], color); // +x+y to max
}

void Renderer_Debug::draw_frustum(const glm::vec3& cameraPos, const glm::vec3& cameraDir, const glm::vec3& cameraUp, const float& fov, const float& aspect, const float& near, const float& far) {
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
    add_line(corners[0], corners[1], glm::vec3(1.0f, 0.0f, 0.0f)); // bottom
    add_line(corners[1], corners[2], glm::vec3(1.0f, 0.0f, 0.0f)); // right
    add_line(corners[2], corners[3], glm::vec3(1.0f, 0.0f, 0.0f)); // top
    add_line(corners[3], corners[0], glm::vec3(1.0f, 0.0f, 0.0f)); // left

    // far plane
    add_line(corners[4], corners[5], glm::vec3(1.0f, 0.0f, 0.0f)); // bottom
    add_line(corners[5], corners[6], glm::vec3(1.0f, 0.0f, 0.0f)); // right
    add_line(corners[6], corners[7], glm::vec3(1.0f, 0.0f, 0.0f)); // top
    add_line(corners[7], corners[4], glm::vec3(1.0f, 0.0f, 0.0f)); // left

    // connecting lines
    add_line(corners[0], corners[4], glm::vec3(1.0f, 0.0f, 0.0f)); // bottom-left
    add_line(corners[1], corners[5], glm::vec3(1.0f, 0.0f, 0.0f)); // bottom-right
    add_line(corners[2], corners[6], glm::vec3(1.0f, 0.0f, 0.0f)); // top-right
    add_line(corners[3], corners[7], glm::vec3(1.0f, 0.0f, 0.0f)); // top-left
}

void Renderer_Debug::render(Shader* debug_shader, const glm::mat4& projection, const glm::mat4& view) {
    if (!lines.empty()) {
        // Build a CPU buffer of vertices: for each line, we have two points, each with (pos + color)
        // that's 6 floats (pos) + 6 floats (color) for the entire line? Actually it's 6 floats total: 
        //   pos(3) + color(3) for each vertex. 
        // But each line has 2 vertices, so total 2 * 6 = 12 floats per line.

        std::vector<float> lineVertices;
        lineVertices.reserve(lines.size() * 2 * 6);

        for (auto& l : lines) {
            // Vertex 1
            lineVertices.push_back(l.start.x);
            lineVertices.push_back(l.start.y);
            lineVertices.push_back(l.start.z);
            lineVertices.push_back(l.color.r);
            lineVertices.push_back(l.color.g);
            lineVertices.push_back(l.color.b);

            // Vertex 2
            lineVertices.push_back(l.end.x);
            lineVertices.push_back(l.end.y);
            lineVertices.push_back(l.end.z);
            lineVertices.push_back(l.color.r);
            lineVertices.push_back(l.color.g);
            lineVertices.push_back(l.color.b);
        }

        // Upload to GPU
        glBindVertexArray(line_vao);
        glBindBuffer(GL_ARRAY_BUFFER, line_vbo);

        glBufferData(GL_ARRAY_BUFFER, 
                     lineVertices.size() * sizeof(float), 
                     lineVertices.data(), 
                     GL_DYNAMIC_DRAW);

        debug_shader->use();
        debug_shader->set_mat4("projection", projection);
        debug_shader->set_mat4("view", view);
        debug_shader->set_mat4("model", glm::mat4(1.0f));
        debug_shader->set_vec3("debugColor", glm::vec3(0.0f));

        //glDisable(GL_DEPTH_TEST);

        glDrawArrays(GL_LINES, 0, (GLsizei)(lines.size() * 2));
        glBindVertexArray(0);

        //glEnable(GL_DEPTH_TEST);
    }

    // spheres
    if (!spheres.empty()) {
        glBindVertexArray(sphere_vao);
        debug_shader->use();
        debug_shader->set_mat4("projection", projection);
        debug_shader->set_mat4("view", view);

        for (auto& s : spheres) {
            // Build model matrix for each sphere
            glm::mat4 model(1.0f);
            model = glm::translate(model, s.center);
            model = glm::scale(model, glm::vec3(s.radius)); 
            debug_shader->set_mat4("model", model);
            debug_shader->set_vec3("debugColor", s.color);

            // glDisable(GL_DEPTH_TEST); // if needed
            glDrawElements(GL_TRIANGLES, sphere_index_count, GL_UNSIGNED_INT, 0);
            // glEnable(GL_DEPTH_TEST);
        }

        glBindVertexArray(0);
    }

    lines.clear();
    spheres.clear();
}
