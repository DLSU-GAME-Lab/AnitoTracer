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

	if (bodyID.IsInvalid()) {
		std::cout << "Physics Sphere pair fail" << endl;
	}

	physics_body_pairs.emplace_back(obj, bodyID);
}

//Bridge Function- so I only call one thing later
void TracerPhysics::Step(float deltaTime)
{
	PhysicsEngine::GetInstance()->Step(deltaTime);
	SyncPhysicsToGameObjects();
	//EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
}

void TracerPhysics::SyncPhysicsToGameObjects()
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

		// Debug output for first few frames
		static int frameCount = 0;
		if (frameCount < 5)
		{
			std::cout << "[Frame " << frameCount << "] Body Position: ";
			PhysicsUtils::PrintVec3(physicsPosition);
			std::cout << " | IsActive: " << body.IsActive() << std::endl;
			frameCount++;
		}

		// Convert from Jolt types to GLM types
		glm::vec3 newPosition = PhysicsUtils::ToGlmVec3(physicsPosition);
		glm::quat newRotation = PhysicsUtils::ToGlmQuat(physicsRotation);

		// Update the GameObject's world position and rotation
		// Use world position to ensure correct placement
		pair.gameObject->setLocalPosition(newPosition);
		pair.gameObject->setLocalRotationQuat(newRotation);
	}
}
