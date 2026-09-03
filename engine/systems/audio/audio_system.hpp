#pragma once

#include <cstdint>
#include "engine/core/ecs.hpp"
#include "engine/events/game.hpp"
#include "engine/events/debug.hpp"

namespace tmt {

class AudioSystem : public ISystem {
   public:
    constexpr std::string get_name() override { return "audio system"; }

    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_end() override;
};

}  // namespace tmt
