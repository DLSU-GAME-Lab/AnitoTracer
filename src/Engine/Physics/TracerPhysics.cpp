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
			//std::cout << "[Frame " << frameCount << "] Body " << body.GetID().GetIndexAndSequenceNumber() <<" at: ("
			//	<< physicsPosition.GetX() << ", " << physicsPosition.GetY() << ", " << physicsPosition.GetZ()
			//	<< ") | Active: " << body.IsActive() << std::endl;
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

void TracerPhysics::ToggleBodyActivation(GameObject* obj, bool activate)
{
	if (!obj)
	{
		std::cout << "ToggleBodyActivation: GameObject pointer is null!" << endl;
		return;
	}

	JPH::BodyID targetBodyID;
	bool found = false;

	for (const auto& pair : physics_body_pairs)
	{
		if (pair.gameObject == obj)
		{
			targetBodyID = pair.bodyID;
			found = true;
			break;
		}
	}

	if (!found || targetBodyID.IsInvalid())
	{
		std::cout << "ToggleBodyActivation: GameObject not found in physics pairs or BodyID is invalid!" << endl;
		return;
	}

	PhysicsEngine* engine = PhysicsEngine::GetInstance();
	JPH::BodyInterface& body_interface = engine->physics_system.GetBodyInterface();

	if (activate)
	{
		// 1. Restore to dynamic motion type and wake it up
		body_interface.SetMotionType(targetBodyID, JPH::EMotionType::Dynamic, JPH::EActivation::Activate);
		std::cout << "ToggleBodyActivation: Body " << targetBodyID.GetIndexAndSequenceNumber() << " Activated (Awake)." << std::endl;
	}
	else
	{
		// 2. Clear all linear and angular momentum immediately
		body_interface.SetLinearAndAngularVelocity(targetBodyID, JPH::Vec3::sZero(), JPH::Vec3::sZero());

		// 3. Force Jolt to put the body to sleep right now
		body_interface.DeactivateBody(targetBodyID);

		std::cout << "ToggleBodyActivation: Body " << targetBodyID.GetIndexAndSequenceNumber() << " Deactivated (Frozen & Sleeping)." << std::endl;
	}
}

void TracerPhysics::UpdateBodyPosition(GameObject* obj)
{
	if (!obj)
	{
		std::cout << "UpdateBodyPosition: GameObject pointer is null!" << endl;
		return;
	}

	// Find the corresponding BodyID for this GameObject
	JPH::BodyID targetBodyID;
	bool found = false;

	for (const auto& pair : physics_body_pairs)
	{
		if (pair.gameObject == obj)
		{
			targetBodyID = pair.bodyID;
			found = true;
			break;
		}
	}

	if (!found || targetBodyID.IsInvalid())
	{
		std::cout << "UpdateBodyPosition: GameObject not found in physics pairs or BodyID is invalid!" << endl;
		return;
	}

	PhysicsEngine* engine = PhysicsEngine::GetInstance();
	JPH::BodyInterface& body_interface = engine->physics_system.GetBodyInterface();

	// Get the GameObject's world position and convert to Jolt format
	glm::vec3 newPos = obj->getWorldPosition();
	JPH::RVec3 joltPos = JPH::RVec3(newPos.x, newPos.y, newPos.z);

	// Update the physics body's position
	body_interface.SetPosition(targetBodyID, joltPos, JPH::EActivation::Activate);

	std::cout << "UpdateBodyPosition: Body " << targetBodyID.GetIndexAndSequenceNumber()
		<< " position updated to (" << newPos.x << ", " << newPos.y << ", " << newPos.z << ")" << std::endl;
}

void TracerPhysics::UpdateBodyRotation(GameObject* obj)
{
	if (!obj)
	{
		std::cout << "UpdateBodyRotation: GameObject pointer is null!" << endl;
		return;
	}

	// Find the corresponding BodyID for this GameObject
	JPH::BodyID targetBodyID;
	bool found = false;

	for (const auto& pair : physics_body_pairs)
	{
		if (pair.gameObject == obj)
		{
			targetBodyID = pair.bodyID;
			found = true;
			break;
		}
	}

	if (!found || targetBodyID.IsInvalid())
	{
		std::cout << "UpdateBodyRotation: GameObject not found in physics pairs or BodyID is invalid!" << endl;
		return;
	}

	PhysicsEngine* engine = PhysicsEngine::GetInstance();
	JPH::BodyInterface& body_interface = engine->physics_system.GetBodyInterface();

	// Get the GameObject's world rotation and convert to Jolt format
	glm::quat newRot = obj->getLocalRotationQuat();
	JPH::Quat joltQuat = JPH::Quat(newRot.x, newRot.y, newRot.z, newRot.w);

	// Update the physics body's rotation
	body_interface.SetRotation(targetBodyID, joltQuat, JPH::EActivation::Activate);

	std::cout << "UpdateBodyRotation: Body " << targetBodyID.GetIndexAndSequenceNumber()
		<< " rotation updated to (" << newRot.x << ", " << newRot.y << ", " << newRot.z << ", " << newRot.w << ")" << std::endl;
}

void TracerPhysics::UpdateBodySize(GameObject* obj)
{
	if (!obj)
	{
		std::cout << "UpdateBodySize: GameObject pointer is null!" << endl;
		return;
	}

	// Find the corresponding BodyID for this GameObject
	JPH::BodyID targetBodyID;
	bool found = false;

	for (const auto& pair : physics_body_pairs)
	{
		if (pair.gameObject == obj)
		{
			targetBodyID = pair.bodyID;
			found = true;
			break;
		}
	}

	if (!found || targetBodyID.IsInvalid())
	{
		std::cout << "UpdateBodySize: GameObject not found in physics pairs or BodyID is invalid!" << endl;
		return;
	}

	PhysicsEngine* engine = PhysicsEngine::GetInstance();
	JPH::BodyInterface& body_interface = engine->physics_system.GetBodyInterface();

	// Get the GameObject's world scale
	glm::vec3 newScale = obj->getWorldScale();

	// Acquire read lock to get the current shape
	JPH::BodyLockRead lock(engine->physics_system.GetBodyLockInterface(), targetBodyID);

	if (!lock.Succeeded())
	{
		std::cout << "UpdateBodySize: Failed to acquire body lock!" << endl;
		return;
	}

	const JPH::Body& body = lock.GetBody();
	const JPH::ShapeRefC current_shape = body.GetShape();

	// Note: Jolt Physics doesn't support directly scaling shapes.
	// You would need to recreate the body with a new shape or use a scaling shape.
	// For now, we'll log this limitation.
	std::cout << "UpdateBodySize: Body " << targetBodyID.GetIndexAndSequenceNumber()
		<< " size update requested to scale (" << newScale.x << ", " << newScale.y << ", " << newScale.z << ")"
		<< " - Note: Direct shape scaling not supported. Consider recreating the body with new dimensions." << std::endl;
}

void TracerPhysics::SetBodyPosition(GameObject* obj, const glm::vec3& position)
{
	if (!obj)
	{
		std::cout << "SetBodyPosition: GameObject pointer is null!" << endl;
		return;
	}

	// Find the corresponding BodyID for this GameObject
	JPH::BodyID targetBodyID;
	bool found = false;

	for (const auto& pair : physics_body_pairs)
	{
		if (pair.gameObject == obj)
		{
			targetBodyID = pair.bodyID;
			found = true;
			break;
		}
	}

	if (!found || targetBodyID.IsInvalid())
	{
		std::cout << "SetBodyPosition: GameObject not found in physics pairs or BodyID is invalid!" << endl;
		return;
	}

	PhysicsEngine* engine = PhysicsEngine::GetInstance();
	JPH::BodyInterface& body_interface = engine->physics_system.GetBodyInterface();

	// Convert glm::vec3 to Jolt format and set the position
	JPH::RVec3 joltPos = JPH::RVec3(position.x, position.y, position.z);
	//Turn it off on set position to prevent physics from fighting with the position update
	body_interface.SetPosition(targetBodyID, joltPos, JPH::EActivation::DontActivate);

	std::cout << "SetBodyPosition: Body " << targetBodyID.GetIndexAndSequenceNumber()
		<< " position force set to (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
}

void TracerPhysics::SetBodyRotation(GameObject* obj, const glm::quat& rotation)
{
	if (!obj)
	{
		std::cout << "SetBodyRotation: GameObject pointer is null!" << endl;
		return;
	}

	// Find the corresponding BodyID for this GameObject
	JPH::BodyID targetBodyID;
	bool found = false;

	for (const auto& pair : physics_body_pairs)
	{
		if (pair.gameObject == obj)
		{
			targetBodyID = pair.bodyID;
			found = true;
			break;
		}
	}

	if (!found || targetBodyID.IsInvalid())
	{
		std::cout << "SetBodyRotation: GameObject not found in physics pairs or BodyID is invalid!" << endl;
		return;
	}

	PhysicsEngine* engine = PhysicsEngine::GetInstance();
	JPH::BodyInterface& body_interface = engine->physics_system.GetBodyInterface();

	// Convert glm::quat to Jolt format and set the rotation
	JPH::Quat joltQuat = JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w);
	//Turn it off on set position to prevent physics from fighting with the position update
	body_interface.SetRotation(targetBodyID, joltQuat, JPH::EActivation::DontActivate);

	std::cout << "SetBodyRotation: Body " << targetBodyID.GetIndexAndSequenceNumber()
		<< " rotation force set to (" << rotation.x << ", " << rotation.y << ", " << rotation.z << ", " << rotation.w << ")" << std::endl;
}

void TracerPhysics::SetBodySize(GameObject* obj, const glm::vec3& scale)
{
	if (!obj)
	{
		std::cout << "SetBodySize: GameObject pointer is null!" << endl;
		return;
	}

	// Find the corresponding BodyID for this GameObject
	JPH::BodyID targetBodyID;
	bool found = false;

	for (const auto& pair : physics_body_pairs)
	{
		if (pair.gameObject == obj)
		{
			targetBodyID = pair.bodyID;
			found = true;
			break;
		}
	}

	if (!found || targetBodyID.IsInvalid())
	{
		std::cout << "SetBodySize: GameObject not found in physics pairs or BodyID is invalid!" << endl;
		return;
	}

	// Note: Jolt Physics doesn't support directly scaling shapes.
	// You would need to recreate the body with a new shape or use a scaling shape.
	// For now, we'll log this limitation.
	std::cout << "SetBodySize: Body " << targetBodyID.GetIndexAndSequenceNumber()
		<< " size set requested to scale (" << scale.x << ", " << scale.y << ", " << scale.z << ")"
		<< " - Note: Direct shape scaling not supported. Consider recreating the body with new dimensions." << std::endl;
}
