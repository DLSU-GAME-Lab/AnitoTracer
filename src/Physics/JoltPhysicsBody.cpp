#include "JoltPhysicsBody.hpp"
#include "PhysicsEngine.hpp"

#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Geometry/AABox.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

JoltPhysicsBody::JoltPhysicsBody(JPH::BodyID bodyID, JPH::BodyInterface* bodyInterface, const JPH::BodyLockInterface* bodyLockInterface, float mass)
	: mBodyID(bodyID), mBodyInterface(bodyInterface), mBodyLockInterface(bodyLockInterface), mMass(mass) {
}

//Converted from Diligent (Left) to Jolt (Right)
JPH::RVec3 JoltPhysicsBody::ToJoltVec3(const glm::vec3& value) {
	//return JPH::RVec3(value.x, value.y, value.z);
	// Flip Z-axis to convert from Left-Handed to Right-Handed coordinate system
	return JPH::RVec3(value.x, value.y, -value.z);
}

glm::vec3 JoltPhysicsBody::ToGlmVec3(const JPH::Vec3& value) {
	//return glm::vec3(value.GetX(), value.GetY(), value.GetZ());
	// Flip Z-axis to convert from Right-Handed to Left-Handed coordinate system
	return glm::vec3(value.GetX(), value.GetY(), -value.GetZ());
}

JPH::Quat JoltPhysicsBody::ToJoltQuat(const glm::quat& value) {
	//return JPH::Quat(value.x, value.y, value.z, value.w);
	// Convert quaternion from Left-Handed to Right-Handed coordinate system
	// Inverting the Z-axis requires negating the X and Y components of the quaternion
	return JPH::Quat(-value.x, -value.y, value.z, value.w);
}

glm::quat JoltPhysicsBody::ToGlmQuat(const JPH::Quat& value) {
	//return glm::quat(value.GetW(), value.GetX(), value.GetY(), value.GetZ());
	// Convert quaternion from Right-Handed to Left-Handed coordinate system
	// Inverting the Z-axis requires negating the X and Y components of the quaternion
	// Note: glm::quat constructor takes arguments as (w, x, y, z)
	return glm::quat(value.GetW(), -value.GetX(), -value.GetY(), value.GetZ());
}

void JoltPhysicsBody::SetPosition(const glm::vec3& position) {
	if (!mBodyInterface) return;

	if (mMass <= 0.0f) {
		WakeSurroundingBodies();
	}

	mBodyInterface->SetPosition(mBodyID, ToJoltVec3(position), JPH::EActivation::Activate);

	if (mMass <= 0.0f) {
		WakeSurroundingBodies();
	}
}

glm::vec3 JoltPhysicsBody::GetPosition() const {
	if (!mBodyInterface) {
		return glm::vec3(0.0f);
	}
	return ToGlmVec3(mBodyInterface->GetPosition(mBodyID));
}

void JoltPhysicsBody::SetRotation(const glm::quat& rotation) {
	if (!mBodyInterface) return;

	if (mMass <= 0.0f) {
		WakeSurroundingBodies();
	}

	mBodyInterface->SetRotation(mBodyID, ToJoltQuat(rotation), JPH::EActivation::Activate);

	if (mMass <= 0.0f) {
		WakeSurroundingBodies();
	}
}

glm::quat JoltPhysicsBody::GetRotation() const {
	if (!mBodyInterface) {
		return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	}
	return ToGlmQuat(mBodyInterface->GetRotation(mBodyID));
}

void JoltPhysicsBody::SetPositionAndRotation(const glm::vec3& position, const glm::quat& rotation) {
	if (!mBodyInterface) return;

	if (mMass <= 0.0f) {
		WakeSurroundingBodies();
	}

	mBodyInterface->SetPositionAndRotation(mBodyID, ToJoltVec3(position), ToJoltQuat(rotation), JPH::EActivation::Activate);

	if (mMass <= 0.0f) {
		WakeSurroundingBodies();
	}
}

void JoltPhysicsBody::Activate() {
	if (mBodyInterface && mMass > 0.0f) {
		mBodyInterface->ActivateBody(mBodyID);
	}
}

void JoltPhysicsBody::SetMass(float mass) {
	if (mass <= 0.0f) {
		mMass = 0.0f;
		return;
	}

	mMass = mass;
	ApplyMassProperties();
}

void JoltPhysicsBody::SetRotationLocks(bool lockX, bool lockY, bool lockZ) {
	mAllowedDOFs = JPH::EAllowedDOFs::TranslationX |
		JPH::EAllowedDOFs::TranslationY |
		JPH::EAllowedDOFs::TranslationZ;

	if (!lockX) mAllowedDOFs |= JPH::EAllowedDOFs::RotationX;
	if (!lockY) mAllowedDOFs |= JPH::EAllowedDOFs::RotationY;
	if (!lockZ) mAllowedDOFs |= JPH::EAllowedDOFs::RotationZ;

	ApplyMassProperties();
}

void JoltPhysicsBody::ApplyMassProperties() {
	if (!mBodyInterface || !mBodyLockInterface || mMass <= 0.0f) {
		return;
	}

	JPH::BodyLockWrite lock(*mBodyLockInterface, mBodyID);
	if (lock.Succeeded()) {
		JPH::Body& body = lock.GetBody();
		JPH::MotionProperties* motionProperties = body.GetMotionProperties();
		if (motionProperties) {
			JPH::MassProperties massProperties = body.GetShape()->GetMassProperties();
			massProperties.ScaleToMass(mMass);
			motionProperties->SetMassProperties(mAllowedDOFs, massProperties);
		}
	}
}

float JoltPhysicsBody::GetMass() const {
	return mMass;
}

void JoltPhysicsBody::SetVelocity(const glm::vec3& velocity) {
	if (mBodyInterface) {
		mBodyInterface->SetLinearVelocity(mBodyID, ToJoltVec3(velocity));
	}
}

glm::vec3 JoltPhysicsBody::GetVelocity() const {
	if (!mBodyInterface) {
		return glm::vec3(0.0f);
	}
	return ToGlmVec3(mBodyInterface->GetLinearVelocity(mBodyID));
}

void JoltPhysicsBody::SetAngularVelocity(const glm::vec3& angularVelocity) {
	if (mBodyInterface) {
		mBodyInterface->SetAngularVelocity(mBodyID, ToJoltVec3(angularVelocity));
	}
}

glm::vec3 JoltPhysicsBody::GetAngularVelocity() const {
	if (!mBodyInterface) {
		return glm::vec3(0.0f);
	}
	return ToGlmVec3(mBodyInterface->GetAngularVelocity(mBodyID));
}

void JoltPhysicsBody::ApplyForce(const glm::vec3& force) {
	if (mBodyInterface) {
		mBodyInterface->AddForce(mBodyID, ToJoltVec3(force));
	}
}

void JoltPhysicsBody::ApplyImpulse(const glm::vec3& impulse) {
	if (mBodyInterface) {
		mBodyInterface->AddImpulse(mBodyID, ToJoltVec3(impulse));
	}
}

void JoltPhysicsBody::WakeSurroundingBodies() {
	if (!mBodyInterface) return;
	PhysicsEngine::GetInstance().Get().WakeBodiesAroundBody(this);
}