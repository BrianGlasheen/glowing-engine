#pragma once

#include "glm/glm.hpp"

#include <stdio.h>

namespace Util {
	struct AABB {
		glm::vec3 min;
		glm::vec3 max;
	};

	inline void print_AABB(const AABB& aabb) {
		printf("%f, %f, %f, %f, %f, %f\n", aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x, aabb.max.y, aabb.max.z);
	}

	inline AABB transform_aabb(AABB aabb, const glm::mat4& transform) {
        glm::vec3 corners[8] = {
            {aabb.min.x, aabb.min.y, aabb.min.z},
            {aabb.max.x, aabb.min.y, aabb.min.z},
            {aabb.max.x, aabb.max.y, aabb.min.z},
            {aabb.min.x, aabb.max.y, aabb.min.z},
            {aabb.min.x, aabb.min.y, aabb.max.z},
            {aabb.max.x, aabb.min.y, aabb.max.z},
            {aabb.max.x, aabb.max.y, aabb.max.z},
            {aabb.min.x, aabb.max.y, aabb.max.z}
        };

        AABB result;
        result.min = glm::vec3(FLT_MAX);
        result.max = glm::vec3(-FLT_MAX);

        for (const auto& corner : corners) {
            glm::vec3 transformed = glm::vec3(transform * glm::vec4(corner, 1.0f));
            result.min = glm::min(result.min, transformed);
            result.max = glm::max(result.max, transformed);
        }

        return result;
	}
}
