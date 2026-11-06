#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::audio {
anito::AudioAPI GetAPI();
bool HasAudioSource(anito::ObjectId);
bool HasClip(anito::ObjectId);
void Play(anito::ObjectId);
void Stop(anito::ObjectId);
void PlayDetached(anito::ObjectId);
float GetVolume(anito::ObjectId);
void SetVolume(anito::ObjectId, float);
bool GetLoop(anito::ObjectId);
void SetLoop(anito::ObjectId, bool);
float GetPitch(anito::ObjectId);
void SetPitch(anito::ObjectId, float);
bool GetSpatialized(anito::ObjectId);
void SetSpatialized(anito::ObjectId, bool);
void SetDistance(anito::ObjectId, float, float);
}
