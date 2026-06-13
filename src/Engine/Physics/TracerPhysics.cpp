#include "TracerPhysics.h"

void TracerPhysics::Initialize()
{
	if (floor_initialized)
	{
		std::cout << "Floor already initialized, skipping..." << endl;
		return;
	}

	PhysicsEngine::GetInstance()->CreateDefaultFloor(glm::vec3(0, -100, 0));
	floor_initialized = true;
	std::cout << "TracerPhysics: Default floor initialized" << endl;
}

void TracerPhysics::AddSphere(GameObject* obj)
{
	// Ensure floor is initialized before adding objects
	if (!floor_initialized)
	{
		Initialize();
	}

	auto pos = obj->getWorldPosition();
	auto scale = obj->getWorldScale();

	auto bodyID = PhysicsEngine::GetInstance()->CreateSphere(scale.x, pos);

	AddPair(obj, bodyID);
}

void TracerPhysics::AddBox(GameObject* obj, bool isStatic)
{
	// Ensure floor is initialized before adding objects
	if (!floor_initialized)
	{
		Initialize();
	}

	auto pos = obj->getWorldPosition();
	auto scale = obj->getWorldScale();
	auto rot = obj->getLocalRotationQuat();

	BodyID boxID;

	if (isStatic) {
		boxID = PhysicsEngine::GetInstance()->CreateStaticBox(scale, pos, rot);

	}
	else {
		boxID = PhysicsEngine::GetInstance()->CreateBox(scale, pos, rot);
	}

	AddPair(obj, boxID);
}

//Bridge Function- so I only call one thing later
//Only broadcast dirty if outside gameRenderer
void TracerPhysics::Step(float deltaTime, bool broadcastSceneDirty)
{
	PhysicsEngine::GetInstance()->Step(deltaTime);
	SyncPhysicsToGameObjects();

	if(broadcastSceneDirty)
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void TracerPhysics::SyncPhysicsToGameObjects(bool broadcastDirty)
{
	PhysicsEngine* engine = PhysicsEngine::GetInstance();
	BodyInterface* body_interface = &engine->physics_system.GetBodyInterface();

	for (const auto& pair : physics_body_pairs)
	{
		if (!pair.gameObject || pair.bodyID.IsInvalid())
			continue;

		// Acquire read lock to safely access the body
		JPH::BodyLockRead lock(engine->physics_system.GetBodyLockInterface(), pair.bodyID);

		if (!lock.Succeeded())
			continue;

		const JPH::Body& body = lock.GetBody();

		// Get position and rotation from physics body
		JPH::RVec3 physicsPosition = body.GetPosition();
		JPH::Quat physicsRotation = body.GetRotation();

		// Debug output for EVERY frame to see what's happening
		static int frameCount = 0;
		if (frameCount < 100)  // More frames for better debugging
		{
			std::cout << "[Frame " << frameCount << "] Body " << body.GetID().GetIndexAndSequenceNumber() <<" at: ("
				<< physicsPosition.GetX() << ", " << physicsPosition.GetY() << ", " << physicsPosition.GetZ()
				<< ") | Active: " << body.IsActive() << std::endl;
			frameCount++;
		}

		// Convert from Jolt types to GLM types
		glm::vec3 newPosition = PhysicsUtils::ToGlmVec3(physicsPosition);
		glm::quat newRotation = PhysicsUtils::ToGlmQuat(physicsRotation);

		// Update the GameObject's world position and rotation
		// Physics engine works in world space, so set world position
		pair.gameObject->setLocalPosition (newPosition);
		pair.gameObject->setLocalRotationQuat(newRotation);
	}

	// Only broadcast scene dirty if explicitly requested (expensive operation)
	// This allows physics updates without triggering full scene rebuilds
	if (broadcastDirty)
	{
		EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
	}
}

void TracerPhysics::AddPair(GameObject* obj, JPH::BodyID id)
{
	if (id.IsInvalid()) {
		std::cout << "Physics Body pair fail" << endl;
	}

	physics_body_pairs.emplace_back(obj, id);
}
