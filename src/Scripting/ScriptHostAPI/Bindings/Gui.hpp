#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::gui {
anito::GuiAPI GetAPI();
void WorldToScreen(anito::Vec3, anito::Vec3*);
void FillRect(anito::Vec3, anito::Vec3, anito::Vec3, float);
void DrawRect(anito::Vec3, anito::Vec3, anito::Vec3, float, float);
}
