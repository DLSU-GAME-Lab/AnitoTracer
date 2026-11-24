#pragma once

#include "Components/ComponentBase.hpp"
#include "HierarchyObject.hpp"
#include "AssetRef.hpp"
#include "ScriptApiRegistry.hpp"
#include "Types/AnimationClip.hpp"
#include "Types/UpdateTrigger.hpp"

#include <string>
#include <vector>

class AnimationComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    AnimationComponent(gbe::AssetRef<AnimationClip> clip = {},
        gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
        : ComponentBase("AnimationComponent", owner), m_clip(clip) {}

    void OnStart() override;
    void OnUpdate(float deltaTime) override;

    void SetAnimationClip(gbe::AssetRef<AnimationClip> clip);
    gbe::AssetRef<AnimationClip> GetAnimationClip() const { return m_clip; }
    gbe::AssetRef<AnimationClip>& GetAnimationClipRef() { return m_clip; }

    void SetClip(const std::string& projectPath);
    std::string GetClip() const;
    bool HasAnimationClip() const { return m_clip.IsValid(); }

    void Play();
    void Pause();
    void Resume();
    void Stop();

    bool IsPlaying() const { return m_state == PlaybackState::Playing; }
    bool IsPaused() const { return m_state == PlaybackState::Paused; }
    float GetTime() const { return m_time; }
    void SetTime(float time);
    float GetDuration() const;

    float GetSpeed() const { return m_speed; }
    void SetSpeed(float speed);
    bool GetLoop() const { return m_loop; }
    void SetLoop(bool loop) { m_loop = loop; }
    bool GetPlayOnAwake() const { return m_playOnAwake; }
    void SetPlayOnAwake(bool playOnAwake) { m_playOnAwake = playOnAwake; }

    static HierarchyObject::Ref ResolveRelativeObject(
        HierarchyObject::Ref root, const std::string& relativePath);
    static void EvaluateClip(const AnimationClipData& clip, float time, HierarchyObject::Ref root);
    static std::string GetRelativePath(HierarchyObject::Ref root, HierarchyObject::Ref target);

private:
    enum class PlaybackState {
        Stopped,
        Playing,
        Paused
    };

    void Evaluate();
    bool SetClipPath(const std::string& projectPath, bool logError);

    gbe::AssetRef<AnimationClip> m_clip;
    GBE_SERIALIZE_FIELD(m_clip);

    bool m_playOnAwake = false;
    GBE_SERIALIZE_FIELD(m_playOnAwake);

    bool m_loop = false;
    GBE_SERIALIZE_FIELD(m_loop);

    float m_speed = 1.0f;
    GBE_SERIALIZE_FIELD(m_speed);

    float m_time = 0.0f;
    PlaybackState m_state = PlaybackState::Stopped;
    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(AnimationComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(AnimationComponent, ComponentBase);

ANITO_SCRIPT_TYPE(AnimationComponent, "animation", "anito::Animation", true, "animation");
ANITO_SCRIPT_PROPERTY_RO(AnimationComponent, "animation", "hasClip", HasAnimationClip);
ANITO_SCRIPT_PROPERTY(AnimationComponent, "animation", "clip", GetClip, SetClip);
ANITO_SCRIPT_PROPERTY_RO(AnimationComponent, "animation", "isPlaying", IsPlaying);
ANITO_SCRIPT_PROPERTY_RO(AnimationComponent, "animation", "duration", GetDuration);
ANITO_SCRIPT_PROPERTY(AnimationComponent, "animation", "time", GetTime, SetTime);
ANITO_SCRIPT_PROPERTY(AnimationComponent, "animation", "speed", GetSpeed, SetSpeed);
ANITO_SCRIPT_PROPERTY(AnimationComponent, "animation", "loop", GetLoop, SetLoop);
ANITO_SCRIPT_METHOD(AnimationComponent, "animation", "play", Play);
ANITO_SCRIPT_METHOD(AnimationComponent, "animation", "pause", Pause);
ANITO_SCRIPT_METHOD(AnimationComponent, "animation", "resume", Resume);
ANITO_SCRIPT_METHOD(AnimationComponent, "animation", "stop", Stop);
