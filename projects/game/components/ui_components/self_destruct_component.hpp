#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"

namespace game {

class SelfDestructComponent : public tmt::GameComponent<SelfDestructComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "ButtonSelfDestruct"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void self_destruct();
    tmt::Entity player_entity = entt::null;

   private:
};

}  // namespace game

TMT_OBJECT(game::SelfDestructComponent, (player_entity));
