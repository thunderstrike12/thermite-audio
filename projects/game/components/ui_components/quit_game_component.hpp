#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"

namespace game {

class QuitGameComponent : public tmt::GameComponent<QuitGameComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "ButtonQuitGame"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void quit_game();

   private:
};

}  // namespace game
