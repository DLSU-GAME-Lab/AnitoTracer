#include "TracerPhysics.h"

void TracerPhysics::SpawnFountain()
{
	std::cout << "Spawning Fountain" << std::endl;

	m_isFountainActive = true;
	m_spheresLeftToSpawn = 80;
	m_spawnTimer = 0.0f;
	std::cout << "Fountain triggered: 10 spheres queued." << std::endl;

	//glm::vec3 fountainPos = glm::vec3(0, -600, 0);

	////string name = "SpawnedSphere_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

	////auto sphere = GameObjectFactory::CreateSphere(name, 0.2f);

	////// Set the position before transferring ownership
	////sphere->setLocalPosition(fountainPos);
	////GameObject* spherePtr = sphere.get();

	//////SetBodyPosition(spherePtr, fountainPos);
	////ApplyForce(spherePtr, glm::vec3(0, 50, 0));
	////
	////ModelManager::getInstance()->addObject(std::move(sphere));

	//// Set up random generator for spray dispersion
	//std::random_device rd;
	//std::mt19937 gen(rd());
	//// Adjust these values to make the spray wider or tighter
	//std::uniform_real_distribution<float> lateralForce(-15.0f, 15.0f);
	//std::uniform_real_distribution<float> upwardForce(45.0f, 65.0f);

	//// Get a base timestamp to keep string generation fast and unique
	//auto baseTime = std::chrono::system_clock::now().time_since_epoch().count();

	//for (int i = 0; i < 10; ++i)
	//{
	//	// 1. Create unique name for each of the 100 spheres
	//	std::string name = "SpawnedSphere_" + std::to_string(baseTime) + "_" + std::to_string(i);

	//	// 2. Instantiate the sphere
	//	auto sphere = GameObjectFactory::CreateSphere(name, 0.2f);

	//	// 3. Set the initial position
	//	sphere->setLocalPosition(fountainPos);
	//	GameObject* spherePtr = sphere.get();

	//	// 4. Calculate a random fountain force vector
	//	float forceX = lateralForce(gen);
	//	float forceY = upwardForce(gen); // Strong upward push
	//	float forceZ = lateralForce(gen);
	//	glm::vec3 fountainForce(forceX, forceY, forceZ);

	//	// 5. Apply the physical force
	//	ApplyForce(spherePtr, fountainForce);

	//	// Optional: Mix in your previous random color logic if desired
	//	// TracerCollisionHandlers::ChangeColor(spherePtr);

	//	// 6. Transfer ownership to the manager
	//	ModelManager::getInstance()->addObject(std::move(sphere));
	//}

	//std::cout << "Done Spawning Fountain" << std::endl;
}

void TracerPhysics::UpdateFountain(float deltaTime)
{
	if (!m_isFountainActive || m_spheresLeftToSpawn <= 0)
	{
		m_isFountainActive = false;
		return;
	}

	// Accumulate the elapsed frame time
	m_spawnTimer += deltaTime;

	// Set up random generator for spray dispersion
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> lateralForce(-15.0f, 15.0f);
	static std::uniform_real_distribution<float> upwardForce(45.0f, 65.0f);

	glm::vec3 fountainPos = glm::vec3(0, -600, 0);
	auto baseTime = std::chrono::system_clock::now().time_since_epoch().count();

	// Spawn as many spheres as dictated by the elapsed time
	while (m_spawnTimer >= SPAWN_INTERVAL && m_spheresLeftToSpawn > 0)
	{
		// Decrement timer by the interval step
		m_spawnTimer -= SPAWN_INTERVAL;
		m_spheresLeftToSpawn--;

		// Unique name variant
		std::string name = "SpawnedSphere_" + std::to_string(baseTime) + "_" + std::to_string(m_spheresLeftToSpawn);

		// Instantiate
		auto sphere = GameObjectFactory::CreateSphere(name, 0.2f);
		sphere->setLocalPosition(fountainPos);
		GameObject* spherePtr = sphere.get();

		// Calculate and apply physical force
		glm::vec3 fountainForce(lateralForce(gen), upwardForce(gen), lateralForce(gen));
		UpdateBodyPosition(spherePtr);
		ApplyForce(spherePtr, fountainForce);

		// Transfer ownership
		ModelManager::getInstance()->addObject(std::move(sphere));
	}

	if (m_spheresLeftToSpawn == 0)
	{
		std::cout << "Done Spawning Fountain seamlessly." << std::endl;
	}
}

void TracerPhysics::AddSponzaColliders()
{
	std::cout << "Adding Sponza colliders..." << std::endl;
	if (sponza_colliders_added)
	{
		std::cout << "Sponza colliders have already been added. Skipping." << std::endl;
		return;
	}
	auto eng = PhysicsEngine::GetInstance();
	auto q_id = glm::identity<glm::quat>();

	//Ground Floor
	eng->CreateStaticBox(
		glm::vec3(55, 1, 24),
		glm::vec3(0, -670, 0),
		q_id
	);

	//LR Walls
	glm::vec3 LR_wall_size = glm::vec3(55, 655, 1);
	eng->CreateStaticBox(
		LR_wall_size,
		glm::vec3(0,0, -620),
		q_id
	);
	eng->CreateStaticBox(
		LR_wall_size,
		glm::vec3(0,0, 630),
		q_id
	);

	//UD Walls
	glm::vec3 UD_wall_size = glm::vec3(1, 55, 26);
	eng->CreateStaticBox(
		UD_wall_size,
		glm::vec3(1380, 0, 0),
		q_id
	);
	eng->CreateStaticBox(
		UD_wall_size,
		glm::vec3(-1380, 0, 0),
		q_id
	);

	//Middle Pillars
	glm::vec3 mid_pillar_size = glm::vec3(1.2f, 30.f, 1.2f);
	eng->CreateStaticBox(
		mid_pillar_size,
		glm::vec3(550, -80, 240),
		q_id
	);
	eng->CreateStaticBox(
		mid_pillar_size,
		glm::vec3(180, -80, 240),
		q_id
	);
	eng->CreateStaticBox(
		mid_pillar_size,
		glm::vec3(-190, -80, 240),
		q_id
	);
	eng->CreateStaticBox(
		mid_pillar_size,
		glm::vec3(-560, -80, 240),
		q_id
	);

	eng->CreateStaticBox(
		mid_pillar_size,
		glm::vec3(550, -80, -240),
		q_id
	);
	eng->CreateStaticBox(
		mid_pillar_size,
		glm::vec3(180, -80, -240),
		q_id
	);
	eng->CreateStaticBox(
		mid_pillar_size,
		glm::vec3(-190, -80, -240),
		q_id
	);
	eng->CreateStaticBox(
		mid_pillar_size,
		glm::vec3(-560, -80, -240),
		q_id
	);

	//Corner Pillars
	glm::vec3 corner_pillar_size = glm::vec3(1.9f, 30.f, 1.9f);
	eng->CreateStaticBox(
		corner_pillar_size,
		glm::vec3(-940, -80, -230),
		q_id
	);
	eng->CreateStaticBox(
		corner_pillar_size,
		glm::vec3(930, -80, -230),
		q_id
	);
	eng->CreateStaticBox(
		corner_pillar_size,
		glm::vec3(930, -80, 230),
		q_id
	);
	eng->CreateStaticBox(
		corner_pillar_size,
		glm::vec3(-940, -80, 230),
		q_id
	);

	//Second Floor Floors
	float secondFloorY = -260;
	glm::vec3 scaleLR2 = glm::vec3(55, 1, 7);
	eng->CreateStaticBox(
		scaleLR2,
		glm::vec3(0, secondFloorY, 450),
		glm::identity<glm::quat>()
	);

	eng->CreateStaticBox(
		scaleLR2,
		glm::vec3(0, secondFloorY, -450),
		glm::identity<glm::quat>()
	);

	glm::vec3 scaleUD = glm::vec3(9, 1, 12);
	eng->CreateStaticBox(
		scaleUD,
		glm::vec3(1150, secondFloorY, 0),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		scaleUD,
		glm::vec3(-1150, secondFloorY, 0),
		glm::identity<glm::quat>()
	);

	//Second Floor Pillars
	glm::vec3 second_floor_pillar_size = glm::vec3(0.9f, 8.f, 0.9f);
	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(940, -80, 3),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(-940, -80, 3),
		glm::identity<glm::quat>()
	);

	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(-744, -80, 245),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(-374, -80, 245),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(-4, -80, 245),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(366, -80, 245),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(736, -80, 245),
		glm::identity<glm::quat>()
	);

	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(-744, -80, -242),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(-374, -80, -242),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(-4, -80, -242),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(366, -80, -242),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		second_floor_pillar_size,
		glm::vec3(736, -80, -242),
		glm::identity<glm::quat>()
	);

	//2nd Floor Smoll Walls
	glm::vec3 SmollWallLR = glm::vec3(1.5f, 2.8f, 11.f);
	eng->CreateStaticBox(
		SmollWallLR,
		glm::vec3(938, -205, 0),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		SmollWallLR,
		glm::vec3(-950, -205, 0),
		glm::identity<glm::quat>()
	);

	glm::vec3 SmollWallUD = glm::vec3(39.f, 2.8f, 1.5f);
	eng->CreateStaticBox(
		SmollWallUD,
		glm::vec3(0, -205, 250),
		glm::identity<glm::quat>()
	);
	eng->CreateStaticBox(
		SmollWallUD,
		glm::vec3(0, -205, -245),
		glm::identity<glm::quat>()
	);

	sponza_colliders_added = true;
	std::cout << "Sponza colliders added." << std::endl;
}

void TracerPhysics::Initialize()
{
	if (floor_initialized)
	{
		std::cout << "Floor already initialized, skipping..." << endl;
		return;
	}

	auto tracerCollisionHandler = new TracerCollisionHandlers();
	PhysicsEngine::GetInstance()->SetContactListener(tracerCollisionHandler);

	//PhysicsEngine::GetInstance()->CreateDefaultFloor(glm::vec3(0, -100, 0));
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
	auto scale = obj->getLocalScale();

	auto bodyID = PhysicsEngine::GetInstance()->CreateSphere(scale.x, pos);

	AddPair(obj, bodyID);

	//Turn off physics by default
	ToggleBodyActivation(obj, false);
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
	UpdateFountain(deltaTime);

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

		//Revalidate
		if (!pair.gameObject)
			continue;

		// Update the GameObject's world position and rotation
		// Physics engine works in world space, so set world position
		pair.gameObject->setLocalPosition(newPosition);
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
	// Validate both GameObject and BodyID before adding
	if (!obj) {
		std::cout << "Physics Body pair fail: GameObject is null" << endl;
		return;
	}

	if (id.IsInvalid()) {
		std::cout << "Physics Body pair fail: BodyID is invalid" << endl;
		return;
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

void TracerPhysics::ApplyForce(GameObject* obj, const glm::vec3& force)
{
	if (!obj)
	{
		std::cout << "ApplyForce: GameObject pointer is null!" << endl;
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
		std::cout << "ApplyForce: GameObject not found in physics pairs or BodyID is invalid!" << endl;
		return;
	}

	PhysicsEngine* engine = PhysicsEngine::GetInstance();
	JPH::BodyInterface& body_interface = engine->physics_system.GetBodyInterface();

	// Convert glm::vec3 to Jolt format
	JPH::Vec3 joltForce = JPH::Vec3(force.x, force.y, force.z);

	// Apply the force to the body
	body_interface.AddForce(targetBodyID, joltForce, JPH::EActivation::Activate);

	body_interface.SetLinearVelocity(targetBodyID, joltForce);

	std::cout << "ApplyForce: Body " << targetBodyID.GetIndexAndSequenceNumber()
		<< " force applied: (" << force.x << ", " << force.y << ", " << force.z << ")" << std::endl;
}

GameObject* TracerPhysics::GetGameObjectByBodyID(JPH::BodyID bodyID) const
{
	if (bodyID.IsInvalid())
	{
		std::cout << "GetGameObjectByBodyID: BodyID is invalid!" << endl;
		return nullptr;
	}

	for (const auto& pair : physics_body_pairs)
	{
		if (pair.bodyID == bodyID)
		{
			return pair.gameObject;
		}
	}

	std::cout << "GetGameObjectByBodyID: No GameObject found for BodyID " << bodyID.GetIndexAndSequenceNumber() << std::endl;
	return nullptr;
}
