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
	physics_system.SetContactListener(contact_listener);

	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	
	std::cout << "Get Body Interface" << endl;
	body_interface = &physics_system.GetBodyInterface();

	std::cout << "Done Initializing Physics System" << endl;
}

void PhysicsEngine::Step(float deltaTime)
{
	physics_system.Update(deltaTime, cCollisionSteps, temp_allocator, job_system);

	if (!ball.IsInvalid())
	{
		// 3. Request a safe read lock from the physics system using the ID
		JPH::BodyLockRead lock(physics_system.GetBodyLockInterface(), ball);

		// 4. Check if the body still exists in the physics world
		if (lock.Succeeded())
		{
			const JPH::Body& body = lock.GetBody();
			JPH::RVec3 position = body.GetPosition();

			std::cout << "Current Ball Pos ";
			PhysicsUtils::PrintVec3(position);
			std::cout << std::endl;
		}
		else
		{
			std::cout << "Ball body was destroyed or is no longer valid." << std::endl;
		}
	}
}

void PhysicsEngine::CreateDefaultFloor(glm::vec3 pos)
{
	BoxShapeSettings floorSettings (Vec3(100.0f, 1.0f, 100.0f));

	ShapeSettings::ShapeResult shapeRes =  floorSettings.Create();
	ShapeRefC floorShape = shapeRes.Get();

	BodyCreationSettings floor_setting(
		floorShape,
		PhysicsUtils::ToJoltVec3(pos),
		Quat::sIdentity(), 
		EMotionType::Static, 
		Layers::NON_MOVING);

	floor_setting.mRestitution = 0.8f;//Bouncy test

	Body* floor = body_interface->CreateBody(floor_setting);
	body_interface->AddBody(floor->GetID(), EActivation::DontActivate);
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


