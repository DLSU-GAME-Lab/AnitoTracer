#include "InspectorScreen.h"

#include <glm/gtx/string_cast.hpp>

#include "imgui.h"
#include "imgui_internal.h"
#include "From-GDGRAP2/ModelManager.h"
#include "UIManager.h"
#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/GameObject.h"
#include "From-GDGRAP2/TransformHistory.h"
#include "IconsMaterialDesign.h"
#include "StateManagement/CommandManager.hpp"
#include "StateManagement/ConcreteCommands/InspectorCommands.hpp"

InspectorScreen::InspectorScreen() : AUIScreen(UINames::INSPECTOR_SCREEN)
{
	this->isUniformScalingEnabled = UIManager::getInstance()->config()->inspectorUniformScaling;
}

InspectorScreen::~InspectorScreen()
= default;

void InspectorScreen::drawUI()
{
	//setWindowAlignment(ScreenAlign::TOP_RIGHT);

	ImGui::Begin("Inspector", &enabled, UISettings::GlobalWindowFlags);

	this->selectedObject = ModelManager::getInstance()->getSelectedObject();

	if (this->selectedObject != nullptr)
	{
		if (ImGui::BeginTable("GameObject Quick Options", 3))
		{
			float labelWidth = ImGui::CalcTextSize("Static").x;

			ImGui::TableSetupColumn("ActiveToggle", ImGuiTableColumnFlags_WidthFixed, 30.0f);
			ImGui::TableSetupColumn("GameObjectName", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("StaticToggle", ImGuiTableColumnFlags_WidthFixed, 30.0f + labelWidth);

			{ // Name, Active, and Static Row
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				bool isObjectActive = this->selectedObject->isActive();

				if (ImGui::Checkbox("##Enabled", &isObjectActive))
				{
					CommandManager::getInstance()->executeCommand(
						new AlterTransformCommand(
							this->selectedObject,
							[](GameObject* g, AlterTransformCommand::Variant v) { g->setActive(std::get<bool>(v)); },
							this->selectedObject->isActive(),
							isObjectActive
						));
				}

				ImGui::TableNextColumn();
				ImGui::TableSetColumnIndex(1);

				char nameBuf[256];
				std::strncpy(nameBuf, this->selectedObject->getName().c_str(), sizeof(nameBuf));
				nameBuf[sizeof(nameBuf) - 1] = '\0';
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

				if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
				{
					CommandManager::getInstance()->executeCommand(
						new AlterTransformCommand(
							this->selectedObject,
							[](GameObject* g, AlterTransformCommand::Variant v) { g->setName(std::get<std::string>(v)); },
							this->selectedObject->getName(),
							String(nameBuf)
						));
				}

				ImGui::TableNextColumn();
				ImGui::TableSetColumnIndex(2);

				bool tempStatic = false;
				ImGui::Checkbox("Static", &tempStatic);
			}

			ImGui::EndTable();
		}

		ImGui::Separator();

		this->updateTransformDisplays();
		this->updateLightPropsDisplays();

		bool enabled = this->selectedObject->isActive();

		this->drawTransformTab();
		this->drawLightTab();

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

void InspectorScreen::drawLightTab()
{
	auto type = this->selectedObject->getType();

	bool isLight =
		type == GameObject::PrimitiveType::POINT_LIGHT ||
		type == GameObject::PrimitiveType::DIRECTIONAL_LIGHT ||
		type == GameObject::PrimitiveType::SPOT_LIGHT;

	if (!isLight) return;

	if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginTable("LightInputsTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody))
		{
			float labelWidth = ImGui::CalcTextSize("Light Type").x;

			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
			ImGui::TableSetupColumn("InputFields", ImGuiTableColumnFlags_WidthStretch);

			/* Light Color */
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Light Color");

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(1);
			{
				float previewButtonSize = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.9f;
				float buttonSize = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().FrameBorderSize) * 0.1f;

				if (ImGui::ColorButton("##LightColor_Preview", lightColorDisplay, 0, ImVec2(previewButtonSize, 20.0f)))
				{
					isColorPickerOpen = !isColorPickerOpen;
				}

				ImGui::SameLine();

				// Optional "picker" icon button (like your material UI)
				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[5]);
				if (ImGui::Button(ICON_MD_COLORIZE, ImVec2(buttonSize, 20.0f)))
				{
					isColorPickerOpen = !isColorPickerOpen;
				}
				ImGui::PopFont();
			}

			/* Intensity */
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Intensity");

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(1);
			{
				float sliderSize = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.8f;
				float inputSize = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::GetStyle().FrameBorderSize) * 0.2f;

				ImGui::SetNextItemWidth(sliderSize);
				bool sliderChanged = ImGui::SliderFloat("##Intensity_Slider", &this->intensityDisplay, 0.0f, 1.0f, " %.05f", ImGuiSliderFlags_NoInput);

				ImGui::SameLine();

				ImGui::SetNextItemWidth(inputSize);
				bool inputChanged = ImGui::InputFloat("##Intensity_Input", &this->intensityDisplay, 0.0f, 0.0f, " %.05f");

				if (sliderChanged || inputChanged)
				{
					this->onLightPropsUpdate();
					EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
				}
			}

			/* Light Type */
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Light Type");

			ImGui::TableNextColumn();
			ImGui::TableSetColumnIndex(1);
			{
				const char* lightTypes[] = { "Point Light", "Directional Light", "Spot Light" };

				const char* currentType =
					(this->selectedObject->getType() == GameObject::POINT_LIGHT) ? lightTypes[0] :
					(this->selectedObject->getType() == GameObject::DIRECTIONAL_LIGHT) ? lightTypes[1] :
					(this->selectedObject->getType() == GameObject::SPOT_LIGHT) ? lightTypes[2] : "None";

				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::BeginCombo("##LightType_Combo", currentType, ImGuiComboFlags_None))
				{
					for (int i = 0; i < 3; i++)
					{
						bool isSelected = (strcmp(currentType, lightTypes[i]) == 0);
						if (ImGui::Selectable(lightTypes[i], isSelected))
						{
							lightTypeDisplay =
								(i == 0) ? Light::PointLight :
								(i == 1) ? Light::DirectionalLight :
								(i == 2) ? Light::SpotLight : Light::PointLight;

							this->onLightPropsUpdate();
							EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
						}
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}

			ImGui::EndTable();
		}
	}

	else
	{
		isLight = false;
	}
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
	Light* light = dynamic_cast<Light*>(this->selectedObject);

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

bool InspectorScreen::IsUniformScalingEnabled() const
{
	return this->isUniformScalingEnabled;
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

void InspectorScreen::drawTransformTab()
{
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginTable("TransformTable", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody)) // Label, Button, Input Rects
		{
			float labelWidth = ImGui::CalcTextSize("Rotation").x;

			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
			ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed, this->transformUniformScalingButtonWidth); // reserve space for link button
			ImGui::TableSetupColumn("Fields", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Position");
			ImGui::TableSetColumnIndex(1);
			ImGui::Dummy(ImVec2(this->transformUniformScalingButtonWidth, 0.0f)); // placeholder to keep alignment
			ImGui::TableSetColumnIndex(2);
			this->drawVector3Field("Pos", this->positionDisplay, EditorAction::Move);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Rotation");
			ImGui::TableSetColumnIndex(1);
			ImGui::Dummy(ImVec2(this->transformUniformScalingButtonWidth, 0.0f));
			ImGui::TableSetColumnIndex(2);
			this->drawVector3Field("Rot", this->rotationDisplay, EditorAction::Rotate);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Scale");
			ImGui::TableSetColumnIndex(1);
			ImGui::PushID("ScaleLink");

			ImGui::PushFont(UIManager::getInstance()->GetIconFont());

			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[3]);
			if (ImGui::Selectable(this->isUniformScalingEnabled ? ICON_MD_LINK : ICON_MD_LINK_OFF))
			{
				this->isUniformScalingEnabled = !this->isUniformScalingEnabled;
				UIManager::getInstance()->config()->inspectorUniformScaling = this->isUniformScalingEnabled;
			}
			ImGui::PopFont();

			ImGui::PopFont();
			ImGui::PopID();

			ImGui::TableSetColumnIndex(2);
			this->drawVector3Field("Sca", this->scaleDisplay, EditorAction::Scale);

			ImGui::EndTable();
		}
	}
}

void InspectorScreen::drawVector3Field(const char* label, float* values, EditorAction action)
{
	ImGui::PushID(label);

	bool isUpdated = false;
	glm::vec3 scale;

	//Calculate Field Width
	float totalAvailableSpace = ImGui::GetContentRegionAvail().x;
	float spacing = ImGui::GetStyle().ItemSpacing.x;

	float labelWidth = ImGui::CalcTextSize("X").x;
	labelWidth = std::max(labelWidth, ImGui::CalcTextSize("Y").x);
	labelWidth = std::max(labelWidth, ImGui::CalcTextSize("Z").x);

	labelWidth += ImGui::GetStyle().ItemInnerSpacing.x;

	float totalLabelSpace = (labelWidth + spacing) * 3.0f - spacing;

	float inputWidth = (totalAvailableSpace - totalLabelSpace) / 3.0f;
	if (inputWidth < 1.0f) inputWidth = 1.0f; // min spacing

	auto axisInput = [&](const char* name, float& v)
		{
			ImGui::AlignTextToFramePadding();
			ImGui::Text(name);
			ImGui::SameLine();

			ImGui::PushItemWidth(inputWidth - ImGui::GetStyle().ItemSpacing.x / 2.0f);

			std::string id = "##";
			id += label;
			id += name;

			if (ImGui::InputFloat(id.c_str(), &v, 0.0f, 0.0f, "%0.3f"))
			{
				if (ImGui::IsItemDeactivatedAfterEdit())
					isUpdated = true;
			}

			ImGui::PopItemWidth();

			ImGui::SameLine();
		};

	if (action == EditorAction::Scale) //record old scale for uniform scaling
	{
		scale = { values[0], values[1], values[2] };
	}

	axisInput("X", values[0]);
	axisInput("Y", values[1]);
	axisInput("Z", values[2]);

	if (isUpdated)
	{
		switch (action)
		{
		case EditorAction::Move: 

			CommandManager::getInstance()->executeCommand(
				new AlterTransformCommand(
					this->selectedObject,
					[](GameObject* g, AlterTransformCommand::Variant v) { g->setLocalPosition(std::get<glm::vec3>(v)); },
					this->selectedObject->getLocalPosition(),
					glm::vec3(values[0], values[1], values[2])
				));

			break;

		case EditorAction::Rotate:

			CommandManager::getInstance()->executeCommand(
				new AlterTransformCommand(
					this->selectedObject,
					[](GameObject* g, AlterTransformCommand::Variant v) { g->setLocalRotation(std::get<glm::vec3>(v)); },
					this->selectedObject->getLocalRotation(),
					glm::vec3(values[0], values[1], values[2])
				));

			break;

		case EditorAction::Scale:
			if(IsUniformScalingEnabled()) 
				scale = ScaleUniformly(scale, values);

			CommandManager::getInstance()->executeCommand(
				new AlterTransformCommand(
					this->selectedObject,
					[](GameObject* g, AlterTransformCommand::Variant v) { g->setLocalScale(std::get<glm::vec3>(v)); },
					this->selectedObject->getLocalScale(),
					scale)
				);

			break;
		}
	}

	ImGui::PopID();
}

// Helper function
glm::vec3 InspectorScreen::ScaleUniformly(const glm::vec3& beforeScale, const float* values)
{
	glm::vec3 result = beforeScale;

	if (values[0] != beforeScale.x)
	{
		float ratio = values[0] / beforeScale.x;
		result.x = values[0];
		result.y = beforeScale.y * ratio;
		result.z = beforeScale.z * ratio;
	}
	else if (values[1] != beforeScale.y)
	{
		float ratio = values[1] / beforeScale.y;
		result.x = beforeScale.x * ratio;
		result.y = values[1];
		result.z = beforeScale.z * ratio;
	}
	else if (values[2] != beforeScale.z)
	{
		float ratio = values[2] / beforeScale.z;
		result.x = beforeScale.x * ratio;
		result.y = beforeScale.y * ratio;
		result.z = values[2];
	}

	return result;
}

void InspectorScreen::setUniformScalingEnabled(bool flag)
{
	this->isUniformScalingEnabled = flag;
	UIManager::getInstance()->config()->inspectorUniformScaling = flag;
}

void InspectorScreen::onLightPropsUpdate() const
{
	if (this->selectedObject != nullptr)
	{
		Light* light = dynamic_cast<Light*>(this->selectedObject);
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
