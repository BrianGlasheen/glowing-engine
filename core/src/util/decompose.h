#pragma once

#include "glm/glm.hpp"

namespace Util {
  /*  void decompose(const glm::mat4& m, glm::vec3& pos, glm::quat& rot, glm::vec3& scale) {
        pos = m[3];

        for (int i = 0; i < 3; i++)
            scale[i] = glm::length(vec3(m[i]));

        const glm::mat3 rotMtx(
            glm::vec3(m[0]) / scale[0],
            glm::vec3(m[1]) / scale[1],
            glm::vec3(m[2]) / scale[2]);

        rot = glm::quat_cast(rotMtx);
    }*/

    inline void decompose(const glm::mat4& m, glm::vec3& pos, glm::vec3& scale, glm::vec3& rot) {
        // Extract translation directly
        pos = glm::vec3(m[3]);

        // Extract scale by getting length of basis vectors
        glm::vec3 col0 = glm::vec3(m[0]);
        glm::vec3 col1 = glm::vec3(m[1]);
        glm::vec3 col2 = glm::vec3(m[2]);

        scale.x = glm::length(col0);
        scale.y = glm::length(col1);
        scale.z = glm::length(col2);

        // Normalize columns to remove scale from rotation matrix
        if (scale.x != 0.0f) col0 /= scale.x;
        if (scale.y != 0.0f) col1 /= scale.y;
        if (scale.z != 0.0f) col2 /= scale.z;

        glm::mat3 rotationMatrix(col0, col1, col2);
        glm::quat rotationQuat = glm::quat_cast(rotationMatrix);
        rot = glm::eulerAngles(rotationQuat);
    }
}
