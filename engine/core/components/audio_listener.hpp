#pragma once

#include "engine/core/reflection.hpp"

namespace tmt {

class AudioListener {
   public:
    [[nodiscard]] int get_listener_index() const { return listener_index; }

    // Set the weight of the listener [0.0f-1.0f].
    void set_weight(float weight);
    [[nodiscard]] float get_weight() const;

   private:
    BEFRIEND_VISITABLE()
    friend class Audio;

    mutable float cached_weight { 1.0f };
    int listener_index { -1 };  // Index used internally in FMOD, doesn't need to be serialized/reflected.
};

}  // namespace tmt

TMT_COMPONENT(tmt::AudioListener, "Audio Listener", (cached_weight));