#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// includ this stuff cus
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <util/aabb.h>
#include <util/obb.h>

namespace JPH {
    class BodyInterface;
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystemThreadPool;
    class Body;
    class Shape;
    using BodyID = class BodyID;
}
// includ this stuff cus

namespace Physics {

    bool init();
    void shutdown();
    void update(float deltaTime = 1.0f / 60.0f);
    void optimize_broad_phase();
    // todo add call to somewhere OptimizeBroadPhase();
    // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
    // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
    // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.

    JPH::BodyID addBox(const glm::vec3& pos, const glm::vec3& size, bool isStatic = false);
    JPH::BodyID addSphere(const glm::vec3& pos, float radius, bool isStatic = false);
    void removeBody(JPH::BodyID id);

    glm::vec3 getBodyPosition(JPH::BodyID id);
    void setBodyPosition(JPH::BodyID id, const glm::vec3& pos);
    glm::vec3 getBodyVelocity(JPH::BodyID id);
    void setBodyVelocity(JPH::BodyID id, const glm::vec3& vel);
    glm::quat getBodyRotation(JPH::BodyID id);
    void setBodyRotation(JPH::BodyID id, const glm::quat& rot);

    Util::AABB getWorldAABB(JPH::BodyID id);
    Util::OBB getShapeOBB(JPH::BodyID id);

    bool shoot(const glm::vec3& origin, const glm::vec3& direction, float force, float maxDistance);

    JPH::BodyInterface& getBodyInterface();

    //glm::vec3 toGlm(const JPH::RVec3& v);
    //glm::quat toGlm(const JPH::Quat& q);
    //JPH::RVec3 toJolt(const glm::vec3& v);
    //JPH::Quat toJolt(const glm::quat& q);


    struct CharacterController {
        JPH::BodyID body_id;
        glm::vec3 velocity = glm::vec3(0.0f);
        bool is_on_ground = false;
        float ground_distance = 0.0f;

        // Source-style movement constants
        static constexpr float MAX_SPEED = 320.0f;           // units per second
        static constexpr float ACCELERATION = 10.0f;         // ground acceleration
        static constexpr float AIR_ACCELERATION = 10.0f;     // air acceleration  
        static constexpr float FRICTION = 4.0f;              // ground friction
        static constexpr float STOP_SPEED = 100.0f;          // speed below which friction is amplified
        static constexpr float JUMP_SPEED = 268.3281572999747f; // jump velocity
        static constexpr float GRAVITY = 800.0f;             // gravity acceleration
        static constexpr float AIR_CAP = 30.0f;              // air acceleration cap
        static constexpr float GROUND_TRACE_DISTANCE = 2.0f; // distance to check for ground
    };

}
