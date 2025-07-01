#include "InspectorScreen.h"

#include <glm/gtx/string_cast.hpp>

#include "imgui.h"
#include "imgui_internal.h"
#include "From-GDGRAP2/ModelManager.h"
#include "UIManager.h"
#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/GameObject.h"

InspectorScreen::InspectorScreen() : AUIScreen(UINames::INSPECTOR_SCREEN)
{
}

InspectorScreen::~InspectorScreen()
= default;

void InspectorScreen::drawUI()
{
	//setWindowAlignment(ScreenAlign::TOP_RIGHT);

	ImGui::Begin("Inspector Window", nullptr, UISettings::GlobalWindowFlags);
	this->selectedObject = ModelManager::getInstance()->getSelectedObject();
	if (this->selectedObject != nullptr)
	{
		String name = this->selectedObject->getName();
		ImGui::TextWrapped("Selected Object: %s", name.c_str());

		this->updateTransformDisplays();
		this->updateLightPropsDisplays();

		bool enabled = this->selectedObject->isEnabled();
		if (ImGui::Checkbox("Enabled", &enabled)) { this->selectedObject->setEnabled(enabled); }
		ImGui::SameLine();
		if (ImGui::Button("Delete")) {
			ModelManager::getInstance()->deleteObject(this->selectedObject);
			ModelManager::getInstance()->setSelectedObject(static_cast<std::shared_ptr<GameObject>>(nullptr));
		}

		if (ImGui::InputFloat3("Position", this->positionDisplay, "%.3f")) { if (ImGui::IsItemDeactivatedAfterEdit())this->onTransformUpdate(); }
		if (ImGui::InputFloat3("Rotation", this->rotationDisplay, "%.3f")) { if (ImGui::IsItemDeactivatedAfterEdit()) this->onTransformUpdate(); }

		if (this->selectedObject->getType() == GameObject::PrimitiveType::SPHERE)
		{
			if (ImGui::InputFloat("Resize", this->scaleDisplay, 0, 0, "%.3f")) { if (ImGui::IsItemDeactivatedAfterEdit())this->onTransformUpdate(); }
		}
		else {
			if (ImGui::InputFloat3("Scale", this->scaleDisplay, "%.3f")) { if (ImGui::IsItemDeactivatedAfterEdit()) this->onTransformUpdate(); }
		}

		// Light "Component"
		if (this->selectedObject->getType() == GameObject::PrimitiveType::POINT_LIGHT
			|| this->selectedObject->getType() == GameObject::PrimitiveType::DIRECTIONAL_LIGHT
			|| this->selectedObject->getType() == GameObject::PrimitiveType::SPOT_LIGHT)
		{
			isLight = true;
			ImGui::Separator();

			ImGui::Text("Light Color");
			ImGui::SameLine();
			if (ImGui::ColorButton("Light Color", lightColorDisplay, 0, ImVec2(75, 25)))
			{
				isColorPickerOpen = !isColorPickerOpen;
			}
			if (ImGui::SliderFloat("Intensity", &this->intensityDisplay, 0, 1))
			{
				this->onLightPropsUpdate();
				EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
			}

			// TODO : Light Type
			std::string lightTypes[3] = { "Point Light", "Directional Light", "Spot Light" };
			std::string currentType = (this->selectedObject->getType() == GameObject::POINT_LIGHT) ? lightTypes[0] :
				(this->selectedObject->getType() == GameObject::DIRECTIONAL_LIGHT) ? lightTypes[1] :
				(this->selectedObject->getType() == GameObject::SPOT_LIGHT) ? lightTypes[2] : "None";
			if (ImGui::BeginCombo("Light Type", currentType.c_str(), ImGuiComboFlags_None))
			{
				for (std::string type : lightTypes)
				{
					if (ImGui::Selectable(type.c_str()))
					{
						lightTypeDisplay = (type == lightTypes[0]) ? Light::PointLight :
							(type == lightTypes[1]) ? Light::DirectionalLight :
							(type == lightTypes[2]) ? Light::SpotLight : Light::PointLight;
						this->onLightPropsUpdate();
					}
				}
				ImGui::EndCombo();
			}
		}
		else { isLight = false; }

		this->drawMaterialsTab();
	}
	else {
		ImGui::TextWrapped("No object selected. Select an object first.");
	}

	ImGui::End();

	// Color Picker
	if (isColorPickerOpen && !enabled)
		isColorPickerOpen = false;

	if (isColorPickerOpen)
		showColorPickerWindow();
}

void InspectorScreen::updateTransformDisplays()
{
	typedef glm::vec3 vec3;
	vec3 pos = this->selectedObject->getLocalPosition();
	this->positionDisplay[0] = pos.x;
	this->positionDisplay[1] = pos.y;
	this->positionDisplay[2] = pos.z;

	vec3 rot = this->selectedObject->getLocalRotation();
	this->rotationDisplay[0] = rot.x;
	this->rotationDisplay[1] = rot.y;
	this->rotationDisplay[2] = rot.z;

	vec3 scale = this->selectedObject->getLocalScale();
	this->scaleDisplay[0] = scale.x;
	this->scaleDisplay[1] = scale.y;
	this->scaleDisplay[2] = scale.z;
}

void InspectorScreen::updateLightPropsDisplays()
{
	std::shared_ptr<Light> light = ModelManager::getInstance()->findLightObjectByName(this->selectedObject->getName());
	if (light)
	{
		glm::vec4 lightCol = light->getLightColor();
		this->lightColorDisplay.x = lightCol.x;
		this->lightColorDisplay.y = lightCol.y;
		this->lightColorDisplay.z = lightCol.z;

		// Divide intensity by 1000000.0f (max intensity) so it's a range between 0 to 1.
		this->intensityDisplay = lightCol.a / lightIntensityMultiplier;

		Light::LightType type = (light->getLightType() == Assets::LightProperties::Enum::PointLight) ? Light::PointLight :
			(light->getLightType() == Assets::LightProperties::Enum::DirectionalLight) ? Light::DirectionalLight :
			(light->getLightType() == Assets::LightProperties::Enum::SpotLight) ? Light::SpotLight : Light::PointLight;
		this->lightTypeDisplay = type;
	}
}

void InspectorScreen::SendResult(String materialPath)
{
	// TexturedCube* texturedObj = static_cast<TexturedCube*>(this->selectedObject);
	// texturedObj->getRenderer()->setMaterialPath(materialPath);
	// this->popupOpen = false;
}

void InspectorScreen::FormatMatImage()
{
	//convert to wchar format
	// String textureString = this->materialPath;
	// std::wstring widestr = std::wstring(textureString.begin(), textureString.end());
	// const wchar_t* texturePath = widestr.c_str();
	//
	// this->materialDisplay = static_cast<Texture*>(TextureManager::getInstance()->createTextureFromFile(texturePath));
}

void InspectorScreen::drawMaterialsTab()
{
	int BUTTON_WIDTH = 225;
	int BUTTON_HEIGHT = 20;

	// if(this->selectedObject->getType() != AHittable::TEXTURED_CUBE)
	// {
	// 	return;
	// }

	// TexturedCube* texturedObj = static_cast<TexturedCube*>(this->selectedObject);
	// this->materialPath = texturedObj->getRenderer()->getMaterialPath();
	// this->FormatMatImage();
	// ImGui::SetCursorPosX(50);
	// ImGui::Image(static_cast<void*>(this->materialDisplay->getShaderResource()), ImVec2(150, 150));
	//
	// std::vector<String> paths = StringUtils::split(this->materialPath, '\\');
	// this->materialName = paths[paths.size() - 1];
	// String displayText = "Material: " + this->materialName;
	// ImGui::Text(displayText.c_str());
	// if (ImGui::Button("Add Material", ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
	// 	this->popupOpen = !this->popupOpen;
	// 	UINames uiNames;
	// 	MaterialScreen* materialScreen = static_cast<MaterialScreen*>(UIManager::getInstance()->findUIByName(uiNames.MATERIAL_SCREEN));
	// 	materialScreen->linkInspectorScreen(this, this->materialPath);
	// 	UIManager::getInstance()->setEnabled(uiNames.MATERIAL_SCREEN, this->popupOpen);
	// }
}

void InspectorScreen::onTransformUpdate() const
{
	if (this->selectedObject != nullptr)
	{
		// ActionHistory::getInstance()->recordAction(this->selectedObject);

		this->selectedObject->setLocalPosition(this->positionDisplay[0], this->positionDisplay[1], this->positionDisplay[2]);
		this->selectedObject->setLocalRotation(this->rotationDisplay[0], this->rotationDisplay[1], this->rotationDisplay[2]);

		if (this->selectedObject->getType() == GameObject::PrimitiveType::SPHERE)
		{
			this->selectedObject->setLocalScale(this->scaleDisplay[0], this->scaleDisplay[0], this->scaleDisplay[0]);
		}
		else
		{
			this->selectedObject->setLocalScale(this->scaleDisplay[0], this->scaleDisplay[1], this->scaleDisplay[2]);
		}

	}
}

void InspectorScreen::onLightPropsUpdate() const
{
	if (this->selectedObject != nullptr)
	{
		std::shared_ptr<Light> light = ModelManager::getInstance()->findLightObjectByName(this->selectedObject->getName());
		if (light)
		{
			light->setLightColor(this->lightColorDisplay.x, this->lightColorDisplay.y, this->lightColorDisplay.z, intensityDisplay * lightIntensityMultiplier);
			light->setLightType(lightTypeDisplay);
		}
	}
}

void InspectorScreen::showColorPickerWindow()
{
	ImGui::SetNextWindowPos(ImGui::FindWindowByName("Inspector Window")->Pos);
	ImGui::SetNextWindowSize(ImVec2(300, 350));
	if (ImGui::Begin("Light Color Picker", &isColorPickerOpen) && isLight)
	{
		ImGui::SameLine();
		ImGui::ColorPicker3("Light Color", reinterpret_cast<float*>(&lightColor), 0);

		if (ImGui::Button("Close & Apply"))
		{
			isColorPickerOpen = false;

			lightColorDisplay.x = lightColor.x;
			lightColorDisplay.y = lightColor.y;
			lightColorDisplay.z = lightColor.z;
			onLightPropsUpdate();
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		}
	}
	ImGui::End();
}
