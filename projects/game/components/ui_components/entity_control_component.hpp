#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"

namespace game {

class EntityControlComponent : public tmt::GameComponent<EntityControlComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "ButtonEntityControl"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    /*
    std::vector<tmt::Entity> entities_to_disable = { entt::null };
    std::vector<tmt::Entity> entities_to_enable = { entt::null };
    */

    tmt::Entity entity_to_disable = entt::null;
    tmt::Entity entity_to_enable = entt::null;

   private:
    void button_functionality();
    void disable(tmt::Entity entity_to_disable);
    void enable(tmt::Entity entity_to_enable);
};

}  // namespace game

TMT_OBJECT(game::EntityControlComponent, (entity_to_disable, entity_to_enable));
