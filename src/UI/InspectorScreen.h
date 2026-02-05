#pragma once
#include "AUIScreen.h"
#include "Engine/LightSystem/Light.h"
#include "From-GDGRAP2/GameObject.h"

enum EditorAction
{
	Move = 0,
	Rotate,
	Scale
};

class Texture;
class InspectorScreen :    public AUIScreen
{
public:
	InspectorScreen();
	~InspectorScreen();

	bool IsUniformScalingEnabled() const;
	
private:

	virtual void drawUI() override;

	void drawTransformTab();
	void drawLightTab();
	void drawCameraTab();
	void showColorPickerWindow();

	void updateTransformDisplays();
	void updateLightPropsDisplays();
	void onLightPropsUpdate() const;

	void drawVector3Field(const char* label, float* values, EditorAction action);
	glm::vec3 ScaleUniformly(const glm::vec3& beforeScale, const float* values);
	void setUniformScalingEnabled(bool flag);

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

	bool isUniformScalingEnabled = false;

	GameObject* selectedObject = nullptr;
	const String DEFAULT_MATERIAL = "None";
	String materialPath = DEFAULT_MATERIAL;
	String materialName = DEFAULT_MATERIAL;
	Texture* materialDisplay;
	
	float lightIntensityMultiplier = 500000.0f;
	
};