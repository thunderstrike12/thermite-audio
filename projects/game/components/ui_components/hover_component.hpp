#pragma once
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class HoverComponent : public tmt::GameComponent<HoverComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "UI Hover Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    tmt::Entity hover_entity = entt::null;

   private:
    void disable(tmt::Button::Context context);
    void enable(tmt::Button::Context context);
};

}  // namespace game

TMT_OBJECT(game::HoverComponent, (hover_entity));
