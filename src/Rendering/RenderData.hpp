#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Models/ModelStructs.hpp"
#include "Models/MaterialOverrides.hpp"
#include "Pipelines/Data/PipelineDefs.hpp"
#include "ParticleRenderData.hpp"

#include "Graphics/GraphicsEngine/interface/TopLevelAS.h" 
#include "Graphics/GraphicsEngine/interface/Buffer.h"    

namespace Diligent {

    // The layers over a surface's base material, as the shaders get them: what the material has, replaced by the
    // object's overrides, and left out where the project's Materials settings switch them off.
    struct MaterialLayers {
        float Clearcoat = 0.0f;             // 0: no coat
        float ClearcoatRoughness = 0.0f;
        glm::vec3 SheenColor{ 0.0f };       // Black: no sheen
        float SheenRoughness = 0.5f;
        float Transmission = 0.0f;          // 0: nothing goes through
    };

    struct ModelRenderInstance {
        Model* ModelData = nullptr;
        glm::mat4 WorldTransform{ 1.0f };
        uint64_t OwnerID = 0;

        // The object's overrides, from its ModelComponent: they replace both the material factor and texture of every submesh
        MaterialOverrides Overrides;
        // The same for each submesh, when some submesh has overrides of its own (its node in the hierarchy, see SubMeshComponent):
        // the object's overrides with the submesh's laid over them. Empty when none does, and every submesh then uses Overrides.
        std::vector<MaterialOverrides> SubMeshOverrides;
        // True when the object or a submesh is turned into glass by an alpha override, or lets light through by a transmission
        // override: its geometry is then handed to the rays to judge instead of being solid
        bool AnyOverrideAlpha = false;
        bool AnyOverrideTransmission = false;

        // The overrides that apply to one submesh
        const MaterialOverrides& OverridesFor(uint32_t submesh) const {
            return submesh < SubMeshOverrides.size() ? SubMeshOverrides[submesh] : Overrides;
        }

        // What the project's Materials settings allow (HierarchyManager::GatherRenderModels). Switched off, the layer
        // is left out whatever the material or the override says.
        bool ClearcoatEnabled = true;
        bool SheenEnabled = true;
        bool TransmissionEnabled = true;
        float SpecularAAStrength = 1.0f;    // 0: specular anti-aliasing off

        // The layers of one of this object's materials (the one submesh draws with), with overrides and the project's switches applied
        MaterialLayers ResolveLayers(const PBRMaterial& material, uint32_t submesh) const {
            const MaterialOverrides& over = OverridesFor(submesh);
            MaterialLayers layers;
            if (ClearcoatEnabled) {
                layers.Clearcoat = over.OverrideClearcoat ? over.Clearcoat : material.Clearcoat;
                layers.ClearcoatRoughness = over.OverrideClearcoat ? over.ClearcoatRoughness : material.ClearcoatRoughness;
            }
            if (SheenEnabled) {
                layers.SheenColor = over.OverrideSheen ? over.SheenColor : glm::vec3(material.SheenColor.r, material.SheenColor.g, material.SheenColor.b);
                layers.SheenRoughness = over.OverrideSheen ? over.SheenRoughness : material.SheenRoughness;
            }
            if (TransmissionEnabled) {
                layers.Transmission = over.OverrideTransmission ? over.Transmission : material.Transmission;
            }
            return layers;
        }

		//Opaque and transparent meshes are separated for proper rendering order
        std::vector<uint32_t> OpaqueSubmeshIndices;
        std::vector<uint32_t> TransparentSubmeshIndices;
        // Submeshes that are opaque but let light through (see ResolveLayers): drawn with the glass by the renderers that draw
        // glass (BasePipeline::DrawsGlass), and with the opaque ones by the rest. Never in the opaque list while they are glass.
        std::vector<uint32_t> TransmissiveSubmeshIndices;
    };

    class RenderData {
        public:
            glm::mat4 ViewMatrix{ 1.0f };
            glm::mat4 ProjectionMatrix{ 1.0f };
            glm::mat4 ViewProjectionMatrix{ 1.0f };
            glm::vec3 CameraPosition{ 0.0f };
            bool IsValid = false;

            // The sub-pixel shift TAA added to ProjectionMatrix this frame, in clip space units (0 without TAA).
            // Motion vectors are measured without it.
            glm::vec2 ProjectionJitter{ 0.0f };
            glm::mat4 GetUnjitteredProjection() const {
                return glm::translate(glm::mat4(1.0f), glm::vec3(-ProjectionJitter, 0.0f)) * ProjectionMatrix;
            }

            // Seeds the per-frame noise of ray-traced shadows, AO and fog. Set by RendererManager to a counter
            // that changes every frame only while TAA is accumulating; 0 keeps the noise still, since without
            // TAA a changing pattern would only flicker.
            uint32_t NoiseFrame = 0;

            std::vector<ModelRenderInstance> Models;
            LightConstants Lights;

            std::vector<ParticleBatch> Particles;

            // Debug Data
            std::vector<DebugLineVertex> DebugLines;

            //RT Data
            RefCntAutoPtr<ITopLevelAS> pTLAS;
            RefCntAutoPtr<IBuffer> pTLASScratchBuffer;

            RenderData() = default;
    };

}