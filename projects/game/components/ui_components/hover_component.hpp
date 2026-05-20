#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"

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
    void disable(tmt::UIInteractable::Context context);
    void enable(tmt::UIInteractable::Context context);
};

}  // namespace game

TMT_GAME_COMPONENT(game::HoverComponent, (hover_entity));
