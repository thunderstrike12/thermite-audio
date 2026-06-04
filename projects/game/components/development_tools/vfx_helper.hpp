#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/emitter.hpp"

namespace game {

class GameVFXHelper : public tmt::GameComponent<GameVFXHelper> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Game VFX Helper"; }

    void start() override {}
    void update(const tmt::FrameData& time) override {}
    void end() override {}

    static void burst_vfx_emitter(tmt::Entity emitter_entity);
    static void activate_vfx_emitter(tmt::Entity emitter_entity);
    static void deactivate_vfx_emitter(tmt::Entity emitter_entity);

   private:
};

}  // namespace game