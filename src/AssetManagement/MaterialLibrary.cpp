#include "MaterialLibrary.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/BufferUtil.hpp"
#include "Material.hpp"
#include "RayTracer.hpp"
#include "From-GDGRAP2/Debug.h"

MaterialLibrary* MaterialLibrary::sharedInstance = nullptr;

MaterialLibrary* MaterialLibrary::getInstance() 
{
	return sharedInstance;
}

void MaterialLibrary::initialize(uint32_t framesInFlight) 
{
	sharedInstance = new MaterialLibrary();
	sharedInstance->m_framesInFlight = framesInFlight;
}

void MaterialLibrary::destroy()  
{
	delete sharedInstance; 
}

MaterialLibrary::MaterialPtr MaterialLibrary::CreateMaterial(const std::string& materialName, MaterialLibrary::MaterialType type, const glm::vec3& diffuseColor, const uint32_t& diffuseTextureId, float fuzziness, float refractiveIndex)
{
	MaterialPtr material = std::make_shared<Assets::Material>(glm::vec4(diffuseColor, 1), diffuseTextureId, fuzziness, refractiveIndex, type);
	this->RegisterMaterial(materialName, material);
	return material;
}

MaterialLibrary::MaterialPtr MaterialLibrary::CreateLambertianMaterial(const std::string& materialName, const glm::vec3& diffuseColor, const uint32_t& diffuseTextureId)
{
	MaterialPtr material = std::make_shared<Assets::Material>(glm::vec4(diffuseColor, 1), diffuseTextureId, 0.0f, 0.0f, MaterialType::Lambertian);
	this->RegisterMaterial(materialName, material);
	return material;
}

MaterialLibrary::MaterialPtr MaterialLibrary::CreateMetallicMaterial(const std::string& materialName, const glm::vec3& diffuseColor, float fuzziness, const uint32_t& diffuseTextureId)
{
	MaterialPtr material = std::make_shared<Assets::Material>(glm::vec4(diffuseColor, 1), diffuseTextureId, fuzziness, 0.0f, MaterialType::Metallic);
	this->RegisterMaterial(materialName, material);
	return material;
}

MaterialLibrary::MaterialPtr MaterialLibrary::CreateDielectricMaterial(const std::string& materialName, float refractiveIndex, const uint32_t& diffuseTextureId)
{
	MaterialPtr material = std::make_shared<Assets::Material>(glm::vec4(0.7f, 0.7f, 1.0f, 1.0f), diffuseTextureId, 0.0f, refractiveIndex, MaterialType::Dielectric);
	this->RegisterMaterial(materialName, material);
	return material;
}

MaterialLibrary::MaterialPtr MaterialLibrary::CreateIsotropicMaterial(const std::string& materialName, const glm::vec3& diffuseColor, const uint32_t& diffuseTextureId)
{
	MaterialPtr material = std::make_shared<Assets::Material>(glm::vec4(diffuseColor, 1), diffuseTextureId, 0.0f, 0.0f, MaterialType::Isotropic);
	this->RegisterMaterial(materialName, material);
	return material;
}

MaterialLibrary::MaterialPtr MaterialLibrary::CreateDiffuseLightMaterial(const std::string& materialName, const glm::vec3& diffuseColor, const uint32_t& diffuseTextureId)
{
	MaterialPtr material = std::make_shared<Assets::Material>(glm::vec4(diffuseColor, 1), diffuseTextureId, 0.0f, 0.0f, MaterialType::DiffuseLight);
	this->RegisterMaterial(materialName, material);
	return material;
}

MaterialLibrary::MaterialLibrary() 
{
    m_materialBuffers.resize(m_framesInFlight);
    m_materialMemory.resize(m_framesInFlight);

	//Current Max; Add more as needed/create more dynamically if needed
	std::vector<Assets::Material::GPUData> emptyMaterials(MAX_MATERIALS); 

    // Create one buffer per frame
    for (uint32_t i = 0; i < m_framesInFlight; ++i)
    {
        std::string bufferName = "MaterialBuffer_Frame" + std::to_string(i);
        
        Vulkan::BufferUtil::CreateDeviceBuffer<Assets::Material::GPUData>(
            RayTracer::getInstance()->CommandPool(),
            bufferName.c_str(),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			emptyMaterials,
            m_materialBuffers[i],
            m_materialMemory[i]
        );
    }

	this->CreateLambertianMaterial("Default_White", glm::vec3(0.73f, 0.73f, 0.73f));
	this->CreateLambertianMaterial("BaseMaterial", glm::vec3(glm::vec3(0.f, 0.f, 0.f )));
	this->CreateMetallicMaterial("Mirror", glm::vec3(0.21f, 0.43f, 0.71f), 0.0f);

	this->m_buffersDirty = true;
	SyncBuffersToGPU();
}

void MaterialLibrary::DeleteMaterial(const std::string& materialName)
{
	auto it = m_materialMap.find(materialName);
	if (it != m_materialMap.end())
	{
		Debug::Log("Deleted material: " + materialName);
		m_materialMap.erase(it);
		this->m_buffersDirty = true;
	}
	else
	{
		Debug::Log("Material: " + materialName + " not found");
	}
}

MaterialLibrary::MaterialPtr MaterialLibrary::GetMaterial(const std::string& materialName)
{
	auto it = m_materialMap.find(materialName);

	if (it != m_materialMap.end())
	{
		return it->second;
	}

	Debug::Log("Material: " + materialName + " not found");
	return nullptr;
}

bool MaterialLibrary::UpdateMaterial(const std::string& materialName, const Assets::Material& data)
{
	auto it = m_materialMap.find(materialName);

	if (it == m_materialMap.end())
	{
		Debug::Log("Material: " + materialName + " not found. Cannot update.");
		return false;
	}

	MaterialPtr material = it->second;
	*material = data;

	Debug::Log("Updated material: " + materialName);
	this->m_buffersDirty = true;
	return true;
}

MaterialLibrary::BufferPtr& MaterialLibrary::GetMaterialBuffer(uint32_t frameIndex)
{
	return m_materialBuffers[frameIndex];
}

void MaterialLibrary::SyncBuffersToGPU()
{
	if(this->m_buffersDirty)
	{
		std::vector<Assets::Material::GPUData> materialData;
		materialData.reserve(m_materialMap.size());

		uint32_t currentIndex = 0;
		for (auto& [name, material] : m_materialMap)
		{
			// Assign new index during sync
			material->SetIndex(currentIndex);
			materialData.push_back(material->GetGPUData());
			currentIndex++;
		}

		// Ensure we don't exceed MAX_MATERIALS
		if (materialData.size() > MAX_MATERIALS)
		{
			Debug::Log("Warning: Material count exceeds MAX_MATERIALS. Truncating.");
			materialData.resize(MAX_MATERIALS);
		}

		// Pad with empty materials if needed
		while (materialData.size() < MAX_MATERIALS)
		{
			materialData.push_back(Assets::Material::GPUData{});
		}

		// Update all frame buffers
		for (uint32_t i = 0; i < m_framesInFlight; ++i)
		{
			Vulkan::BufferUtil::UpdateDeviceBuffer<Assets::Material::GPUData>(
				RayTracer::getInstance()->CommandPool(),
				materialData,
				m_materialBuffers[i]
			);
		}

		Debug::Log("Synced " + std::to_string(currentIndex) + " materials to GPU");
		this->m_buffersDirty = false;
	}
}

void MaterialLibrary::RegisterMaterial(std::string materialName, MaterialPtr material)
{
	// Check if material already exists
	auto it = m_materialMap.find(materialName);
	if (it != m_materialMap.end())
	{
		Debug::Log("Warning: Material '" + materialName + "' already exists. Overwriting.");
	}

	m_materialMap[materialName] = material; 

	Debug::Log("Registered material: " + materialName);
}