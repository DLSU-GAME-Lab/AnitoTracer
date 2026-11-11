#include "Bindings/Gui.hpp"
#include "Common.hpp"

#include "GUIManager.hpp"

namespace ScriptHost::Bindings::gui {
using detail::ToGlm;
using detail::FromGlm;

void WorldToScreen(anito::Vec3 world, anito::Vec3* out) {
    *out = FromGlm(Diligent::GUIManager::GetInstance().ScriptWorldToScreen(ToGlm(world)));
}

void FillRect(anito::Vec3 min, anito::Vec3 max, anito::Vec3 rgb, float alpha) {
    Diligent::GUIManager::GetInstance().ScriptFillRect(ToGlm(min), ToGlm(max), ToGlm(rgb), alpha);
}

void DrawRect(anito::Vec3 min, anito::Vec3 max, anito::Vec3 rgb, float alpha, float thickness) {
    Diligent::GUIManager::GetInstance().ScriptDrawRect(
        ToGlm(min), ToGlm(max), ToGlm(rgb), alpha, thickness);
}

anito::GuiAPI GetAPI() {
    return {WorldToScreen, FillRect, DrawRect};
}
} // namespace ScriptHost::Bindings::gui
