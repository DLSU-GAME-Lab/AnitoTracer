#pragma once
#include "AUIScreen.h"
#include "Engine/LightSystem/Light.h"
#include "From-GDGRAP2/GameObject.h"

class Texture;
class InspectorScreen :    public AUIScreen
{

public:
	InspectorScreen();
	~InspectorScreen();
	void SendResult(String materialPath);

	bool IsUniformScalingEnabled() const;
	
private:

	void onTransformUpdate() const;
	void onLightPropsUpdate() const;
	void showColorPickerWindow();
	virtual void drawUI() override;
	void updateTransformDisplays();
	void updateLightPropsDisplays();
	void FormatMatImage();
	void drawMaterialsTab();
	friend class UIManager;

	float positionDisplay[3] = {0.0f, 0.0f, 0.0f};
	float rotationDisplay[3] = {0.0f, 0.0f, 0.0f};
	float scaleDisplay[3] = { 1.0f, 1.0f, 1.0f };
	bool popupOpen = false;

	bool isLight = false;
	ImVec4 lightColor = ImVec4(1, 1, 1, 1);
	float intensityDisplay = 1.0f;
	bool isColorPickerOpen = false;
	ImVec4 lightColorDisplay = ImVec4(1, 1, 1, 1);
	Light::LightType lightTypeDisplay = Light::PointLight;

	bool isUniformScalingEnabled = true;

	std::shared_ptr<GameObject> selectedObject = nullptr;
	const String DEFAULT_MATERIAL = "None";
	String materialPath = DEFAULT_MATERIAL;
	String materialName = DEFAULT_MATERIAL;
	Texture* materialDisplay;
	
	float lightIntensityMultiplier = 500000.0f;
};

