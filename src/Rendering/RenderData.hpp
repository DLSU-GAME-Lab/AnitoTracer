#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Objects/Models/ModelStructs.hpp"
#include "Pipelines/PipelineDefs.hpp"

#include "Graphics/GraphicsEngine/interface/TopLevelAS.h" 
#include "Graphics/GraphicsEngine/interface/Buffer.h"    

namespace Diligent {

    struct ModelRenderInstance {
        Model* ModelData = nullptr;
        glm::mat4 WorldTransform{ 1.0f };
    };

    class RenderData {
        public:
            glm::mat4 ViewMatrix{ 1.0f };
            glm::mat4 ProjectionMatrix{ 1.0f };
            glm::mat4 ViewProjectionMatrix{ 1.0f };
            glm::vec3 CameraPosition{ 0.0f };
            bool IsValid = false;

            std::vector<ModelRenderInstance> Models;
            LightConstants Lights;

            //RT Data
            RefCntAutoPtr<ITopLevelAS> pTLAS;
            RefCntAutoPtr<IBuffer> pTLASScratchBuffer;

            RenderData() = default;
    };

}