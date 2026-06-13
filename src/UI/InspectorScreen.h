#pragma once
#include "AUIScreen.h"
#include "Engine/LightSystem/Light.h"
#include "From-GDGRAP2/GameObject.h"
#include "Engine/AnimationSystem/Animation.h"
#include "Vulkan/Vk_Game/ShadowMapSettings.hpp"
#include <algorithm>
#include <optional>
#include <unordered_map>

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
	void drawShadowSettingsTab();   // per-light shadow overrides (directional lights only)
	void drawPhysicsTab();          // physics activation toggle
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
	float lightDirectionDisplay[3] = { 0.0f, -1.0f, 0.0f };  // Display-only for directional lights

	bool isUniformScalingEnabled = false;

	bool isPhysicsObject = false;  // Track if selected object is a physics object (sphere/cube)
	bool physicsActivationState = true;  // Current activation state of physics body

	GameObject* selectedObject = nullptr;
	const String DEFAULT_MATERIAL = "None";
	String materialPath = DEFAULT_MATERIAL;
	String materialName = DEFAULT_MATERIAL;
	Texture* materialDisplay;

	float lightIntensityMultiplier = 500000.0f;

	// ── Per-light shadow settings display state ────────────────────────────────
	/// Index of the directional light slot whose overrides are being shown.
	/// Reset whenever selection changes.
	int shadowLightSlotIndex_{ 0 };

	/// Working copy of the ShadowLightSettings that is currently being edited.
	/// Pushed back to GameRenderer on change.
	Vulkan::Game::ShadowLightSettings shadowLightEdit_{};

	/// Whether the working copy has been initialised for the current selection.
	bool shadowEditInitialised_{ false };

	/// Tracks which slot was last initialized to force re-init if slot changes
	int lastInitializedSlot_{ -1 };

	/// Cache of the last-applied shadow settings per (light pointer, slot) pair.
	/// Each directional light can have different overrides per slot.
	/// Key: hash of (light pointer address + slot index)
	std::unordered_map<size_t, Vulkan::Game::ShadowLightSettings> shadowSettingsCache_;

	/// Helper to create a unique cache key from light pointer and slot index
	static size_t MakeCacheKey(GameObject* light, int slot)
	{
		// Combine pointer hash and slot into a single key
		size_t h1 = std::hash<GameObject*>{}(light);
		size_t h2 = std::hash<int>{}(slot);
		return h1 ^ (h2 << 1);
	}

};