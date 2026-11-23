#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::object {
anito::ObjectAPI GetAPI();
bool IsValid(anito::ObjectId);
const char* GetName(anito::ObjectId);
void SetName(anito::ObjectId, const char*);
anito::ObjectId GetParent(anito::ObjectId);
anito::ObjectId GetRoot(anito::ObjectId);
void SetParent(anito::ObjectId, anito::ObjectId);
int32_t GetChildCount(anito::ObjectId);
anito::ObjectId GetChild(anito::ObjectId, int32_t);
anito::ObjectId FindChild(anito::ObjectId, const char*);
int32_t GetComponentCount(anito::ObjectId);
const char* GetComponentName(anito::ObjectId, int32_t);
bool HasComponent(anito::ObjectId, const char*);
void AddRigidbody(anito::ObjectId, float);
void AddCollider(anito::ObjectId, int32_t, anito::Vec3);
void Destroy(anito::ObjectId);
}
