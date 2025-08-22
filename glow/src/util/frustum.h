#pragma once

#include <cmath>

#include "glm/glm.hpp"

namespace Util {

    struct Plane {
        glm::vec3 normal;
        float distance;

        Plane() = default;
        Plane(const glm::vec3& n, float d) : normal(n), distance(d) {}

        // Calculate signed distance from point to plane
        float distanceToPoint(const glm::vec3& point) const {
            return glm::dot(normal, point) + distance;
        }
    };

    struct Frustum {
        Plane planes[6]; // left, right, bottom, top, near, far

        Frustum(const glm::vec3& cameraPos, const glm::vec3& cameraDir, const glm::vec3& right, const glm::vec3& up, float fov, float aspect, float near, float far) {
            float halfVSide = far * tanf(fov * 0.5f);
            float halfHSide = halfVSide * aspect;
            glm::vec3 frontMultFar = far * cameraDir;

            // Near and far planes
            planes[4] = Plane(cameraDir, -glm::dot(cameraDir, cameraPos + near * cameraDir)); // near
            planes[5] = Plane(-cameraDir, glm::dot(cameraDir, cameraPos + frontMultFar)); // far

            // Left plane
            glm::vec3 leftNormal = glm::normalize(glm::cross(frontMultFar - right * halfHSide, up));
            planes[0] = Plane(leftNormal, -glm::dot(leftNormal, cameraPos));

            // Right plane
            glm::vec3 rightNormal = glm::normalize(glm::cross(up, frontMultFar + right * halfHSide));
            planes[1] = Plane(rightNormal, -glm::dot(rightNormal, cameraPos));

            // Bottom plane
            glm::vec3 bottomNormal = glm::normalize(glm::cross(right, frontMultFar - up * halfVSide));
            planes[2] = Plane(bottomNormal, -glm::dot(bottomNormal, cameraPos));

            // Top plane
            glm::vec3 topNormal = glm::normalize(glm::cross(frontMultFar + up * halfVSide, right));
            planes[3] = Plane(topNormal, -glm::dot(topNormal, cameraPos));
        }

        bool intersectsAABB(const Util::AABB& aabb, bool infinite_far = false) const {
            return intersectsAABB(aabb.min, aabb.max, infinite_far);
        }

        bool intersectsAABB(const glm::vec3& min, const glm::vec3& max, bool infinite_far = false) const {
            for (int i = 0; i < 6; i++) {
                if (infinite_far && i == 5) continue;

                // Get the positive vertex (farthest in plane normal direction)
                glm::vec3 positiveVertex;
                positiveVertex.x = (planes[i].normal.x >= 0) ? max.x : min.x;
                positiveVertex.y = (planes[i].normal.y >= 0) ? max.y : min.y;
                positiveVertex.z = (planes[i].normal.z >= 0) ? max.z : min.z;

                // If positive vertex is outside this plane, AABB is completely outside frustum
                if (planes[i].distanceToPoint(positiveVertex) < 0) {
                    return false;
                }
            }
            return true; // AABB intersects or is inside frustum
        }

        bool intersectsFrustum(const Frustum& other_frustum) const {

        }
    };
}