#pragma once

#include "Common.hpp"

namespace anito {
class AudioSource {
public:
    ObjectId id = 0;

    bool valid() const { return detail::H().audio.HasAudioSource(id); }
    inline GameObject gameObject() const;

    bool hasClip() const { return detail::H().audio.HasClip(id); }
    void play() const { detail::H().audio.Play(id); }
    void stop() const { detail::H().audio.Stop(id); }
    // Plays a fire-and-forget copy that keeps going after the owner is destroyed.
    void playDetached() const { detail::H().audio.PlayDetached(id); }
    float volume() const { return detail::H().audio.GetVolume(id); }
    void setVolume(float v) const { detail::H().audio.SetVolume(id, v); }
    bool loop() const { return detail::H().audio.GetLoop(id); }
    void setLoop(bool v) const { detail::H().audio.SetLoop(id, v); }
    float pitch() const { return detail::H().audio.GetPitch(id); }
    void setPitch(float v) const { detail::H().audio.SetPitch(id, v); }
    bool spatialized() const { return detail::H().audio.GetSpatialized(id); }
    void setSpatialized(bool v) const { detail::H().audio.SetSpatialized(id, v); }
    void setDistance(float minDistance, float maxDistance) const { detail::H().audio.SetDistance(id, minDistance, maxDistance); }

    bool operator==(const AudioSource& o) const { return id == o.id; }
    bool operator!=(const AudioSource& o) const { return id != o.id; }
};
} // namespace anito
