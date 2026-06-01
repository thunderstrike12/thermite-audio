#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/button.hpp"

namespace game {

class EntityControlComponent : public tmt::GameComponent<EntityControlComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "ButtonEntityControl"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void handle_action() const;

    std::vector<tmt::Entity> entities_to_disable = { entt::null };
    std::vector<tmt::Entity> entities_to_enable = { entt::null };

   private:
    void button_functionality(tmt::Button::Context context);
    void disable(tmt::Entity entity_to_disable) const;
    void enable(tmt::Entity entity_to_enable) const;
};

}  // namespace game

TMT_GAME_COMPONENT(game::EntityControlComponent, (entities_to_disable, entities_to_enable));
