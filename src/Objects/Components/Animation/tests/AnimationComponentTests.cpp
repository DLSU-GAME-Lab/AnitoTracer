#include "Components/Animation/AnimationComponent.hpp"
#include "Components/Transform.hpp"
#include "HierarchyManager.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
    void Check(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    bool Near(float left, float right, float epsilon = 0.0001f) {
        return std::abs(left - right) <= epsilon;
    }
}

int main() {
    const auto rootPath = std::filesystem::temp_directory_path() /
        ("anito-animation-tests-" + gbe::GUID::Generate().ToString());
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            HierarchyManager::GetInstance().Clear();
            gbe::AssetDatabase::Clear();
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{rootPath};

    try {
        std::filesystem::create_directories(rootPath);
        const auto clipPath = rootPath / "Move.anim";
        AnimationClip source;
        AnimationClipData& sourceData = source.GetData();
        sourceData.name = "Move";
        sourceData.duration = 2.0f;
        sourceData.framesPerSecond = 24.0f;
        AnimationTrack track;
        track.relativePath = "Child";
        track.position[0].keys = {{0.0f, 0.0f, AnimationInterpolation::Linear},
                                  {2.0f, 4.0f, AnimationInterpolation::Linear}};
        track.position[1].keys = {{0.0f, 1.0f, AnimationInterpolation::Step},
                                  {1.0f, 3.0f, AnimationInterpolation::Step}};
        sourceData.tracks.push_back(track);
        Check(source.SaveToFile(clipPath), "clip save failed");

        AnimationClip loaded;
        Check(loaded.LoadFromFile(clipPath), "clip load failed");
        Check(loaded.GetData().tracks.size() == 1, "track did not round-trip");
        float value = -1.0f;
        Check(loaded.GetData().tracks[0].position[0].Evaluate(1.0f, 0.0f, value) &&
              Near(value, 2.0f), "linear interpolation failed");
        Check(loaded.GetData().tracks[0].position[1].Evaluate(0.5f, 0.0f, value) &&
              Near(value, 1.0f), "step interpolation failed");

        loaded.SetPath(clipPath);
        const gbe::GUID clipGuid = gbe::GUID::Generate();
        Check(gbe::AssetDatabase::RegisterAsset(&loaded, clipGuid) == clipGuid,
            "clip registration failed");

        HierarchyObject::Ref root = HierarchyManager::GetInstance().CreateNewEmpty("Root");
        root.GetPtr()->AddComponent(std::make_unique<Transform>());
        HierarchyObject::Ref child = HierarchyManager::GetInstance().CreateNewEmpty("Child");
        child.GetPtr()->AddComponent(std::make_unique<Transform>());
        Check(HierarchyManager::GetInstance().ReparentObject(child, root), "child reparent failed");

        auto component = std::make_unique<AnimationComponent>(
            gbe::AssetRef<AnimationClip>(&loaded));
        AnimationComponent* animation = component.get();
        root.GetPtr()->AddComponent(std::move(component));
        animation->Play();
        animation->OnUpdate(0.5f);
        Check(Near(child.GetPtr()->GetTransform()->GetLocalPosition().x, 1.0f),
            "runtime animation did not evaluate child transform");
        animation->OnUpdate(2.0f);
        Check(!animation->IsPlaying(), "non-looping animation did not stop");
        Check(Near(animation->GetTime(), 2.0f), "animation did not clamp to duration");

        AnimationTrack& renamedTrack = loaded.GetData().tracks[0];
        renamedTrack.relativePath = "RenamedChild";
        renamedTrack.objectGuid = child.GetPtr()->GetGUID().ToString();
        animation->SetTime(1.5f);
        Check(Near(child.GetPtr()->GetTransform()->GetLocalPosition().x, 3.0f),
            "GUID fallback did not recover a renamed animation target");

        animation->SetLoop(true);
        animation->Play();
        animation->OnUpdate(2.5f);
        Check(animation->IsPlaying() && Near(animation->GetTime(), 0.5f),
            "looping animation did not wrap");

        const std::string resolved = AnimationComponent::GetRelativePath(root, child);
        Check(resolved == "Child", "relative hierarchy path is incorrect");
        Check(AnimationComponent::ResolveRelativeObject(root, resolved).GetPtr() == child.GetPtr(),
            "relative hierarchy path did not resolve");
    } catch (const std::exception& error) {
        std::cerr << "Animation test failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
