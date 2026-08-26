#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Interface for components with position/rotation independent of
// Transform (e.g. physics bodies).
class ITeleportable {
public:
	virtual ~ITeleportable() = default;
	virtual void Teleport(const glm::vec3& position, const glm::quat& rotation) = 0;
};