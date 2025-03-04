#pragma once
#include <imgui.h>

#include "AUIScreen.h"
#include "Vulkan/DescriptorSetLayout.hpp"

class ViewportScreen final :
    public AUIScreen
{
	enum EFillMode
	{
		SOLID,
		WIREFRAME,
		SOLID_WIREFRAME
	};

private:
	int index;
	int camIndex;

	// SceneCameraPtr currentCamera;
	// SceneCameraPtr ownCamera;

	EFillMode currentFillMode = SOLID;

	//RenderTexturePtr renderTexture;

	bool isRightMouseDown = false;

	int selectedProj = 0;
	int selectedState = 0;
	int selectedCameraIndex = 0;
	//std::vector<VkDescriptorSet> m_Dset;

public:
	ViewportScreen();
	virtual ~ViewportScreen() = default;

protected:
	void drawUI() override;
	void drawViewportUI(ImVec2 position);
};

