#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Light.h"
#include "From-GDGRAP2/EventBroadcaster.h"

namespace Vulkan
{
    class Buffer;
    class DeviceMemory;
}

class LightManager : public Observer
{
public:
    using BufferPtr = std::unique_ptr<Vulkan::Buffer>;
    using DeviceMemoryPtr = std::unique_ptr<Vulkan::DeviceMemory>;
    using LightPtr = Light*;
    using LightMap = std::unordered_map<uint32_t, LightPtr>;

    static LightManager* getInstance();
    static void initialize(uint32_t framesInFlight);
    static void destroy();

    void AddLight(LightPtr light);
    void RemoveLight(LightPtr light);

    bool UpdateLight(const uint32_t& lightId, const Light& data);

    size_t GetLightCount() const { return m_lightMap.size(); }

    BufferPtr& GetLightBuffer(uint32_t frameIndex);
    void SyncBuffersToGPU();

private:
    LightManager();
    ~LightManager() = default;
    LightManager(LightManager const&) = delete;
    LightManager& operator=(LightManager const&) = delete;

    static LightManager* sharedInstance;
    static const int MAX_LIGHTS = 256;

    std::vector<BufferPtr> m_lightBuffers;      // One per frame
    std::vector<DeviceMemoryPtr> m_lightMemory;

    LightMap m_lightMap;

    uint32_t m_framesInFlight = 2;
    bool m_buffersDirty = false;

    // Inherited via Observer
    void onTriggeredEvent(String eventName, std::shared_ptr<Parameters> parameters) override;
};