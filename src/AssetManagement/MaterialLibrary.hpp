#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace Assets
{
	class Material;
}

namespace Vulkan
{
	class Buffer;
	class DeviceMemory;
}

// TODO: Need Text Files for created materials.
class MaterialLibrary
{
public:
	using BufferPtr = std::unique_ptr<Vulkan::Buffer>;
	using DeviceMemoryPtr = std::unique_ptr<Vulkan::DeviceMemory>;
	using MaterialPtr = std::shared_ptr<Assets::Material>;
	using MaterialMap = std::unordered_map <std::string, MaterialPtr>;
	using MaterialType = Assets::Material::MaterialType;

	static MaterialLibrary* getInstance();
	static void initialize(uint32_t framesInFlight);
	static void destroy();

	MaterialPtr CreateMaterial(const std::string& materialName, MaterialType type, const glm::vec3& diffuseColor, const uint32_t& diffuseTextureId, float fuzziness, float refractiveIndex);
	MaterialPtr CreateLambertianMaterial(const std::string& materialName, const glm::vec3& diffuseColor, const uint32_t& diffuseTextureId = 0);
	MaterialPtr CreateMetallicMaterial(const std::string& materialName, const glm::vec3& diffuseColor, float fuzziness, const uint32_t& diffuseTextureId = 0);
	MaterialPtr CreateDielectricMaterial(const std::string& materialName, float refractiveIndex, const uint32_t& diffuseTextureId = 0);
	MaterialPtr CreateIsotropicMaterial(const std::string& materialName, const glm::vec3& diffuseColor, const uint32_t& diffuseTextureId = 0);
	MaterialPtr CreateDiffuseLightMaterial(const std::string& materialName, const glm::vec3& diffuseColor, const uint32_t& diffuseTextureId = 0);

	void DeleteMaterial(const std::string& materialName);
	MaterialPtr GetMaterial(const std::string& materialName);

	bool UpdateMaterial(const std::string& materialName, const Assets::Material& data);
	size_t GetMaterialCount() const { return m_materialMap.size(); }

	BufferPtr& GetMaterialBuffer(uint32_t frameIndex);
	
	void SyncBuffersToGPU();
private:
	MaterialLibrary();
	~MaterialLibrary() = default;
	MaterialLibrary(MaterialLibrary const&) = delete;
	MaterialLibrary&operator= (MaterialLibrary const&) = delete;

	void RegisterMaterial(std::string materialName, MaterialPtr material);

	static MaterialLibrary* sharedInstance;
	static const int MAX_MATERIALS = 1024;

	std::vector<BufferPtr> m_materialBuffers;      // One per frame
	std::vector<DeviceMemoryPtr> m_materialMemory;
	MaterialMap m_materialMap;

	uint32_t m_framesInFlight = 2;

	bool m_buffersDirty = false;
};