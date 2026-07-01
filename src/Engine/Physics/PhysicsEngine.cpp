#include "PhysicsEngine.h"

// Initialize static member
PhysicsEngine* PhysicsEngine::instance = nullptr;

// Private constructor
PhysicsEngine::PhysicsEngine() {}

PhysicsEngine::~PhysicsEngine()
{
	delete temp_allocator;
	delete job_system;
	delete broad_phase_layer_interface;
	delete object_vs_broadphase_layer_filter;
	delete object_vs_object_layer_filter;
	delete body_activation_listener;
	delete contact_listener;
}

// Get singleton instance
PhysicsEngine* PhysicsEngine::GetInstance()
{
	if (instance == nullptr) {
		std::cout << "Physics Engine not yet initialized- Initializing" << endl;

		instance = new PhysicsEngine();
		instance->InitializeEngine();
	}
	return instance;
}

void PhysicsEngine::DestroyInstance()
{
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;

		// Clear out the Jolt global factory reference to avoid memory leaks
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}
}

void PhysicsEngine::InitializeEngine()
{
	RegisterDefaultAllocator();

	JPH::Factory::sInstance = new JPH::Factory();

	RegisterTypes();

	// Allocate to heap so variables survive outside this function scope
	temp_allocator = new TempAllocatorImpl(10 * 1024 * 1024);
	job_system = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

	broad_phase_layer_interface = new BPLayerInterfaceImpl();
	object_vs_broadphase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();
	object_vs_object_layer_filter = new ObjectLayerPairFilterImpl();

	std::cout << "Creating Physics System" << endl;
	// Now we can create the actual physics system.
	physics_system.Init(
		cMaxBodies,
		cNumBodyMutexes,
		cMaxBodyPairs,
		cMaxContactConstraints,
		*broad_phase_layer_interface,
		*object_vs_broadphase_layer_filter,
		*object_vs_object_layer_filter
	);

	body_activation_listener = new MyBodyActivationListener();
	physics_system.SetBodyActivationListener(body_activation_listener);

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	contact_listener = new MyContactListener();
	contact_listenerManager = new ContactListenerManager();

	physics_system.SetContactListener(contact_listenerManager);



	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)

	std::cout << "Get Body Interface" << endl;
	body_interface = &physics_system.GetBodyInterface();

	// Set gravity - Standard Earth gravity is -9.81 m/s^2 on the Y axis
	physics_system.SetGravity(Vec3(0.0f, -9.81f, 0.0f));
	std::cout << "Gravity set to: (0, -9.81, 0)" << endl;

	SetContactListener(contact_listener);

	std::cout << "Done Initializing Physics System" << endl;
}

void PhysicsEngine::Step(float deltaTime)
{
	physics_system.Update(deltaTime, 1, temp_allocator, job_system);
}

void PhysicsEngine::SetContactListener(ContactListener* listener, bool useDefaultType)
{
	if (listener != nullptr) {
		// Delete the existing listener if it was created by the engine
		if (contact_listener != nullptr) {
			//delete contact_listener;
		}

		//Use default type- only for initialization-
		//Will remove laturs desu
		if (useDefaultType) {
			// Assign the new listener - use a cast since we expect MyContactListener
			contact_listener = static_cast<MyContactListener*>(listener);
			contact_listenerManager->AddListener(contact_listener);
		}
		else {
			contact_listenerManager->AddListener(listener);
		}
		
		std::cout << "Contact listener assigned to physics system successfully" << endl;
	}
	else {
		std::cout << "Warning: Attempted to assign null contact listener" << endl;
	}
}

void PhysicsEngine::CreateDefaultFloor(glm::vec3 pos)
{
	BoxShapeSettings floorSettings(Vec3(100.0f, 1.0f, 100.0f));
	floorSettings.SetDensity(1.0f); // Ensure proper density

	ShapeSettings::ShapeResult shapeRes = floorSettings.Create();
	ShapeRefC floorShape = shapeRes.Get();

	BodyCreationSettings floor_setting(
		floorShape,
		PhysicsUtils::ToJoltVec3(pos),
		Quat::sIdentity(), 
		EMotionType::Static, 
		Layers::NON_MOVING);

	floor_setting.mRestitution = 0.8f; // Bouncy test
	floor_setting.mCollisionGroup.SetGroupFilter(0); // Enable collision with all groups
	floor_setting.mMotionQuality = EMotionQuality::LinearCast; // Enable CCD for floor too

	Body* floor = body_interface->CreateBody(floor_setting);
	body_interface->AddBody(floor->GetID(), EActivation::DontActivate);

	std::cout << "Created floor at: ";
	PhysicsUtils::PrintVec3(PhysicsUtils::ToJoltVec3(pos));
	std::cout << " | Floor ID: " << floor->GetID().GetIndexAndSequenceNumber() << " | Layer: NON_MOVING" << std::endl;
}

void PhysicsEngine::CreateDefaultBall(float r, glm::vec3 pos)
{
	BodyCreationSettings sphere_settings(new SphereShape(r),
		PhysicsUtils::ToJoltVec3(pos),
		Quat::sIdentity(), EMotionType::Dynamic, 
		Layers::MOVING);

	sphere_settings.mRestitution = 0.8;//Bouncy test

	ball = body_interface->CreateAndAddBody(sphere_settings, EActivation::Activate);

	//body_interface->SetLinearVelocity(ball, Vec3(0.0f, -5.0f, 0.0f));
}

BodyID PhysicsEngine::CreateSphere(float r, glm::vec3 pos, float restitution)
{
	//Scale needs to offset by 50 >..>
	BodyCreationSettings sphere_settings(new SphereShape(r * 50),
		PhysicsUtils::ToJoltVec3(pos),
		Quat::sIdentity(), EMotionType::Dynamic,
		Layers::MOVING);

	sphere_settings.mRestitution = restitution;
	sphere_settings.mGravityFactor = 1.0f; // Ensure gravity is applied
	sphere_settings.mLinearDamping = 0.05f; // Slight damping to stabilize
	sphere_settings.mAngularDamping = 0.05f; // Slight rotational damping
	sphere_settings.mAllowedDOFs = EAllowedDOFs::All; // Allow all degrees of freedom
	sphere_settings.mMotionQuality = EMotionQuality::LinearCast; // Enable continuous collision detection
	sphere_settings.mCollisionGroup.SetGroupFilter(0); // Enable collision with ALL groups

	BodyID sphere_id = body_interface->CreateAndAddBody(sphere_settings, EActivation::Activate);

	std::cout << "Created sphere at: ";
	PhysicsUtils::PrintVec3(PhysicsUtils::ToJoltVec3(pos));
	std::cout << " with radius: " << r << " | Layer: MOVING | CCD: Enabled" << std::endl;

	return sphere_id;
}

BodyID PhysicsEngine::CreateBox(glm::vec3 scale, glm::vec3 pos, glm::quat rot, float restitution)
{
	//glm::vec3 half_extents = scale * 0.5f; // Jolt's BoxShape uses half extents
	glm::vec3 half_extents = scale * 25.f; //25 is the correct offset >..>

	BodyCreationSettings box_settings(new BoxShape(PhysicsUtils::ToJoltVec3(half_extents)),
		PhysicsUtils::ToJoltVec3(pos),
		PhysicsUtils::ToJoltQuat(rot),
		EMotionType::Dynamic,
		Layers::MOVING);

	box_settings.mRestitution = restitution;
	box_settings.mGravityFactor = 1.0f; // Ensure gravity is applied
	box_settings.mLinearDamping = 0.05f; // Slight damping to stabilize
	box_settings.mAngularDamping = 0.05f; // Slight rotational damping
	box_settings.mAllowedDOFs = EAllowedDOFs::All; // Allow all degrees of freedom
	box_settings.mMotionQuality = EMotionQuality::LinearCast; // Enable continuous collision detection
	box_settings.mCollisionGroup.SetGroupFilter(0); // Enable collision with ALL groups

	BodyID box_id = body_interface->CreateAndAddBody(box_settings, EActivation::Activate);

	std::cout << "Created box at: ";
	PhysicsUtils::PrintVec3(PhysicsUtils::ToJoltVec3(pos));
	std::cout << " with scale: (" << scale.x << ", " << scale.y << ", " << scale.z << ") | Layer: MOVING | CCD: Enabled" << std::endl;

	return box_id;
}

BodyID PhysicsEngine::CreateStaticBox(glm::vec3 scale, glm::vec3 pos, glm::quat rot, float restitution)
{
	//glm::vec3 half_extents = scale * 0.5f; // Jolt's BoxShape uses half extents
	glm::vec3 half_extents = scale * 25.f; //25 is the correct offset >..>

	BodyCreationSettings box_settings(new BoxShape(PhysicsUtils::ToJoltVec3(half_extents)),
		PhysicsUtils::ToJoltVec3(pos),
		PhysicsUtils::ToJoltQuat(rot),
		EMotionType::Static,
		Layers::NON_MOVING);

	box_settings.mRestitution = restitution;
	box_settings.mGravityFactor = 1.0f; // Ensure gravity is applied
	box_settings.mLinearDamping = 0.05f; // Slight damping to stabilize
	box_settings.mAngularDamping = 0.05f; // Slight rotational damping
	box_settings.mAllowedDOFs = EAllowedDOFs::All; // Allow all degrees of freedom
	box_settings.mMotionQuality = EMotionQuality::LinearCast; // Enable continuous collision detection
	box_settings.mCollisionGroup.SetGroupFilter(0); // Enable collision with all groups

	BodyID box_id = body_interface->CreateAndAddBody(box_settings, EActivation::DontActivate);

	std::cout << "Created box at: ";
	PhysicsUtils::PrintVec3(PhysicsUtils::ToJoltVec3(pos));
	std::cout << " with scale: (" << scale.x << ", " << scale.y << ", " << scale.z << ") | Layer: NON_MOVING | CCD: Enabled" << std::endl;

	return box_id;
}




