#include "Utilities/Glm.hpp"

namespace Assets
{
	struct alignas(16) RayVertex final
	{
		glm::vec3 Position;
	};

	struct alignas(16) RayInfo final {
		uint32_t RayCount;
		uint32_t RayOffset;
	};
}