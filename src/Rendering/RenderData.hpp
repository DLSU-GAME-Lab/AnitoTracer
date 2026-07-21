#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Diligent {

    class RenderData {
        public:
            glm::mat4 ViewMatrix{ 1.0f };
            glm::mat4 ProjectionMatrix{ 1.0f };
            glm::mat4 ViewProjectionMatrix{ 1.0f };
            glm::vec3 CameraPosition{ 0.0f };
            bool IsValid = false;

            RenderData() = default;
    };

}