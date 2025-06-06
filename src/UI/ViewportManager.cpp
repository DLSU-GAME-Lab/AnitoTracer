#include "ViewportManager.h"

#include "Assets/UniformBuffer.hpp"
#include "Vulkan/Viewport.h"

ViewportManager* ViewportManager::P_SHARED_INSTANCE = nullptr;
ViewportManager::ViewportManager()
{

}

ViewportManager::~ViewportManager()
{
}

ViewportManager::ViewportManager(const ViewportManager&) {}

ViewportManager* ViewportManager::getInstance()
{
	//if (P_SHARED_INSTANCE == NULL)
	//	P_SHARED_INSTANCE = new ViewportManager();

	return P_SHARED_INSTANCE;
}

void ViewportManager::initialize()
{
	P_SHARED_INSTANCE = new ViewportManager();
}

void ViewportManager::destroy()
{
	delete P_SHARED_INSTANCE;
}

void ViewportManager::renderScenes(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	/*for (Vulkan::Viewport* viewport : this->viewports)
	{
		viewport->Render(commandBuffer, imageIndex);
	}*/
	viewport->Render(commandBuffer, imageIndex);
}
void ViewportManager::drawUI()
{
	/*for (Vulkan::Viewport* viewport : this->viewports)
	{
		viewport->drawUI();
	}*/
	viewport->drawUI();
}

void ViewportManager::createViewport(const class Vulkan::SwapChain& swapChain, const class Assets::Scene& scene)
{
	//Vulkan::Viewport* viewport = new Vulkan::Viewport(swapChain, scene);

	//this->viewports.push_back(viewport);

	//UIManager::getInstance()->addViewport(viewport);

	viewport.reset(new Vulkan::Viewport(swapChain, scene));
}

void ViewportManager::deleteViewport(Vulkan::Viewport* viewport)
{
	/*int index = 0;
	for (int i = 0; i < this->viewports.size(); i++)
	{
		if (this->viewports[i] == viewport)
		{
			this->viewports.erase(this->viewports.begin() + index);
		}

		index++;
	}
	delete viewport;*/
}

void ViewportManager::deleteAllViewports()
{
	/*for (Vulkan::Viewport* viewport : this->viewports)
	{
		delete viewport;
	}

	this->viewports.clear();*/
}

void ViewportManager::addViewport(AUIScreen* viewport)
{
	//Vulkan::Viewport* c_viewport = (Vulkan::Viewport*)viewport;
	//this->viewports.push_back(c_viewport);

}

void ViewportManager::setNumViewports(int count)
{
	/*while (viewports.size() > count)
	{
		this->deleteViewport(viewports.back());
	}

	while (viewports.size() < count)
	{
		//this->createViewport();
	}*/
}

std::vector<Vulkan::Viewport*> ViewportManager::getViewports()
{
	return this->viewports;
}