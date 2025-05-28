#include "RayScene.hpp"

#include <iostream>

#include "Model.hpp"
#include "Vulkan/BufferUtil.hpp"
#include "Utilities/Exception.hpp"
#include "Vulkan/SingleTimeCommands.hpp"

using namespace glm;
namespace Assets {

RayScene::RayScene(Vulkan::CommandPool& commandPool, std::vector<Model>&& models) :
	models_(std::move(models))
{
	// Concatenate all the models
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<glm::uvec2> offsets;

	Vertex vertex1{ vec3(-100.0f,-100.0f,0.0f), vec3(0,0,0), vec2(0,0), -1 };
	Vertex vertex2{ vec3(100.0f,100.0f,0.0f), vec3(0,0,0), vec2(0,0), -1 };

	// Remember the index, vertex offsets.
	const auto indexOffset = static_cast<uint32_t>(indices.size());
	const auto vertexOffset = static_cast<uint32_t>(vertices.size());

	offsets.emplace_back(indexOffset, vertexOffset);

	// Copy model data one after the other.
	//vertices.insert(vertices.end(), model.Vertices().begin(), model.Vertices().end());
	//indices.insert(indices.end(), model.Indices().begin(), model.Indices().end());

	vertices.push_back(vertex1);
	vertices.push_back(vertex2);

	indices.push_back(0);
	indices.push_back(1);

	constexpr auto flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Vertices", VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | flags, vertices, vertexBuffer_, vertexBufferMemory_);
	Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Indices", VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | flags, indices, indexBuffer_, indexBufferMemory_);
	Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Offsets", flags, offsets, offsetBuffer_, offsetBufferMemory_);
}

RayScene::~RayScene()
{
	offsetBuffer_.reset();
	offsetBufferMemory_.reset(); // release memory after bound buffer has been destroyed
	indexBuffer_.reset();
	indexBufferMemory_.reset(); // release memory after bound buffer has been destroyed
	vertexBuffer_.reset();
	vertexBufferMemory_.reset(); // release memory after bound buffer has been destroyed
}

}
