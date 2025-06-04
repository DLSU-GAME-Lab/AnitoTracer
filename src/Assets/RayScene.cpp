#include "RayScene.hpp"

#include <iostream>

#include "Ray.hpp"
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
	//std::vector<uint32_t> indices;
	//std::vector<glm::uvec2> offsets;

	Vertex vertex1{ vec3(50.0f,-100.0f,0.0f), vec3(0,0,0), vec2(0,0), -1 };
	Vertex vertex2{ vec3(-50.0f,100.0f,0.0f), vec3(0,0,0), vec2(0,0), -1 };
	Vertex vertex3{ vec3(50.0f,2000.0f,2000.0f), vec3(0,0,0), vec2(0,0), -1 };

	// Remember the index, vertex offsets.
	//const auto indexOffset = static_cast<uint32_t>(indices.size());
	//const auto vertexOffset = static_cast<uint32_t>(vertices.size());

	//offsets.emplace_back(indexOffset, vertexOffset);

	// Copy model data one after the other.
	//vertices.insert(vertices.end(), model.Vertices().begin(), model.Vertices().end());
	//indices.insert(indices.end(), model.Indices().begin(), model.Indices().end());

	vertices.push_back(vertex1);
	vertices.push_back(vertex2);
	vertices.push_back(vertex3);

	//indices.push_back(0);
	//indices.push_back(1);

	rays_.push_back(new Ray(commandPool, vertices));

	//constexpr auto flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	//Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Vertices", VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | flags, vertices, vertexBuffer_, vertexBufferMemory_);
}

RayScene::~RayScene()
{
	rays_.clear();
	//vertexBuffer_.reset();
	//vertexBufferMemory_.reset(); // release memory after bound buffer has been destroyed
}

void RayScene::Update()
{

}
}
