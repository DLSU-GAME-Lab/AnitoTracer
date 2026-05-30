#pragma once
#include "AUIScreen.h"
#include "imgui.h"
#include "Vulkan/Vulkan.hpp"
#include <array>
#include <string>

namespace Vulkan::Game { class IBLPrecompute; }
struct UserSettings;

// ─────────────────────────────────────────────────────────────────────────────
/// @brief Debug panel that displays the three IBL textures produced by the
///        IBL compute shaders:
///
///   • Irradiance cubemap  — all 6 faces shown as a cross layout (32×32 each)
///   • Prefiltered env map — face +X across all mip levels (128→1 px)
///   • BRDF integration LUT — full 512×512 displayed at a fixed preview size
///
/// The panel is only visible when the Game Renderer is active and a skybox
/// was loaded (IBLPrecompute != nullptr).  When hidden the ImGui descriptor
/// sets are still alive but not drawn.
// ─────────────────────────────────────────────────────────────────────────────
class IBLDebugScreen : public AUIScreen
{
public:
	IBLDebugScreen();
	~IBLDebugScreen();

	/// @brief Point the panel at the IBL resources owned by GameRenderer.
	///        Call this whenever the GameRenderer (re)creates its IBLPrecompute.
	///        Passing nullptr is safe — the panel will show a "No IBL" message.
	void SetIBL(const Vulkan::Game::IBLPrecompute* ibl);

	/// @brief Provide a read-only view of the current user settings so the
	///        panel can reflect the UseColorIBL / IBLSkyColor override state.
	///        Safe to call with nullptr — the panel will omit the status row.
	void SetUserSettings(const UserSettings* settings);

	/// @brief Must be called once after ImGui_ImplVulkan has been fully
	///        initialized (i.e. after UIManager::initializeUI returns).
	///        Registers all needed ImGui texture descriptors.
	void RegisterDescriptors();

	/// @brief Release all ImGui_ImplVulkan descriptor sets.
	///        Must be called before the Vulkan device is destroyed.
	void ReleaseDescriptors();

	/// @brief Clear all stored descriptor set handles and the registered flag
	///        WITHOUT calling ImGui_ImplVulkan_RemoveTexture.
	///        Use this after UIManager::ReinitializeBackends() has already
	///        destroyed the old descriptor pool, so the handles are stale
	///        and must never be passed back to the driver.
	void InvalidateDescriptors();

private:
	virtual void drawUI() override;

	/// Re-register all descriptors (called automatically inside SetIBL when
	/// the IBL pointer changes after initial registration).
	void RefreshDescriptors();

	const Vulkan::Game::IBLPrecompute* ibl_{ nullptr };
	const UserSettings*                userSettings_{ nullptr };

	bool descriptorsRegistered_{ false };

	// Irradiance: one descriptor per face (VK_IMAGE_VIEW_TYPE_2D, 32×32)
	std::array<VkDescriptorSet, 6> irrDs_{};

	// Prefiltered: face +X across all 7 mip levels
	static constexpr uint32_t kMaxMips = 7;
	std::array<VkDescriptorSet, kMaxMips> preDs_{};

	// BRDF LUT: single 2D texture
	VkDescriptorSet lutDs_{ VK_NULL_HANDLE };

	// UI state
	int  selectedFace_{ 0 };          // 0-5: which cubemap face to preview
	int  selectedMip_{ 0 };           // 0-(kPrefilteredMips-1)
	bool showCrossLayout_{ true };    // irradiance cross vs single face
	float previewSize_{ 96.0f };      // px side length for cubemap face previews

	static constexpr const char* kFaceLabels[6] =
		{ "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
};
