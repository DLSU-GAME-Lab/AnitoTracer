#pragma once

#include <imgui.h>
#include "InputSystem.hpp"

// Maps Dear ImGui keys to your custom gbe::Key enum
inline gbe::Key MapImGuiKeyToGbeKey(ImGuiKey key) {
	switch (key) {
	case ImGuiKey_A:             return gbe::Key::A;
	case ImGuiKey_B:             return gbe::Key::B;
	case ImGuiKey_C:             return gbe::Key::C;
	case ImGuiKey_D:             return gbe::Key::D;
	case ImGuiKey_E:             return gbe::Key::E;
	case ImGuiKey_F:             return gbe::Key::F;
	case ImGuiKey_G:             return gbe::Key::G;
	case ImGuiKey_H:             return gbe::Key::H;
	case ImGuiKey_I:             return gbe::Key::I;
	case ImGuiKey_J:             return gbe::Key::J;
	case ImGuiKey_K:             return gbe::Key::K;
	case ImGuiKey_L:             return gbe::Key::L;
	case ImGuiKey_M:             return gbe::Key::M;
	case ImGuiKey_N:             return gbe::Key::N;
	case ImGuiKey_O:             return gbe::Key::O;
	case ImGuiKey_P:             return gbe::Key::P;
	case ImGuiKey_Q:             return gbe::Key::Q;
	case ImGuiKey_R:             return gbe::Key::R;
	case ImGuiKey_S:             return gbe::Key::S;
	case ImGuiKey_T:             return gbe::Key::T;
	case ImGuiKey_U:             return gbe::Key::U;
	case ImGuiKey_V:             return gbe::Key::V;
	case ImGuiKey_W:             return gbe::Key::W;
	case ImGuiKey_X:             return gbe::Key::X;
	case ImGuiKey_Y:             return gbe::Key::Y;
	case ImGuiKey_Z:             return gbe::Key::Z;
	case ImGuiKey_Space:         return gbe::Key::Space;
	case ImGuiKey_Enter:         return gbe::Key::Enter;
	case ImGuiKey_Escape:        return gbe::Key::Escape;
	case ImGuiKey_LeftArrow:     return gbe::Key::Left;
	case ImGuiKey_RightArrow:    return gbe::Key::Right;
	case ImGuiKey_UpArrow:       return gbe::Key::Up;
	case ImGuiKey_DownArrow:     return gbe::Key::Down;

		// Mouse button keys (ImGui standard keys include mouse buttons)
	case ImGuiKey_MouseLeft:     return gbe::Key::MouseLeft;
	case ImGuiKey_MouseRight:    return gbe::Key::MouseRight;
	case ImGuiKey_MouseMiddle:   return gbe::Key::MouseMiddle;

	default: return gbe::Key::COUNT;
	}
}

// Ingests raw inputs from Dear ImGui into gbe::InputSystem
inline void ForwardImGuiInputToSystem() {
	ImGuiIO& io = ImGui::GetIO();

	// 1. Pass modifier states
	gbe::InputSystem::SetRawModifierState(gbe::KeyModifier::Shift, io.KeyShift);
	gbe::InputSystem::SetRawModifierState(gbe::KeyModifier::Ctrl, io.KeyCtrl);
	gbe::InputSystem::SetRawModifierState(gbe::KeyModifier::Alt, io.KeyAlt);

	// 2. Pass mouse position and delta via native structs
	gbe::InputSystem::SetMouseDelta({ io.MouseDelta.x, io.MouseDelta.y });
	gbe::InputSystem::SetMousePosition({ io.MousePos.x, io.MousePos.y });

	// 3. Iterate through named keys and update raw key states
	for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; ++i) {
		ImGuiKey imguiKey = static_cast<ImGuiKey>(i);
		gbe::Key engineKey = MapImGuiKeyToGbeKey(imguiKey);

		if (engineKey != gbe::Key::COUNT) {
			bool isDown = ImGui::IsKeyDown(imguiKey);
			gbe::InputSystem::SetRawKeyState(engineKey, isDown);
		}
	}
}