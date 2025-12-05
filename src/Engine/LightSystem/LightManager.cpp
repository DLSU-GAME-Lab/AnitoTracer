#include "LightManager.hpp"
#include <From-GDGRAP2/Debug.h>
#include <Vulkan/BufferUtil.hpp>
#include "RayTracer.hpp"

LightManager* LightManager::sharedInstance = nullptr;

LightManager* LightManager::getInstance()
{
	return sharedInstance;
}

void LightManager::initialize(uint32_t framesInFlight)
{
	sharedInstance = new LightManager();
	sharedInstance->m_framesInFlight = framesInFlight;
}

void LightManager::destroy()
{
	delete sharedInstance;
}

LightManager::LightManager()
{
	m_lightBuffers.resize(m_framesInFlight);
	m_lightMemory.resize(m_framesInFlight);

	//Current Max; Add more as needed/create more dynamically if needed
	std::vector<Assets::LightProperties> emptyLights(MAX_LIGHTS);

	// Create one buffer per frame
	for (uint32_t i = 0; i < m_framesInFlight; ++i)
	{
		std::string bufferName = "LightBuffer_Frame" + std::to_string(i);

		Vulkan::BufferUtil::CreateDeviceBuffer<Assets::LightProperties>(
			RayTracer::getInstance()->CommandPool(),
			bufferName.c_str(),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			emptyLights,
			m_lightBuffers[i],
			m_lightMemory[i]
		);
	}

	/* No Default Properties */
	//this->m_buffersDirty = true;
	//SyncBuffersToGPU();
}

void LightManager::AddLight(LightPtr light)
{
	auto it = m_lightMap.find(light->GetID());

	if (it != m_lightMap.end())
	{
		Debug::Log("Light with ID: " + std::to_string(light->GetID()) + " already exists. Overwritting.");
	}

	this->m_lightMap[light->GetID()] = light;
	this->m_buffersDirty = true;
}

void LightManager::RemoveLight(LightPtr light)
{
	auto it = m_lightMap.find(light->GetID());

	if (it == m_lightMap.end())
	{
		Debug::Log("Light with ID: " + std::to_string(light->GetID()) + " not found. Cannot remove.");
		return;
	}

	this->m_lightMap.erase(light->GetID());
	this->m_buffersDirty = true;
}

bool LightManager::UpdateLight(const uint32_t& lightId, const Light& data)
{
	auto it = m_lightMap.find(lightId);

	if (it == m_lightMap.end())
	{
		Debug::Log("Light with ID: " + std::to_string(lightId) + " not found. Cannot update.");
		return false;
	}

	LightPtr light = it->second;
	*light = data;
	Debug::Log("Updated light with ID: " + std::to_string(lightId));
	this->m_buffersDirty = true;
	return true;
}

LightManager::BufferPtr& LightManager::GetLightBuffer(uint32_t frameIndex)
{
	return m_lightBuffers[frameIndex];
}

void LightManager::SyncBuffersToGPU()
{
	if (this->m_buffersDirty)
	{
		std::vector<Assets::LightProperties> lightData;
		lightData.reserve(m_lightMap.size());

		for (const auto& [id, light] : m_lightMap)
		{
			lightData.push_back(light->Properties());
		}

		// Ensure we don't exceed MAX_LIGHTS
		if (lightData.size() > MAX_LIGHTS)
		{
			Debug::Log("Warning: Light count exceeds MAX_LIGHTS. Truncating.");
			lightData.resize(MAX_LIGHTS);
		}

		// Pad with empty materials if needed
		while (lightData.size() < MAX_LIGHTS)
		{
			lightData.push_back(Assets::LightProperties{});
		}

		// Update all frame buffers
		for (uint32_t i = 0; i < m_framesInFlight; ++i)
		{
			Vulkan::BufferUtil::UpdateDeviceBuffer<Assets::LightProperties>(
				RayTracer::getInstance()->CommandPool(),
				lightData,
				m_lightBuffers[i]
			);
		}

		Debug::Log("Synced " + std::to_string(m_lightMap.size()) + " lightProperties to GPU");
		this->m_buffersDirty = false;
	}
}

void LightManager::onTriggeredEvent(String eventName, std::shared_ptr<Parameters> parameters)
{
	if (eventName == EventNames::ON_LIGHT_ADDED)
	{
		Light* light = reinterpret_cast<Light*>(parameters->getIntData(EventNames::ON_LIGHT_ADDED, 0));
		this->AddLight(light);
	}
	else if (eventName == EventNames::ON_LIGHT_REMOVED)
	{
		Light* light = reinterpret_cast<Light*>(parameters->getIntData(EventNames::ON_LIGHT_REMOVED, 0));
		this->RemoveLight(light);
	}
}
