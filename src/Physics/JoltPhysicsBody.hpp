#pragma once

#include "IPhysicsEngine.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class JoltPhysicsBody : public IPhysicsBody {
public:
	JoltPhysicsBody(JPH::BodyID bodyID, JPH::BodyInterface* bodyInterface, const JPH::BodyLockInterface* bodyLockInterface, float mass = 0.0f);
	~JoltPhysicsBody() override = default;

	void SetPosition(const glm::vec3& position) override;
	glm::vec3 GetPosition() const override;

	void SetRotation(const glm::quat& rotation) override;
	glm::quat GetRotation() const override;

	void SetMass(float mass) override;
	float GetMass() const override;

	void SetVelocity(const glm::vec3& velocity) override;
	glm::vec3 GetVelocity() const override;

	void SetAngularVelocity(const glm::vec3& angularVelocity) override;
	glm::vec3 GetAngularVelocity() const override;

	void ApplyForce(const glm::vec3& force) override;
	void ApplyImpulse(const glm::vec3& impulse) override;

	JPH::BodyID GetBodyID() const { return mBodyID; }
private:
	JPH::BodyID mBodyID;
	JPH::BodyInterface* mBodyInterface;
	const JPH::BodyLockInterface* mBodyLockInterface;
	float mMass;

	static JPH::RVec3 ToJoltVec3(const glm::vec3& value);
	static glm::vec3 ToGlmVec3(const JPH::Vec3& value);
	static JPH::Quat ToJoltQuat(const glm::quat& value);
	static glm::quat ToGlmQuat(const JPH::Quat& value);
};