#include "InspectorScreen.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "UIManager.h"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/GameObject.h"
#include "IconsMaterialDesign.h"
#include "Engine/CameraSystem/Camera.h"
#include "Engine/Physics/TracerPhysics.h"
#include "StateManagement/CommandManager.hpp"
#include "StateManagement/ConcreteCommands/InspectorCommands.hpp"
#include "Vulkan/Vk_Game/GameRenderer.hpp"

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

	// Reset per-light shadow edit state only when a DIFFERENT DIRECTIONAL LIGHT is selected.
	// (Not when switching to other object types — those don't use shadow settings anyway.)
	static GameObject* lastDirectionalLightSelected = nullptr;
	const bool isDirectionalLight = this->selectedObject && 
		this->selectedObject->getType() == GameObject::PrimitiveType::DIRECTIONAL_LIGHT;

	if (isDirectionalLight && this->selectedObject != lastDirectionalLightSelected)
	{
		// Changed to a different directional light — reset editing state
		lastDirectionalLightSelected = this->selectedObject;
		shadowEditInitialised_       = false;
		shadowLightSlotIndex_        = 0;
	}
	else if (!isDirectionalLight)
	{
		// Non-directional-light object selected — remember this for next time
		lastDirectionalLightSelected = nullptr;
		// Don't reset shadowEditInitialised_ or shadowLightSlotIndex_
		// so they persist if we return to a directional light
	}

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
		this->drawShadowSettingsTab();
		this->drawPhysicsTab();
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

	// ---- Light Direction (directional lights only) ----
	if (type == GameObject::PrimitiveType::DIRECTIONAL_LIGHT)
	{
		labelCell("Direction");
		{
			ImGui::TextDisabled("(Read-only) Light direction in world space:");
			ImGui::Text("X: %.3f", lightDirectionDisplay[0]);
			ImGui::Text("Y: %.3f", lightDirectionDisplay[1]);
			ImGui::Text("Z: %.3f", lightDirectionDisplay[2]);
		}
	}

	ImGui::EndTable();
}

// ─────────────────────────────────────────────────────────────────────────────
// Shadow Settings Tab (directional lights only)
// ─────────────────────────────────────────────────────────────────────────────

void InspectorScreen::drawShadowSettingsTab()
{
	// Only show for directional lights.
	if (!selectedObject) return;
	if (selectedObject->getType() != GameObject::PrimitiveType::DIRECTIONAL_LIGHT) return;

	if (!ImGui::CollapsingHeader("Shadow Settings")) return;

	// ── Fetch current global settings from GameRenderer ───────────────────────
	// GameRenderer is reached via the event system: we read back settings through
	// a Parameters handle round-trip.  However, for the read path we hold a local
	// working copy (shadowLightEdit_) that is initialized once per selection and
	// then kept in sync.
	//
	// To get the current global settings we broadcast a query — but that requires
	// a two-way coupling.  Instead, we keep a self-contained working copy and let
	// the user commit changes; on commit we broadcast ON_SHADOW_SETTINGS_CHANGED
	// with the full merged settings object.

	// ── Initialise working copy once per selection ────────────────────────────
	if (!shadowEditInitialised_ || shadowLightSlotIndex_ != lastInitializedSlot_)
	{
		// Check if we have a cached version of settings for this light + slot combo
		const size_t cacheKey = MakeCacheKey(selectedObject, shadowLightSlotIndex_);
		const auto cacheIt = shadowSettingsCache_.find(cacheKey);
		if (cacheIt != shadowSettingsCache_.end())
		{
			// Restore from cache (what was previously applied for THIS light)
			shadowLightEdit_ = cacheIt->second;
		}
		else
		{
			// No cache — initialize to defaults
			shadowLightEdit_ = Vulkan::Game::ShadowLightSettings{};
		}
		shadowEditInitialised_ = true;
		lastInitializedSlot_ = shadowLightSlotIndex_;
	}

	// ── Light slot selector ───────────────────────────────────────────────────
	ImGui::TextDisabled("Light slot index (0 = first directional light):");
	ImGui::SetNextItemWidth(80.0f);
	ImGui::InputInt("##SlotIdx", &shadowLightSlotIndex_);
	shadowLightSlotIndex_ = std::clamp(
		shadowLightSlotIndex_,
		0,
		static_cast<int>(Vulkan::Game::kMaxShadowLights) - 1);

	ImGui::Separator();

	// ── Resolution override ───────────────────────────────────────────────────
	bool useResOverride = shadowLightEdit_.Resolution.has_value();
	if (ImGui::Checkbox("Override Resolution##SRes", &useResOverride))
	{
		if (useResOverride) shadowLightEdit_.Resolution = 2048u;
		else                shadowLightEdit_.Resolution.reset();
	}
	if (useResOverride)
	{
		ImGui::SameLine();
		int res = static_cast<int>(*shadowLightEdit_.Resolution);
		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::InputInt("##SResVal", &res, 128, 512))
			shadowLightEdit_.Resolution = static_cast<uint32_t>(std::max(64, res));
	}

	// ── Depth bias overrides ──────────────────────────────────────────────────
	ImGui::Spacing();
	bool useBiasOverride = shadowLightEdit_.DepthBiasEnable.has_value();
	if (ImGui::Checkbox("Override Depth Bias##SBias", &useBiasOverride))
	{
		if (useBiasOverride)
		{
			shadowLightEdit_.DepthBiasEnable         = true;
			shadowLightEdit_.DepthBiasConstantFactor = 1.25f;
			shadowLightEdit_.DepthBiasSlopeFactor    = 1.75f;
			shadowLightEdit_.DepthBiasClamp          = 0.0f;
		}
		else
		{
			shadowLightEdit_.DepthBiasEnable.reset();
			shadowLightEdit_.DepthBiasConstantFactor.reset();
			shadowLightEdit_.DepthBiasSlopeFactor.reset();
			shadowLightEdit_.DepthBiasClamp.reset();
		}
	}
	if (useBiasOverride)
	{
		bool biasOn = *shadowLightEdit_.DepthBiasEnable;
		if (ImGui::Checkbox("Bias Enabled##SBiasOn", &biasOn))
			shadowLightEdit_.DepthBiasEnable = biasOn;

		float constF = shadowLightEdit_.DepthBiasConstantFactor.value_or(1.25f);
		ImGui::SetNextItemWidth(180.0f);
		if (ImGui::SliderFloat("Constant Factor##SBiasConst", &constF, 0.0f, 10.0f, "%.3f"))
			shadowLightEdit_.DepthBiasConstantFactor = constF;

		float slopeF = shadowLightEdit_.DepthBiasSlopeFactor.value_or(1.75f);
		ImGui::SetNextItemWidth(180.0f);
		if (ImGui::SliderFloat("Slope Factor##SBiasSlope", &slopeF, 0.0f, 10.0f, "%.3f"))
			shadowLightEdit_.DepthBiasSlopeFactor = slopeF;

		float clampF = shadowLightEdit_.DepthBiasClamp.value_or(0.0f);
		ImGui::SetNextItemWidth(180.0f);
		if (ImGui::SliderFloat("Clamp##SBiasClamp", &clampF, 0.0f, 1.0f, "%.4f"))
			shadowLightEdit_.DepthBiasClamp = clampF;
	}

	// ── Frustum / scene coverage overrides ───────────────────────────────────
	ImGui::Spacing();
	bool useMarginOverride = shadowLightEdit_.SceneMargin.has_value();
	if (ImGui::Checkbox("Override Scene Margin##SMargin", &useMarginOverride))
	{
		if (useMarginOverride) shadowLightEdit_.SceneMargin = 100.0f;
		else                   shadowLightEdit_.SceneMargin.reset();
	}
	if (useMarginOverride)
	{
		float margin = *shadowLightEdit_.SceneMargin;
		ImGui::SetNextItemWidth(180.0f);
		if (ImGui::DragFloat("Scene Margin##SMarginVal", &margin, 1.0f, 0.0f, 2000.0f, "%.1f"))
			shadowLightEdit_.SceneMargin = margin;
	}

	bool useNearOverride = shadowLightEdit_.NearPlane.has_value();
	if (ImGui::Checkbox("Override Near Plane##SNear", &useNearOverride))
	{
		if (useNearOverride) shadowLightEdit_.NearPlane = 0.1f;
		else                 shadowLightEdit_.NearPlane.reset();
	}
	if (useNearOverride)
	{
		float nearP = *shadowLightEdit_.NearPlane;
		ImGui::SetNextItemWidth(180.0f);
		if (ImGui::DragFloat("Near Plane##SNearVal", &nearP, 0.01f, 0.001f, 100.0f, "%.3f"))
			shadowLightEdit_.NearPlane = nearP;
	}

	// ── Apply button ──────────────────────────────────────────────────────────
	ImGui::Spacing();
	ImGui::Separator();
	if (ImGui::Button("Apply Shadow Settings"))
	{
		// Build a heap-allocated ShadowMapSettings so the Parameters handle
		// remains valid when GameRenderer::onTriggeredEvent() reads it.
		// GameRenderer takes a copy and deletes nothing — ownership is ours.
		// We broadcast synchronously so the pointer is valid for the duration.
		static Vulkan::Game::ShadowMapSettings s_pendingSettings{};

		// Grow / resize LightOverrides to cover the selected slot.
		const auto slot = static_cast<size_t>(shadowLightSlotIndex_);
		if (s_pendingSettings.LightOverrides.size() <= slot)
			s_pendingSettings.LightOverrides.resize(slot + 1);

		s_pendingSettings.LightOverrides[slot] = shadowLightEdit_;

		auto params = std::make_shared<Parameters>(EventNames::ON_SHADOW_SETTINGS_CHANGED);
		params->encodeHandle("settings", &s_pendingSettings);
		EventBroadcaster::getInstance()->broadcastEventWithParams(
			EventNames::ON_SHADOW_SETTINGS_CHANGED, params);

		// Cache the applied settings for this light + slot combo
		const size_t cacheKey = MakeCacheKey(selectedObject, shadowLightSlotIndex_);
		shadowSettingsCache_[cacheKey] = shadowLightEdit_;
	}

	ImGui::SameLine();
	if (ImGui::Button("Reset Slot##SReset"))
	{
		shadowLightEdit_     = Vulkan::Game::ShadowLightSettings{};
		shadowEditInitialised_ = true;  // Keep initialized, just reset the values
		const size_t cacheKey = MakeCacheKey(selectedObject, shadowLightSlotIndex_);
		shadowSettingsCache_[cacheKey] = shadowLightEdit_;  // Cache the reset
	}
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
				Animation::getInstance()->SetDuration(duration);
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

		// For directional lights, read the light direction
		if (type == Light::DirectionalLight)
		{
			glm::vec3 dir = light->Properties().LightDir;
			this->lightDirectionDisplay[0] = dir.x;
			this->lightDirectionDisplay[1] = dir.y;
			this->lightDirectionDisplay[2] = dir.z;
		}
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

void InspectorScreen::drawPhysicsTab()
{
	// Only show for physics objects (spheres and cubes)
	if (!selectedObject) return;

	const auto type = selectedObject->getType();
	isPhysicsObject =
		type == GameObject::PrimitiveType::SPHERE ||
		type == GameObject::PrimitiveType::CUBE;

	if (!isPhysicsObject) return;

	if (!ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) return;

	if (!ImGui::BeginTable("PhysicsTable", 2,
		ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody))
		return;

	const float labelWidth = ImGui::CalcTextSize("Active Physics").x;
	ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
	ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);

	// ---- Active Physics Toggle ----
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Active Physics");

		ImGui::TableSetColumnIndex(1);

		if (ImGui::Checkbox("##PhysicsActive", &physicsActivationState))
		{
			// Toggle the physics body activation
			TracerPhysics::GetInstance().ToggleBodyActivation(
				selectedObject,
				physicsActivationState
			);
		}

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
		{
			ImGui::SetTooltip("Enable or disable physics simulation for this object");
		}
	}

	ImGui::EndTable();
}
