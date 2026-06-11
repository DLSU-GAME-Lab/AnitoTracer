#include "PhysicsComponent.hpp"
#include "PhysicsBody.hpp"
#include "PhysicsWorld.hpp"
#include "PhysicsEngine.hpp"
#include "From-GDGRAP2/GameObject.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace Anito::Physics {

	PhysicsComponent::PhysicsComponent(GameObject* owner, const PhysicsBodySettings& settings)
		: mOwner(owner), mSettings(settings), mAutoSyncTransform(true), mEnabled(true), mInitialized(false) {

		if (!mOwner) {
			throw std::invalid_argument("PhysicsComponent requires a valid GameObject owner");
		}
	}

	PhysicsComponent::~PhysicsComponent() {
		Cleanup();
	}

	void PhysicsComponent::Initialize() {
		if (mInitialized || !mOwner || !mEnabled) {
			return;
		}

		// Get the position and rotation from the GameObject
		glm::vec3 position = mOwner->getWorldPosition();
		glm::quat rotation = mOwner->getWorldRotationQuat();

		// Create the physics body through PhysicsEngine's default world
		try {
			auto& engine = PhysicsEngine::Get();
			if (!engine.IsInitialized()) {
				// Physics engine not available, component remains uninitialized
				return;
			}

			auto world = engine.GetDefaultWorld();
			if (!world) {
				// No default world available
				return;
			}

			// Create the physics body in the world
			mPhysicsBody = world->CreateBody(position, rotation, mSettings);

			if (!mPhysicsBody) {
				// Body creation failed
				std::cerr << "[PhysicsComponent] Failed to create physics body" << std::endl;
				return;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[PhysicsComponent::Initialize] Exception: " << e.what() << std::endl;
			return;
		}

		mInitialized = true;
	}

	void PhysicsComponent::Update(float deltaTime) {
		if (!mInitialized || !mOwner || !mPhysicsBody || !mEnabled) {
			return;
		}

		// Sync physics to transform if auto-sync is enabled
		if (mAutoSyncTransform && mPhysicsBody) {
			SyncPhysicsToTransform();
		}
	}

	void PhysicsComponent::Cleanup() {
		if (mPhysicsBody) {
			try {
				auto& engine = PhysicsEngine::Get();
				if (engine.IsInitialized()) {
					auto world = engine.GetDefaultWorld();
					if (world) {
						world->DestroyBody(mPhysicsBody);
					}
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[PhysicsComponent::Cleanup] Exception: " << e.what() << std::endl;
			}
			mPhysicsBody.reset();
		}
		mInitialized = false;
	}

	void PhysicsComponent::SyncPhysicsToTransform() {
		if (!mPhysicsBody || !mOwner) {
			return;
		}

		// Get the position and rotation from the physics body
		glm::vec3 physicsPos = mPhysicsBody->GetPosition();
		glm::quat physicsRot = mPhysicsBody->GetRotation();

		// Update the GameObject's position and rotation
		mOwner->setLocalPosition(physicsPos);
		mOwner->setLocalRotationQuat(physicsRot);
	}

	void PhysicsComponent::SyncTransformToPhysics() {
		if (!mPhysicsBody || !mOwner) {
			return;
		}

		// Get the position and rotation from the GameObject
		glm::vec3 position = mOwner->getWorldPosition();
		glm::quat rotation = mOwner->getWorldRotationQuat();

		// Update the physics body
		mPhysicsBody->SetPosition(position);
		mPhysicsBody->SetRotation(rotation);
	}

	glm::vec3 PhysicsComponent::GetPosition() const {
		if (mPhysicsBody) {
			return mPhysicsBody->GetPosition();
		}
		return mOwner ? mOwner->getWorldPosition() : glm::vec3(0.0f);
	}

	glm::vec3 PhysicsComponent::GetVelocity() const {
		if (mPhysicsBody) {
			return mPhysicsBody->GetLinearVelocity();
		}
		return glm::vec3(0.0f);
	}

	glm::vec3 PhysicsComponent::GetAngularVelocity() const {
		if (mPhysicsBody) {
			return mPhysicsBody->GetAngularVelocity();
		}
		return glm::vec3(0.0f);
	}

	void PhysicsComponent::SetVelocity(const glm::vec3& velocity) {
		if (mPhysicsBody) {
			mPhysicsBody->SetLinearVelocity(velocity);
		}
	}

	void PhysicsComponent::ApplyForce(const glm::vec3& force) {
		if (mPhysicsBody) {
			mPhysicsBody->ApplyForce(force);
		}
	}

	void PhysicsComponent::ApplyImpulse(const glm::vec3& impulse) {
		if (mPhysicsBody) {
			mPhysicsBody->ApplyImpulse(impulse);
		}
	}

} // namespace Anito::Physics
