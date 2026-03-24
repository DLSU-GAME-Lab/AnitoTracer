#include "InspectorScreen.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "UIManager.h"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/GameObject.h"
#include "IconsMaterialDesign.h"
#include "Engine/CameraSystem/Camera.h"
#include "StateManagement/CommandManager.hpp"
#include "StateManagement/ConcreteCommands/InspectorCommands.hpp"

#include <string>

template <typename ButtonFn, typename FieldFn>
static void DrawTransformRow(const char* label, float labelWidth, float buttonWidth, ButtonFn&& button, FieldFn&& field)
{
	ImGui::TableNextRow();

	ImGui::TableSetColumnIndex(0);
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);

	ImGui::TableSetColumnIndex(1);
	button(buttonWidth);

	ImGui::TableSetColumnIndex(2);
	field();
}


InspectorScreen::InspectorScreen() : AUIScreen(UINames::INSPECTOR_SCREEN)
{
	this->isUniformScalingEnabled = UIManager::getInstance()->config()->inspectorUniformScaling;
}

InspectorScreen::~InspectorScreen()
= default;

void InspectorScreen::drawUI()
{
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

		this->drawTransformTab();
		this->drawLightTab();
		this->drawCameraTab();

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
	const auto type = selectedObject->getType();
	isLight =
		type == GameObject::PrimitiveType::POINT_LIGHT ||
		type == GameObject::PrimitiveType::DIRECTIONAL_LIGHT ||
		type == GameObject::PrimitiveType::SPOT_LIGHT;

	if (!isLight) return;
	if (!ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) return;

	if (!ImGui::BeginTable("LightInputsTable", 2,
		ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody))
		return;

	const float labelWidth = ImGui::CalcTextSize("Light Type").x;
	ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
	ImGui::TableSetupColumn("InputFields", ImGuiTableColumnFlags_WidthStretch);

	const auto& style = ImGui::GetStyle();
	const float spacingX = style.ItemSpacing.x;

	auto labelCell = [](const char* text)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(text);
			ImGui::TableNextColumn();
		};

	auto markDirtyAndApply = [&]()
		{
			onLightPropsUpdate();
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::ON_MARK_SCENE_DIRTY);
		};

	// ---- Light Color ----
	labelCell("Light Color");
	{
		const float avail = ImGui::GetContentRegionAvail().x;
		const float previewW = (avail - spacingX) * 0.9f;
		const float buttonW = (avail - spacingX - style.FrameBorderSize) * 0.1f;

		if (ImGui::ColorButton("##LightColor_Preview", lightColorDisplay, 0, ImVec2(previewW, 20.0f)))
			isColorPickerOpen = !isColorPickerOpen;

		ImGui::SameLine();

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[5]);
		if (ImGui::Button(ICON_MD_COLORIZE, ImVec2(buttonW, 20.0f)))
			isColorPickerOpen = !isColorPickerOpen;
		ImGui::PopFont();
	}

	// ---- Intensity ----
	labelCell("Intensity");
	{
		const float avail = ImGui::GetContentRegionAvail().x;
		const float sliderW = (avail - spacingX) * 0.8f;
		const float inputW = (avail - spacingX - style.FrameBorderSize) * 0.2f;

		ImGui::SetNextItemWidth(sliderW);
		const bool sliderChanged =
			ImGui::SliderFloat("##Intensity_Slider", &intensityDisplay, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_NoInput);

		ImGui::SameLine();

		ImGui::SetNextItemWidth(inputW);
		const bool inputChanged =
			ImGui::InputFloat("##Intensity_Input", &intensityDisplay, 0.0f, 0.0f, "%.2f");

		if (sliderChanged || inputChanged)
			markDirtyAndApply();
	}

	// ---- Light Type ----
	labelCell("Light Type");
	{
		static constexpr const char* kNames[] = { "Point Light", "Directional Light", "Spot Light" };

		auto typeToIndex = [](GameObject::PrimitiveType t) -> int
			{
				switch (t)
				{
				case GameObject::PrimitiveType::POINT_LIGHT:       return 0;
				case GameObject::PrimitiveType::DIRECTIONAL_LIGHT: return 1;
				case GameObject::PrimitiveType::SPOT_LIGHT:        return 2;
				default:                                           return 0;
				}
			};

		auto indexToLightType = [](int i) -> Light::LightType
			{
				switch (i)
				{
				case 0: return Light::PointLight;
				case 1: return Light::DirectionalLight;
				case 2: return Light::SpotLight;
				default: return Light::PointLight;
				}
			};

		int currentIndex = typeToIndex(type);

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##LightType_Combo", kNames[currentIndex]))
		{
			for (int i = 0; i < 3; ++i)
			{
				const bool selected = (i == currentIndex);
				if (ImGui::Selectable(kNames[i], selected))
				{
					lightTypeDisplay = indexToLightType(i);
					markDirtyAndApply();
					currentIndex = i;
				}
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	ImGui::EndTable();
}

void InspectorScreen::drawCameraTab()
{
	auto cam = dynamic_cast<Camera*>(this->selectedObject);
	if (!cam) return;

	if (ImGui::CollapsingHeader("Camera"))
	{
		static float editLookAt[3] = { 0, 0, 0 };
		static bool initialized = false;

		ImGui::Text("Set Look At:");
		ImGui::InputFloat3("##LookAt", editLookAt);

		if (ImGui::Button("Apply LookAt"))
		{
			auto activeCam = cam;
			if (activeCam)
			{
				activeCam->lookAt(glm::vec3(editLookAt[0], editLookAt[1], editLookAt[2]));
			}
		}
	}
	if (ImGui::CollapsingHeader("Animation")) 
	{
		static float duration = cam->getDuration();
		int frameCount = 1;
		//TODO: Add keyframe list
		if (ImGui::TreeNode("Keyframes")) {
			if (cam->getKeyFrames().empty())
			{
				ImGui::Text("No keyframes added.");
			}
			else
			{
				for (const auto& keyframe : cam->getKeyFrames())
				{
					std::string name = "Keyframe " + std::to_string(frameCount);
					ImGui::Text(name.c_str());
					/*if (ImGui::Button(name.c_str()))
					{
						cam->getKeyFrames().erase(cam->getKeyFrames().begin() + (frameCount - 1));
					}*/
					frameCount++;
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::Button("Add Keyframe"))
		{
			auto activeCam = cam;
			if (activeCam)
			{
				activeCam->addKeyFrame();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove Last Keyframe"))
		{
			auto activeCam = cam;
			if (activeCam)
			{
				activeCam->removeLastKeyFrame();
			}
		}
		if (ImGui::Button("Start"))
		{
			auto activeCam = cam;
			if (activeCam)
			{
				activeCam->setDuration(duration);
				activeCam->Animate();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Pause"))
		{
			auto activeCam = cam;
			if (activeCam)
			{
				activeCam->TogglePause();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop"))
		{
			auto activeCam = cam;
			if (activeCam)
			{
				activeCam->StopAnimate();
			}
		}
		if (ImGui::InputFloat("##Duration", &duration) )
		{
			auto activeCam = cam;
			if (activeCam)
			{
				activeCam->setDuration(duration);
			}
		}


	}
}


void InspectorScreen::updateTransformDisplays()
{
	typedef glm::vec3 vec3;
	vec3 pos = this->selectedObject->getLocalPosition();
	this->positionDisplay[0] = pos.x;
	this->positionDisplay[1] = pos.y;
	this->positionDisplay[2] = pos.z;

	vec3 rot = this->selectedObject->getLocalRotationEuler();
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

bool InspectorScreen::IsUniformScalingEnabled() const
{
	return this->isUniformScalingEnabled;
}

void InspectorScreen::drawTransformTab()
{
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginTable("TransformTable", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody))
		{
			auto transformLabelWidth = ImGui::CalcTextSize("Rotation").x;
			auto linkIconWidth = ImGui::CalcTextSize(ICON_MD_LINK).x;

			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, transformLabelWidth);
			ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed, linkIconWidth);
			ImGui::TableSetupColumn("Fields", ImGuiTableColumnFlags_WidthStretch);

			auto placeholder = [](float buttonWidth)
				{
					ImGui::Dummy(ImVec2(buttonWidth, 0.0f));
				};

			DrawTransformRow("Position", transformLabelWidth, linkIconWidth,
				placeholder,
				[&] { drawVector3Field("Pos", positionDisplay, EditorAction::Move); }
			);

			DrawTransformRow("Rotation", transformLabelWidth, linkIconWidth,
				placeholder,
				[&] { drawVector3Field("Rot", rotationDisplay, EditorAction::Rotate); }
			);

			DrawTransformRow("Scale", transformLabelWidth, linkIconWidth,
				[&](float buttonWidth)
				{
					ImGui::PushID("ScaleLink");
					ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[3]); 

					const char* icon = isUniformScalingEnabled ? ICON_MD_LINK : ICON_MD_LINK_OFF;
					if (ImGui::Selectable(icon))
					{
						isUniformScalingEnabled = !isUniformScalingEnabled;
						UIManager::getInstance()->config()->inspectorUniformScaling = isUniformScalingEnabled;
					}

					ImGui::PopFont();
					ImGui::PopID();
				},
				[&] { drawVector3Field("Sca", scaleDisplay, EditorAction::Scale); }
			);

			ImGui::EndTable();
		}
	}

}

void InspectorScreen::drawVector3Field(const char* label, float* values, EditorAction action)
{
	ImGui::PushID(label);

	// Width calculation
	const float avail = ImGui::GetContentRegionAvail().x;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float inner = ImGui::GetStyle().ItemInnerSpacing.x;

	float axisLabelW = ImGui::CalcTextSize("X").x;
	axisLabelW = std::max(axisLabelW, ImGui::CalcTextSize("Y").x);
	axisLabelW = std::max(axisLabelW, ImGui::CalcTextSize("Z").x);
	axisLabelW += inner;

	const float totalLabelSpace = (axisLabelW * 3.0f) + (spacing * 2.0f); // X [space] Y [space] Z
	float inputW = (avail - totalLabelSpace) / 3.0f;
	if (inputW < 1.0f) inputW = 1.0f;

	bool anyChanged = false;
	bool anyEditEnded = false;

	auto axisInput = [&](const char* axis, int idx)
		{
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(axis);
			ImGui::SameLine();

			ImGui::PushItemWidth(inputW);

			const bool changed = ImGui::InputFloat(axis[0] == 'X' ? "##X" : axis[0] == 'Y' ? "##Y" : "##Z", &values[idx], 0, 0, "%.2f");

			anyChanged |= changed;
			if (ImGui::IsItemDeactivatedAfterEdit())
				anyEditEnded = true;

			ImGui::PopItemWidth();

			if (axis[0] != 'Z')
				ImGui::SameLine();
		};

	glm::vec3 before;
	switch (action)
	{
	case EditorAction::Move:   before = selectedObject->getLocalPosition(); break;
	case EditorAction::Rotate: before = selectedObject->getLocalRotationEuler(); break;
	case EditorAction::Scale:  before = selectedObject->getLocalScale();    break;
	}

	axisInput("X", 0);
	axisInput("Y", 1);
	axisInput("Z", 2);

	if (anyChanged && anyEditEnded)
	{
		glm::vec3 after(values[0], values[1], values[2]);

		if (action == EditorAction::Scale && IsUniformScalingEnabled())
			after = ScaleUniformly(before, values);

		switch (action)
		{
		case EditorAction::Move:
			CommandManager::getInstance()->executeCommand(
				new AlterTransformCommand(
					selectedObject,
					[](GameObject* g, AlterTransformCommand::Variant v) { g->setLocalPosition(std::get<glm::vec3>(v)); },
					before,
					after));
			break;

		case EditorAction::Rotate:
			CommandManager::getInstance()->executeCommand(
				new AlterTransformCommand(
					selectedObject,
					[](GameObject* g, AlterTransformCommand::Variant v) { g->setLocalRotationEuler(std::get<glm::vec3>(v)); },
					before,
					after));
			break;

		case EditorAction::Scale:
			CommandManager::getInstance()->executeCommand(
				new AlterTransformCommand(
					selectedObject,
					[](GameObject* g, AlterTransformCommand::Variant v) { g->setLocalScale(std::get<glm::vec3>(v)); },
					before,
					after));
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
	ImGui::SetNextWindowPos(ImGui::FindWindowByName("Inspector")->Pos);
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
