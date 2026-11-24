#include "AnimationComponent.hpp"

#include "AssetDatabase.hpp"
#include "HierarchyObject.hpp"
#include "Components/Transform.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

namespace {
    HierarchyObject::Ref FindByGuid(HierarchyObject::Ref root, const gbe::GUID& guid) {
        HierarchyObject* object = root.GetPtr();
        if (!object || guid == gbe::GUID::Empty()) return {};
        if (object->GetGUID() == guid) return root;
        for (const auto& child : object->GetChildren()) {
            if (HierarchyObject::Ref found = FindByGuid(child.get(), guid)) return found;
        }
        return {};
    }

    HierarchyObject::Ref ResolveTrack(
        HierarchyObject::Ref root, const AnimationTrack& track) {
        if (HierarchyObject::Ref target =
                AnimationComponent::ResolveRelativeObject(root, track.relativePath))
            return target;
        const gbe::GUID guid = gbe::GUID::FromString(track.objectGuid);
        return FindByGuid(root, guid);
    }
}

void AnimationComponent::OnStart() {
    if (m_playOnAwake) Play();
}

void AnimationComponent::OnUpdate(float deltaTime) {
    if (m_state != PlaybackState::Playing) return;
    AnimationClip* clip = m_clip.Get();
    if (!clip) {
        Stop();
        return;
    }

    const float duration = clip->GetDuration();
    if (duration <= 0.0f) {
        Stop();
        return;
    }

    m_time += deltaTime * m_speed;
    if (m_loop && m_time >= duration) {
        m_time = std::fmod(m_time, duration);
    } else if (m_time >= duration) {
        m_time = duration;
        m_state = PlaybackState::Stopped;
    } else if (m_time < 0.0f) {
        m_time = m_loop ? duration - std::fmod(-m_time, duration) : 0.0f;
    }
    Evaluate();
}

void AnimationComponent::SetAnimationClip(gbe::AssetRef<AnimationClip> clip) {
    Stop();
    m_clip = clip;
    if (AnimationClip* loaded = m_clip.Get()) m_loop = loaded->IsLooping();
}

void AnimationComponent::SetClip(const std::string& projectPath) {
    SetClipPath(projectPath, true);
}

bool AnimationComponent::SetClipPath(const std::string& projectPath, bool logError) {
    if (projectPath.empty()) {
        SetAnimationClip({});
        return true;
    }

    auto* asset = gbe::AssetDatabase::GetAssetByPath(std::filesystem::path(projectPath));
    auto* clip = dynamic_cast<AnimationClip*>(asset);
    if (!clip) {
        if (logError) {
            std::cerr << "[Animation] Cannot select clip '" << projectPath
                      << "': asset was not found or is not an animation clip.\n";
        }
        return false;
    }

    SetAnimationClip(gbe::AssetRef<AnimationClip>(clip));
    return true;
}

std::string AnimationComponent::GetClip() const {
    AnimationClip* clip = m_clip.Get();
    if (!clip) return {};
    return gbe::AssetDatabase::LocalizePath(clip->GetPath()).generic_string();
}

void AnimationComponent::Play() {
    if (!m_clip.Get()) return;
    m_time = 0.0f;
    m_state = PlaybackState::Playing;
    Evaluate();
}

void AnimationComponent::Pause() {
    if (m_state == PlaybackState::Playing) m_state = PlaybackState::Paused;
}

void AnimationComponent::Resume() {
    if (m_state == PlaybackState::Paused && m_clip.Get()) m_state = PlaybackState::Playing;
}

void AnimationComponent::Stop() {
    m_state = PlaybackState::Stopped;
    m_time = 0.0f;
}

void AnimationComponent::SetTime(float time) {
    const float duration = GetDuration();
    if (duration <= 0.0f) {
        m_time = 0.0f;
    } else if (m_loop) {
        m_time = std::fmod(std::max(0.0f, time), duration);
    } else {
        m_time = std::clamp(time, 0.0f, duration);
    }
    Evaluate();
}

float AnimationComponent::GetDuration() const {
    AnimationClip* clip = m_clip.Get();
    return clip ? std::max(0.0f, clip->GetDuration()) : 0.0f;
}

void AnimationComponent::SetSpeed(float speed) {
    m_speed = std::max(0.0f, speed);
}

HierarchyObject::Ref AnimationComponent::ResolveRelativeObject(
    HierarchyObject::Ref root, const std::string& relativePath) {
    HierarchyObject* target = root.GetPtr();
    if (!target || relativePath.empty() || relativePath == ".") return target;

    std::istringstream stream(relativePath);
    std::string segment;
    while (target && std::getline(stream, segment, '/')) {
        if (segment.empty() || segment == ".") continue;
        HierarchyObject* next = nullptr;
        for (const auto& child : target->GetChildren()) {
            if (child && child->GetName() == segment) {
                next = child.get();
                break;
            }
        }
        target = next;
    }
    return target;
}

void AnimationComponent::EvaluateClip(
    const AnimationClipData& clip, float time, HierarchyObject::Ref root) {
    for (const AnimationTrack& track : clip.tracks) {
        HierarchyObject* object = ResolveTrack(root, track).GetPtr();
        Transform* transform = object ? object->GetTransform() : nullptr;
        if (!transform) continue;

        glm::vec3 position = transform->GetLocalPosition();
        glm::vec3 rotation = transform->GetEulerAnglesDegrees();
        glm::vec3 scale = transform->GetLocalScale();
        for (int channel = 0; channel < 3; ++channel) {
            track.position[channel].Evaluate(time, position[channel], position[channel]);
            track.rotation[channel].Evaluate(time, rotation[channel], rotation[channel]);
            track.scale[channel].Evaluate(time, scale[channel], scale[channel]);
        }
        transform->SetPosition(position);
        transform->SetEulerAnglesDegrees(rotation);
        transform->SetScale(scale);
        transform->SyncPhysics();
    }
}

std::string AnimationComponent::GetRelativePath(
    HierarchyObject::Ref root, HierarchyObject::Ref target) {
    HierarchyObject* rootObject = root.GetPtr();
    HierarchyObject* targetObject = target.GetPtr();
    if (!rootObject || !targetObject) return {};
    if (rootObject == targetObject) return ".";

    std::vector<std::string> reversed;
    HierarchyObject::Ref current = target;
    while (current && current.GetPtr() != rootObject) {
        reversed.push_back(current.GetPtr()->GetName());
        current = current.GetPtr()->GetParent();
    }
    if (!current) return {};

    std::string result;
    for (auto item = reversed.rbegin(); item != reversed.rend(); ++item) {
        if (!result.empty()) result += '/';
        result += *item;
    }
    return result;
}

void AnimationComponent::Evaluate() {
    AnimationClip* clip = m_clip.Get();
    if (!clip) return;
    EvaluateClip(clip->GetData(), m_time, GetOwner());
}

GBE_REGISTER_METHOD(AnimationComponent, Play);
GBE_REGISTER_METHOD(AnimationComponent, Pause);
GBE_REGISTER_METHOD(AnimationComponent, Resume);
GBE_REGISTER_METHOD(AnimationComponent, Stop);
