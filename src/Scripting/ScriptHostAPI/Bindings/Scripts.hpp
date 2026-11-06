#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::scripts {
anito::ScriptAPI GetAPI();
bool Has(anito::ObjectId, const char*);
void Add(anito::ObjectId, const char*);
bool Call(anito::ObjectId, const char*, const char*, const anito::Variant*, uint32_t, anito::Variant*);
void SendMessage(anito::ObjectId, const char*, const anito::Variant*, uint32_t);
}
