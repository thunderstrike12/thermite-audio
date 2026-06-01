#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"

namespace game {

class ClearSaveButtonComponent : public tmt::GameComponent<ClearSaveButtonComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Clear Button Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void on_click(tmt::Button::Context context);

   private:
};

}  // namespace game

TMT_GAME_COMPONENT_EMPTY(game::ClearSaveButtonComponent);
