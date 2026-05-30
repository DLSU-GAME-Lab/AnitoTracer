#include "IBLDebugScreen.h"

#include "imgui_impl_vulkan.h"
#include "UserSettings.hpp"
#include "Vulkan/Vk_Game/IBL/IBLPrecompute.hpp"

// Static string array definition
constexpr const char* IBLDebugScreen::kFaceLabels[6];

// ─────────────────────────────────────────────────────────────────────────────

IBLDebugScreen::IBLDebugScreen()
	: AUIScreen("IBL_DEBUG_SCREEN")
{
	irrDs_.fill(VK_NULL_HANDLE);
	preDs_.fill(VK_NULL_HANDLE);
}

IBLDebugScreen::~IBLDebugScreen()
{
	ReleaseDescriptors();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void IBLDebugScreen::SetIBL(const Vulkan::Game::IBLPrecompute* ibl)
{
	if (ibl_ == ibl) return;
	ibl_ = ibl;

	// Re-register descriptors with the new image views.
	if (descriptorsRegistered_)
		RefreshDescriptors();
}

void IBLDebugScreen::SetUserSettings(const UserSettings* settings)
{
	userSettings_ = settings;
}

void IBLDebugScreen::RegisterDescriptors()
{
	if (descriptorsRegistered_)
		ReleaseDescriptors();

	if (!ibl_) return;

	// Irradiance face views
	for (uint32_t f = 0; f < 6; ++f)
	{
		irrDs_[f] = ImGui_ImplVulkan_AddTexture(
			ibl_->IrradianceSampler(),
			ibl_->IrradianceFaceView(f),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	// Prefiltered env map — face +X, every mip level
	const uint32_t mipCount = Vulkan::Game::IBLPrecompute::kPrefilteredMips;
	for (uint32_t m = 0; m < mipCount && m < kMaxMips; ++m)
	{
		preDs_[m] = ImGui_ImplVulkan_AddTexture(
			ibl_->PrefilteredSampler(),
			ibl_->PrefilteredFaceView(m, 0),   // face 0 = +X
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	// BRDF LUT
	lutDs_ = ImGui_ImplVulkan_AddTexture(
		ibl_->BrdfLutSampler(),
		ibl_->BrdfLutView(),
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	descriptorsRegistered_ = true;
}

void IBLDebugScreen::ReleaseDescriptors()
{
	if (!descriptorsRegistered_) return;

	// ImGui_ImplVulkan_RemoveTexture only invalidates the descriptor set.
	// The underlying Vulkan objects (image views / samplers) are owned by IBLPrecompute.
	for (auto& ds : irrDs_)
	{
		if (ds != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(ds);
			ds = VK_NULL_HANDLE;
		}
	}
	for (auto& ds : preDs_)
	{
		if (ds != VK_NULL_HANDLE)
		{
			ImGui_ImplVulkan_RemoveTexture(ds);
			ds = VK_NULL_HANDLE;
		}
	}
	if (lutDs_ != VK_NULL_HANDLE)
	{
		ImGui_ImplVulkan_RemoveTexture(lutDs_);
		lutDs_ = VK_NULL_HANDLE;
	}

	descriptorsRegistered_ = false;
}

void IBLDebugScreen::InvalidateDescriptors()
{
	// Zero out all handles so ReleaseDescriptors() / RegisterDescriptors()
	// cannot accidentally touch VkDescriptorSet values that belong to a
	// pool that has already been destroyed by a backend reinit.
	for (auto& ds : irrDs_) ds = VK_NULL_HANDLE;
	for (auto& ds : preDs_) ds = VK_NULL_HANDLE;
	lutDs_ = VK_NULL_HANDLE;
	descriptorsRegistered_ = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void IBLDebugScreen::RefreshDescriptors()
{
	ReleaseDescriptors();
	RegisterDescriptors();
}

// ─────────────────────────────────────────────────────────────────────────────
// drawUI
// ─────────────────────────────────────────────────────────────────────────────

void IBLDebugScreen::drawUI()
{
	ImGui::SetNextWindowSize(ImVec2(520, 620), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("IBL Debug", &enabled))
	{
		ImGui::End();
		return;
	}

	if (!ibl_ || !descriptorsRegistered_)
	{
		ImGui::TextDisabled("No IBL available.");
		ImGui::TextWrapped("IBL is only computed when the scene has a skybox. "
						   "Switch to Game Renderer mode and load a scene with a skybox.");
		ImGui::End();
		return;
	}

	// ── Settings row ─────────────────────────────────────────────────────────
	ImGui::SliderFloat("Preview size", &previewSize_, 32.0f, 256.0f, "%.0f px");
	ImGui::Separator();

	// ── UseColorIBL override notice ───────────────────────────────────────────
	// When active, the shader substitutes IBLSkyColor for both the irradiance
	// cubemap and the prefiltered env map samples.  The textures displayed below
	// are still the pre-baked HDR convolutions from the skybox — they are NOT
	// what is contributing to the rendered image in this mode.
	if (userSettings_ && userSettings_->Game.EnableIBL && userSettings_->Game.UseColorIBL)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.1f, 1.0f)); // amber warning
		ImGui::TextWrapped(
			"[!] Use Color in IBL is ON — the shader is using IBLSkyColor "
			"instead of the cubemaps below. The previews show the pre-baked "
			"skybox convolution, which is NOT driving the current output.");
		ImGui::PopStyleColor();

		// Show the active flat colour as a read-only swatch.
		const glm::vec3& c = userSettings_->Game.IBLSkyColor;
		ImGui::ColorButton("##iblSkyColor",
			ImVec4(c.r, c.g, c.b, 1.0f),
			ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
			ImVec2(20.0f, 20.0f));
		ImGui::SameLine();
		ImGui::Text("Active IBL Sky Color  (%.3f, %.3f, %.3f)", c.r, c.g, c.b);
		ImGui::Separator();
	}

	// ══════════════════════════════════════════════════════════════════════════
	// 1. Irradiance cubemap
	// ══════════════════════════════════════════════════════════════════════════
	if (ImGui::CollapsingHeader("Irradiance Cubemap  (32x32, RGBA16F)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Cross layout (all 6 faces)", &showCrossLayout_);

		const float sz = previewSize_;

		if (showCrossLayout_)
		{
			// Cubemap cross layout (standard orientation):
			//         [+Y]
			//  [-X]  [+Z]  [+X]  [-Z]
			//         [-Y]
			// Face order stored in irradianceFaceViews_: +X=0, -X=1, +Y=2, -Y=3, +Z=4, -Z=5

			const float colW = sz + 4.0f;
			const ImVec2 cursor = ImGui::GetCursorScreenPos();

			// Row 0: +Y at column 1
			ImGui::SetCursorScreenPos(ImVec2(cursor.x + colW, cursor.y));
			ImGui::Image(reinterpret_cast<ImTextureID>(irrDs_[2]), ImVec2(sz, sz));
			ImGui::SameLine(); ImGui::SetCursorScreenPos(ImVec2(cursor.x + colW, cursor.y + sz + 4.0f));

			// Row 1: -X, +Z, +X, -Z
			ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + sz + 4.0f));
			ImGui::Image(reinterpret_cast<ImTextureID>(irrDs_[1]), ImVec2(sz, sz)); // -X
			ImGui::SameLine();
			ImGui::Image(reinterpret_cast<ImTextureID>(irrDs_[4]), ImVec2(sz, sz)); // +Z
			ImGui::SameLine();
			ImGui::Image(reinterpret_cast<ImTextureID>(irrDs_[0]), ImVec2(sz, sz)); // +X
			ImGui::SameLine();
			ImGui::Image(reinterpret_cast<ImTextureID>(irrDs_[5]), ImVec2(sz, sz)); // -Z

			// Row 2: -Y at column 1
			ImGui::SetCursorScreenPos(ImVec2(cursor.x + colW, cursor.y + (sz + 4.0f) * 2.0f));
			ImGui::Image(reinterpret_cast<ImTextureID>(irrDs_[3]), ImVec2(sz, sz)); // -Y

			// Advance cursor past all three rows
			ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y + (sz + 4.0f) * 3.0f));
		}
		else
		{
			// Single-face picker
			ImGui::Text("Face:");
			for (int f = 0; f < 6; ++f)
			{
				if (f > 0) ImGui::SameLine();
				if (ImGui::Button(kFaceLabels[f]))
					selectedFace_ = f;
			}
			ImGui::Image(reinterpret_cast<ImTextureID>(irrDs_[selectedFace_]), ImVec2(sz, sz));
			ImGui::SameLine();
			ImGui::TextDisabled("Face %s", kFaceLabels[selectedFace_]);
		}
	}

	// ══════════════════════════════════════════════════════════════════════════
	// 2. Prefiltered env map
	// ══════════════════════════════════════════════════════════════════════════
	if (ImGui::CollapsingHeader("Prefiltered Env Map  (128x128, RGBA16F, 7 mips)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TextDisabled("Face +X across roughness mip levels");
		ImGui::Spacing();

		const uint32_t mipCount = Vulkan::Game::IBLPrecompute::kPrefilteredMips;

		// Show mip strip: each mip at half the previous size, min 8 px
		float mipSz = previewSize_;
		for (uint32_t m = 0; m < mipCount && m < kMaxMips; ++m)
		{
			if (m > 0) ImGui::SameLine(0.0f, 4.0f);
			const float drawSz = std::max(mipSz, 8.0f);
			ImGui::BeginGroup();
			ImGui::Image(reinterpret_cast<ImTextureID>(preDs_[m]),
						 ImVec2(drawSz, drawSz));
			ImGui::Text("M%u", m);
			ImGui::EndGroup();
			mipSz *= 0.5f;
		}

		ImGui::Spacing();
		// Single mip selector for a larger preview
		ImGui::SliderInt("Mip", &selectedMip_, 0, static_cast<int>(mipCount) - 1);
		const float roughness = static_cast<float>(selectedMip_) / static_cast<float>(mipCount - 1);
		ImGui::TextDisabled("roughness = %.2f", roughness);
		ImGui::Image(reinterpret_cast<ImTextureID>(preDs_[selectedMip_]),
					 ImVec2(previewSize_ * 2.0f, previewSize_ * 2.0f));
	}

	// ══════════════════════════════════════════════════════════════════════════
	// 3. BRDF integration LUT
	// ══════════════════════════════════════════════════════════════════════════
	if (ImGui::CollapsingHeader("BRDF Integration LUT  (512x512, RG16F)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TextDisabled("R = F0 scale,  G = F0 bias.  X-axis = NdotV,  Y-axis = roughness.");
		const float lutPreview = std::min(previewSize_ * 2.0f, 256.0f);
		ImGui::Image(reinterpret_cast<ImTextureID>(lutDs_), ImVec2(lutPreview, lutPreview));
	}

	ImGui::End();
}
