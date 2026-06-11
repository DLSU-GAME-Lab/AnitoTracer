#include "PhysicsBody.hpp"
#include "PhysicsWorld.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Jolt includes
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <iostream>

namespace Anito::Physics {

	PhysicsBody::PhysicsBody(
		uint32_t bodyId,
		const glm::vec3& position,
		const glm::quat& rotation,
		const PhysicsBodySettings& settings,
		PhysicsWorld* physicsWorld,
		JPH::BodyID joltBodyId
	) : mBodyId(bodyId), mJoltBodyId(joltBodyId), mPhysicsWorld(physicsWorld), 
		mSettings(settings), mLastPosition(position), mLastRotation(rotation) {
		// The actual Jolt body is managed by PhysicsWorld
		// This wrapper provides convenient access and GLM conversions
	}

	PhysicsBody::~PhysicsBody() {
		// The Jolt body is cleaned up by PhysicsWorld
		// We just release our reference here
	}

	glm::vec3 PhysicsBody::GetPosition() const {
		if (!mPhysicsWorld) {
			return mLastPosition;
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return mLastPosition;
		}

		const JPH::Vec3& joltPos = joltBody->GetPosition();
		return glm::vec3(joltPos.GetX(), joltPos.GetY(), joltPos.GetZ());
	}

	void PhysicsBody::SetPosition(const glm::vec3& position) {
		mLastPosition = position;

		if (!mPhysicsWorld) {
			return;
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return;
		}

		JPH::BodyInterface& bodyInterface = joltBody->GetPhysicsSystem()->GetBodyInterface();
		bodyInterface.SetPosition(mJoltBodyId, JPH::Vec3(position.x, position.y, position.z), JPH::EActivation::Activate);
	}

	glm::quat PhysicsBody::GetRotation() const {
		if (!mPhysicsWorld) {
			return mLastRotation;
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return mLastRotation;
		}

		const JPH::Quat& joltQuat = joltBody->GetRotation();
		return glm::quat(joltQuat.GetW(), joltQuat.GetX(), joltQuat.GetY(), joltQuat.GetZ());
	}

	void PhysicsBody::SetRotation(const glm::quat& rotation) {
		mLastRotation = rotation;

		if (!mPhysicsWorld) {
			return;
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return;
		}

		JPH::BodyInterface& bodyInterface = joltBody->GetPhysicsSystem()->GetBodyInterface();
		bodyInterface.SetRotation(mJoltBodyId, JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w), JPH::EActivation::Activate);
	}

	glm::mat4 PhysicsBody::GetTransform() const {
		// Build transformation matrix from position and rotation
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), mLastPosition);
		transform *= glm::mat4_cast(mLastRotation);
		return transform;
	}

	glm::vec3 PhysicsBody::GetLinearVelocity() const {
		if (!mPhysicsWorld) {
			return glm::vec3(0.0f);
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return glm::vec3(0.0f);
		}

		const JPH::Vec3& velocity = joltBody->GetLinearVelocity();
		return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
	}

	void PhysicsBody::SetLinearVelocity(const glm::vec3& velocity) {
		if (!mPhysicsWorld) {
			return;
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return;
		}

		JPH::BodyInterface& bodyInterface = joltBody->GetPhysicsSystem()->GetBodyInterface();
		bodyInterface.SetLinearVelocity(mJoltBodyId, JPH::Vec3(velocity.x, velocity.y, velocity.z));
	}

	glm::vec3 PhysicsBody::GetAngularVelocity() const {
		if (!mPhysicsWorld) {
			return glm::vec3(0.0f);
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return glm::vec3(0.0f);
		}

		const JPH::Vec3& angVel = joltBody->GetAngularVelocity();
		return glm::vec3(angVel.GetX(), angVel.GetY(), angVel.GetZ());
	}

	void PhysicsBody::SetAngularVelocity(const glm::vec3& angularVelocity) {
		if (!mPhysicsWorld) {
			return;
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return;
		}

		JPH::BodyInterface& bodyInterface = joltBody->GetPhysicsSystem()->GetBodyInterface();
		bodyInterface.SetAngularVelocity(mJoltBodyId, JPH::Vec3(angularVelocity.x, angularVelocity.y, angularVelocity.z));
	}

	void PhysicsBody::ApplyForce(const glm::vec3& force) {
		if (!mPhysicsWorld) {
			return;
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return;
		}

		JPH::BodyInterface& bodyInterface = joltBody->GetPhysicsSystem()->GetBodyInterface();
		bodyInterface.AddForce(mJoltBodyId, JPH::Vec3(force.x, force.y, force.z));
	}

	void PhysicsBody::ApplyForceAtPoint(const glm::vec3& force, const glm::vec3& position) {
		// TODO: Apply force at specific point via BodyInterface
		// Need to convert to local coordinates and apply as torque + linear force
	}

	void PhysicsBody::ApplyImpulse(const glm::vec3& impulse) {
		if (!mPhysicsWorld) {
			return;
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return;
		}

		JPH::BodyInterface& bodyInterface = joltBody->GetPhysicsSystem()->GetBodyInterface();
		bodyInterface.AddImpulse(mJoltBodyId, JPH::Vec3(impulse.x, impulse.y, impulse.z));
	}

	void PhysicsBody::ApplyTorque(const glm::vec3& torque) {
		// TODO: Apply torque to Jolt body via BodyInterface
	}

	void PhysicsBody::ApplyAngularImpulse(const glm::vec3& angularImpulse) {
		// TODO: Apply angular impulse to Jolt body via BodyInterface
	}

	void PhysicsBody::ClearForces() {
		// TODO: Clear accumulated forces from Jolt body
	}

	float PhysicsBody::GetMass() const {
		return mSettings.mass;
	}

	void PhysicsBody::SetMass(float mass) {
		if (mass <= 0.0f && mSettings.type != BodyType::STATIC) {
			return; // Cannot set invalid mass
		}
		mSettings.mass = mass;
		// TODO: Update Jolt body mass
	}

	float PhysicsBody::GetInverseMass() const {
		if (mSettings.mass <= 0.0f) {
			return 0.0f; // Static bodies have infinite mass
		}
		return 1.0f / mSettings.mass;
	}

	bool PhysicsBody::IsActive() const {
		if (!mPhysicsWorld) {
			return true;
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return true;
		}

		return joltBody->IsActive();
	}

	void PhysicsBody::SetActive(bool active) {
		if (!mPhysicsWorld) {
			return;
		}

		JPH::Body* joltBody = mPhysicsWorld->GetJoltBody(mJoltBodyId);
		if (!joltBody) {
			return;
		}

		JPH::BodyInterface& bodyInterface = joltBody->GetPhysicsSystem()->GetBodyInterface();
		if (active) {
			bodyInterface.ActivateBody(mJoltBodyId);
		} else {
			bodyInterface.DeactivateBody(mJoltBodyId);
		}
	}

	void PhysicsBody::SetFriction(float friction) {
		mSettings.material.friction = glm::max(0.0f, friction);
		// TODO: Update Jolt body material
	}

	void PhysicsBody::SetRestitution(float restitution) {
		mSettings.material.restitution = glm::clamp(restitution, 0.0f, 1.0f);
		// TODO: Update Jolt body material
	}

	void PhysicsBody::SetLinearDamping(float damping) {
		mSettings.material.linearDamping = glm::max(0.0f, damping);
		// TODO: Update Jolt body linear damping
	}

	void PhysicsBody::SetAngularDamping(float damping) {
		mSettings.material.angularDamping = glm::max(0.0f, damping);
		// TODO: Update Jolt body angular damping
	}

	void PhysicsBody::SetObjectLayer(ObjectLayer layer) {
		mSettings.layer = layer;
		// TODO: Update Jolt body collision layer
	}

	void PhysicsBody::GetAABB(glm::vec3& outMin, glm::vec3& outMax) const {
		// TODO: Retrieve AABB from Jolt body
		outMin = mLastPosition - glm::vec3(0.5f);
		outMax = mLastPosition + glm::vec3(0.5f);
	}

	void PhysicsBody::Reset(const glm::vec3& position, const glm::quat& rotation) {
		mLastPosition = position;
		mLastRotation = rotation;
		SetLinearVelocity(glm::vec3(0.0f));
		SetAngularVelocity(glm::vec3(0.0f));
		ClearForces();
		// TODO: Reset Jolt body state
	}

} // namespace Anito::Physics
