#include "physics.h"
#include <iostream>
#include <cstdarg>
#include <thread>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/RayCast.h>
//#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CastResult.h>

using namespace JPH;
using namespace JPH::literals;

namespace Physics {

    class ObjectLayerPairFilterImpl;
    class BPLayerInterfaceImpl;
    class ObjectVsBroadPhaseLayerFilterImpl;
    class MyContactListener;
    class MyBodyActivationListener;

    struct PhysicsState {
        std::unique_ptr<TempAllocatorImpl> tempAllocator;
        std::unique_ptr<JobSystemThreadPool> jobSystem;
        std::unique_ptr<PhysicsSystem> physicsSystem;
        std::unique_ptr<BPLayerInterfaceImpl> broadPhaseLayerInterface;
        std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> objectVsBroadphaseLayerFilter;
        std::unique_ptr<ObjectLayerPairFilterImpl> objectVsObjectLayerFilter;
        std::unique_ptr<MyBodyActivationListener> bodyActivationListener;
        std::unique_ptr<MyContactListener> contactListener;
    };

    static PhysicsState g_state;

    namespace Layers {
        static constexpr ObjectLayer NON_MOVING = 0;
        static constexpr ObjectLayer MOVING = 1;
        static constexpr ObjectLayer CHARACTER = 2;
        static constexpr ObjectLayer NUM_LAYERS = 3;
    };

    namespace BroadPhaseLayers {
        static constexpr BroadPhaseLayer NON_MOVING(0);
        static constexpr BroadPhaseLayer MOVING(1);
        static constexpr uint NUM_LAYERS(2);
    };

    static void TraceImpl(const char* inFMT, ...) {
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);
        std::cout << buffer << std::endl;
    }

#ifdef JPH_ENABLE_ASSERTS
    static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
        std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;
        return true;
    };
#endif

    class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
    public:
        virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override {
            switch (inObject1) {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
    public:
        BPLayerInterfaceImpl() {
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        virtual uint GetNumBroadPhaseLayers() const override {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
            switch ((BroadPhaseLayer::Type)inLayer) {
            case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
            case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
            default: JPH_ASSERT(false); return "INVALID";
            }
        }
#endif

    private:
        BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
    public:
        virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
            switch (inLayer1) {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
            }
        }
    };

    class MyContactListener : public ContactListener {
    public:
        virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override {
            return ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override {
            // Handle contact added
            //std::cout << "A contact was added" << std::endl;
        }

        virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override {
            // Handle contact persisted
            //std::cout << "A contact was persisted" << std::endl;
        }

        virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override {
            // Handle contact removed
            //std::cout << "A contact was removed" << std::endl;
        }
    };

    class MyBodyActivationListener : public BodyActivationListener {
    public:
        virtual void OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override {
            std::cout << "A body got activated" << std::endl;
        }

        virtual void OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override {
            std::cout << "A body went to sleep" << std::endl;
        }
    };

    // Public API Implementation
    bool init() {
        // Register allocation hook.In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
        // This needs to be done before any other Jolt function is called.
        RegisterDefaultAllocator();

        // Install trace and assert callbacks
        Trace = TraceImpl;
        JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

        // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
        // It is not directly used in this example but still required.
        Factory::sInstance = new Factory();

        // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
        // If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
        // If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
        RegisterTypes();

        // We need a temp allocator for temporary allocations during the physics update. We're
        // pre-allocating 10 MB to avoid having to do allocations during the physics update.
        // B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
        // If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
        // malloc / free.
        g_state.tempAllocator = std::make_unique<TempAllocatorImpl>(10 * 1024 * 1024);

        // We need a job system that will execute physics jobs on multiple threads. Typically
        // you would implement the JobSystem interface yourself and let Jolt Physics run on top
        // of your own job scheduler. JobSystemThreadPool is an example implementation.
        g_state.jobSystem = std::make_unique<JobSystemThreadPool>(cMaxPhysicsJobs, cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
        // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
        const uint cMaxBodies = 65536;
        // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
        const uint cNumBodyMutexes = 0;
        // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
        // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
        // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
        // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
        const uint cMaxBodyPairs = 65536;
        // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
        // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
        // Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
        const uint cMaxContactConstraints = 10240;

        // Create mapping table from object layer to broadphase layer
        // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
        // Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
        g_state.broadPhaseLayerInterface = std::make_unique<BPLayerInterfaceImpl>();
        // Create class that filters object vs broadphase layers
        // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
        // Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
        g_state.objectVsBroadphaseLayerFilter = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
        // Create class that filters object vs object layers
        // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
        // Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
        g_state.objectVsObjectLayerFilter = std::make_unique<ObjectLayerPairFilterImpl>();

        // Now we can create the actual physics system.
        g_state.physicsSystem = std::make_unique<PhysicsSystem>();
        g_state.physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *g_state.broadPhaseLayerInterface, *g_state.objectVsBroadphaseLayerFilter, *g_state.objectVsObjectLayerFilter);

        // A body activation listener gets notified when bodies activate and go to sleep
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.
        g_state.bodyActivationListener = std::make_unique<MyBodyActivationListener>();
        // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.
        g_state.contactListener = std::make_unique<MyContactListener>();

        // A body activation listener gets notified when bodies activate and go to sleep
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.
        g_state.physicsSystem->SetBodyActivationListener(g_state.bodyActivationListener.get());
        // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.   
        g_state.physicsSystem->SetContactListener(g_state.contactListener.get());

        printf("[PHYSICS] initialized successfully\n");
        return true;
    }

    void shutdown() {
        if (g_state.physicsSystem) {
            g_state.physicsSystem->SetBodyActivationListener(nullptr);
            g_state.physicsSystem->SetContactListener(nullptr);
        }

        g_state.contactListener.reset();
        g_state.bodyActivationListener.reset();
        g_state.physicsSystem.reset();
        g_state.objectVsObjectLayerFilter.reset();
        g_state.objectVsBroadphaseLayerFilter.reset();
        g_state.broadPhaseLayerInterface.reset();
        g_state.jobSystem.reset();
        g_state.tempAllocator.reset();

        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }

    void update(float deltaTime) {
        const int cCollisionSteps = 1;
        g_state.physicsSystem->Update(deltaTime, cCollisionSteps, g_state.tempAllocator.get(), g_state.jobSystem.get());
    }

    void optimize_broad_phase() {
        g_state.physicsSystem->OptimizeBroadPhase();
    }

    JPH::BodyID addBox(const glm::vec3& pos, const glm::vec3& size, bool isStatic) {
        // Create box shape
        RefConst<Shape> box_shape = new BoxShape(Vec3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));

        // Create body creation settings
        Vec3 box_half_extents = Vec3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
        RVec3 corrected_pos = RVec3(pos.x, pos.y + box_half_extents.GetY(), pos.z);

        BodyCreationSettings body_settings(box_shape, RVec3(pos.x, pos.y, pos.z),
            Quat::sIdentity(), isStatic ? EMotionType::Static : EMotionType::Dynamic,
            isStatic ? Layers::NON_MOVING : Layers::MOVING);

        body_settings.mRestitution = 0.2f;

        // Create body
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();
        BodyID body_id = body_interface.CreateAndAddBody(body_settings, EActivation::Activate);

        return body_id;
    }

    JPH::BodyID addSphere(const glm::vec3& pos, float radius, bool isStatic) {
        // Create sphere shape
        RefConst<Shape> sphere_shape = new SphereShape(radius);

        // Create body creation settings
        BodyCreationSettings body_settings(sphere_shape, RVec3(pos.x, pos.y, pos.z),
            Quat::sIdentity(), isStatic ? EMotionType::Static : EMotionType::Dynamic,
            isStatic ? Layers::NON_MOVING : Layers::MOVING);

        // Create body
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();
        BodyID body_id = body_interface.CreateAndAddBody(body_settings, EActivation::Activate);

        return body_id;
    }

    void removeBody(JPH::BodyID id) {
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();
        body_interface.RemoveBody(id);
        body_interface.DestroyBody(id);
    }

    glm::vec3 getBodyPosition(JPH::BodyID id) {
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();
        RVec3 pos = body_interface.GetPosition(id);
        return glm::vec3(static_cast<float>(pos.GetX()), static_cast<float>(pos.GetY()), static_cast<float>(pos.GetZ()));
    }

    void setBodyPosition(JPH::BodyID id, const glm::vec3& pos) {
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();
        body_interface.SetPosition(id, RVec3(pos.x, pos.y, pos.z), EActivation::Activate);
    }

    glm::vec3 getBodyVelocity(JPH::BodyID id) {
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();
        Vec3 vel = body_interface.GetLinearVelocity(id);
        return glm::vec3(vel.GetX(), vel.GetY(), vel.GetZ());
    }

    void setBodyVelocity(JPH::BodyID id, const glm::vec3& vel) {
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();
        body_interface.SetLinearVelocity(id, Vec3(vel.x, vel.y, vel.z));
    }

    glm::quat getBodyRotation(JPH::BodyID id) {
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();
        Quat rot = body_interface.GetRotation(id);
        return glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());
    }

    void setBodyRotation(JPH::BodyID id, const glm::quat& rot) {
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();
        JPH::Quat joltQuat(rot.x, rot.y, rot.z, rot.w);
        body_interface.SetRotation(id, joltQuat, JPH::EActivation::Activate);
    }


    Util::AABB getWorldAABB(JPH::BodyID id) {
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();

        RefConst<Shape> shape = body_interface.GetShape(id);
        RVec3 position = body_interface.GetPosition(id);
        Quat rotation = body_interface.GetRotation(id);

        AABox world_bounds = shape->GetWorldSpaceBounds(Mat44::sRotationTranslation(rotation, position), Vec3::sReplicate(1.0f));

        glm::vec3 min_bounds(world_bounds.mMin.GetX(), world_bounds.mMin.GetY(), world_bounds.mMin.GetZ());
        glm::vec3 max_bounds(world_bounds.mMax.GetX(), world_bounds.mMax.GetY(), world_bounds.mMax.GetZ());

        return Util::AABB{ min_bounds, max_bounds };
    }

    Util::OBB getShapeOBB(JPH::BodyID id) {
        BodyInterface& body_interface = g_state.physicsSystem->GetBodyInterface();

        // Get the shape from the body
        RefConst<Shape> shape = body_interface.GetShape(id);

        // Get the local bounding box of the shape
        AABox local_bounds = shape->GetLocalBounds();

        // Get the body's world transform (position + rotation)
        RVec3 position = body_interface.GetPosition(id);
        Quat rotation = body_interface.GetRotation(id);

        // Create transformation matrix from Jolt transform
        glm::mat4 physics_transform = glm::mat4(1.0f);
        // Apply rotation (convert Jolt quaternion to glm)
        glm::quat glm_rot(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
        glm::mat4 rot_matrix = glm::mat4_cast(glm_rot);
        physics_transform = rot_matrix;
        // Apply translation
        physics_transform[3] = glm::vec4(position.GetX(), position.GetY(), position.GetZ(), 1.0f);

        // Get the 8 corners of the local collision box
        glm::vec3 local_min(local_bounds.mMin.GetX(), local_bounds.mMin.GetY(), local_bounds.mMin.GetZ());
        glm::vec3 local_max(local_bounds.mMax.GetX(), local_bounds.mMax.GetY(), local_bounds.mMax.GetZ());

        Util::OBB result;
        
        glm::vec3 local_corners[8] = {
            glm::vec3(local_min.x, local_min.y, local_min.z), // 0: min corner
            glm::vec3(local_max.x, local_min.y, local_min.z), // 1: +x
            glm::vec3(local_min.x, local_max.y, local_min.z), // 2: +y  
            glm::vec3(local_max.x, local_max.y, local_min.z), // 3: +x+y
            glm::vec3(local_min.x, local_min.y, local_max.z), // 4: +z
            glm::vec3(local_max.x, local_min.y, local_max.z), // 5: +x+z
            glm::vec3(local_min.x, local_max.y, local_max.z), // 6: +y+z
            glm::vec3(local_max.x, local_max.y, local_max.z)  // 7: max corner
        };

        // Transform all corners to world space
        for (int i = 0; i < 8; i++) {
            glm::vec4 world_corner = physics_transform * glm::vec4(local_corners[i], 1.0f);
            result.corners[i] = glm::vec3(world_corner);
        }

        return result;
    }

    bool shoot(const glm::vec3& origin, const glm::vec3& direction,
        float force, float maxDistance) {

        // Debug output
        printf("Shooting from (%.2f, %.2f, %.2f) in direction (%.2f, %.2f, %.2f)\n",
            origin.x, origin.y, origin.z, direction.x, direction.y, direction.z);

        // Create ray
        Vec3 joltOrigin(origin.x, origin.y, origin.z);
        Vec3 joltDir = Vec3(direction.x, direction.y, direction.z).Normalized();
        RRayCast ray(joltOrigin, joltDir * maxDistance);

        // Cast ray
        RayCastResult result;
        if (g_state.physicsSystem->GetNarrowPhaseQuery().CastRay(ray, result)) {
            printf("Ray hit something! BodyID: %u, Fraction: %.3f\n",
                result.mBodyID.GetIndexAndSequenceNumber(), result.mFraction);

            // Calculate hit point from ray fraction
            Vec3 hitPoint = joltOrigin + joltDir * maxDistance * result.mFraction;
            printf("Hit point: (%.2f, %.2f, %.2f)\n", hitPoint.GetX(), hitPoint.GetY(), hitPoint.GetZ());

            // Check if body is dynamic
            BodyInterface& bodyInterface = g_state.physicsSystem->GetBodyInterface();
            EMotionType motionType = bodyInterface.GetMotionType(result.mBodyID);
            printf("Body motion type: %d (0=Static, 1=Kinematic, 2=Dynamic)\n", (int)motionType);

            if (motionType == EMotionType::Dynamic) {
                // Apply impulse
                Vec3 impulse = joltDir * force;
                printf("Applying impulse: (%.2f, %.2f, %.2f) with force %.2f\n",
                    impulse.GetX(), impulse.GetY(), impulse.GetZ(), force);
                bodyInterface.AddImpulse(result.mBodyID, impulse, hitPoint);
                printf("Impulse applied successfully!\n");
            }
            else {
                printf("Body is not dynamic, cannot apply impulse\n");
            }
            return true;
        }
        else {
            printf("Ray didn't hit anything\n");
        }
        return false;
    }

    JPH::BodyInterface& getBodyInterface() {
        return g_state.physicsSystem->GetBodyInterface();
    }

    //glm::vec3 toGlm(const JPH::RVec3& v) {
    //    return glm::vec3(static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ()));
    //}

    //glm::quat toGlm(const JPH::Quat& q) {
    //    return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
    //}

    //JPH::RVec3 toJolt(const glm::vec3& v) {
    //    return JPH::RVec3(v.x, v.y, v.z);
    //}

    //JPH::Quat toJolt(const glm::quat& q) {
    //    return JPH::Quat(q.x, q.y, q.z, q.w);
    //}

}