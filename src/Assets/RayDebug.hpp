#include "Utilities/Glm.hpp"

namespace Assets
{
	struct alignas(16) RayDebug final
	{
        glm::vec3 Origin;
        glm::vec3 Direction;
        glm::vec3 HitPosition;
	};
}