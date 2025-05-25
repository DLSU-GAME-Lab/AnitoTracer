#include "InspectorScreen.h"

#include <glm/gtx/string_cast.hpp>

#include "imgui.h"
#include "From-GDGRAP2/ModelManager.h"
#include "UIManager.h"
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
		ImGui::Text("Selected Object: %s", name.c_str());

		this->updateTransformDisplays();
		this->updateLightPropsDisplays();

		bool enabled = this->selectedObject->isEnabled();
		if (ImGui::Checkbox("Enabled", &enabled)) { this->selectedObject->setEnabled(enabled); }
		ImGui::SameLine();
		if (ImGui::Button("Delete")) {
			ModelManager::getInstance()->deleteObject(this->selectedObject);
			ModelManager::getInstance()->setSelectedObject(static_cast<std::shared_ptr<GameObject>>(nullptr));
		}
		if (ImGui::InputFloat3("Position", this->positionDisplay, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) { this->onTransformUpdate(); }
		if (ImGui::InputFloat3("Rotation", this->rotationDisplay, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) { this->onTransformUpdate(); }

		if (this->selectedObject->getType() == GameObject::PrimitiveType::SPHERE)
		{
			if (ImGui::InputFloat("Resize", this->scaleDisplay, 0, 0, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) { this->onTransformUpdate(); }
		}
		else {
			if (ImGui::InputFloat3("Scale", this->scaleDisplay, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) { this->onTransformUpdate(); }
		}

		// Light "Component"
		if (this->selectedObject->getType() == GameObject::PrimitiveType::POINT_LIGHT
			|| this->selectedObject->getType() == GameObject::PrimitiveType::DIRECTIONAL_LIGHT
			|| this->selectedObject->getType() == GameObject::PrimitiveType::SPOT_LIGHT)
		{
			if (ImGui::InputFloat4("Light Color", this->lightColorDisplay, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) { this->onLightPropsUpdate(); }
			if (ImGui::InputFloat4("Ambient Color", this->ambientColorDisplay, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)) { this->onLightPropsUpdate(); }

			// TODO : Light Type
			std::string lightTypes[3] = {"Point Light", "Directional Light", "Spot Light"};
			std::string currentType = (this->selectedObject->getType() == GameObject::POINT_LIGHT) ? lightTypes[0] :
				(this->selectedObject->getType() == GameObject::DIRECTIONAL_LIGHT) ? lightTypes[1] :
				(this->selectedObject->getType() == GameObject::SPOT_LIGHT) ? lightTypes[2] : "None";
			if (ImGui::BeginCombo("Light Type", currentType.c_str(), ImGuiComboFlags_None))
			{
				for (std::string type: lightTypes)
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

		this->drawMaterialsTab();
	}
	else {
		ImGui::Text("No object selected. Select an object first.");
	}

	ImGui::End();
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
		this->lightColorDisplay[0] = lightCol.x;
		this->lightColorDisplay[1] = lightCol.y;
		this->lightColorDisplay[2] = lightCol.z;
		this->lightColorDisplay[3] = lightCol.w;

		glm::vec4 ambientCol = light->getAmbientColor();
		this->ambientColorDisplay[0] = ambientCol.x;
		this->ambientColorDisplay[1] = ambientCol.y;
		this->ambientColorDisplay[2] = ambientCol.z;
		this->ambientColorDisplay[3] = ambientCol.w;

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
			light->setLightColor(this->lightColorDisplay[0], this->lightColorDisplay[1], this->lightColorDisplay[2], this->lightColorDisplay[3]);
			light->setAmbientColor(this->ambientColorDisplay[0], this->ambientColorDisplay[1], this->ambientColorDisplay[2], this->ambientColorDisplay[3]);
			light->setLightType(lightTypeDisplay);
		}
	}
}
