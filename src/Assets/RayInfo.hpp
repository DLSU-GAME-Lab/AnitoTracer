#pragma once

namespace Assets
{
	struct alignas(16) RayInfo final {
		uint32_t RayCount;
		uint32_t RayOffset;
	};
}