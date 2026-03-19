#pragma once
#include <cstdint>

namespace game {

struct LayerMask {
    uint32_t bits = 0xFFFFFFFF;

    void set(uint32_t layer, bool enabled) {
        if (enabled)
            bits |= (1u << layer);
        else
            bits &= ~(1u << layer);
    }

    bool test(uint32_t layer) const { return (bits >> layer) & 1u; }

    operator uint32_t() const { return bits; }
};

}  // namespace game
TMT_OBJECT(game::LayerMask, (bits));
