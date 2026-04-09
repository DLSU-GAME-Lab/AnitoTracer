#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <algorithm>

namespace gbe {
    // A single point on the timeline
    struct Keyframe {
        int frame;
        float value;

        bool operator<(const Keyframe& other) const { return frame < other.frame; }
    };

    // A track holds the timeline for a single float property (e.g., "Transform X")
    struct AnimationTrack {
        std::vector<Keyframe> keys;
    };

    // All animated properties for a single Object
    struct ObjectAnimation {
        // Map of Property Name -> Track
        // Example: "Position X" -> Track, "Color R" -> Track
        std::unordered_map<std::string, AnimationTrack> tracks;
    };

    // The entire animation file/data
    struct AnimationClip {
        std::string name = "New Animation";
        int maxFrames = 600;
        int fps = 60;

        // Map of Object ID -> Object Animation Data
        std::unordered_map<uint32_t, ObjectAnimation> objectAnimations;
    };
}