#pragma once
#include "events.hpp"
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class Weapon : public tmt::GameComponent<Weapon> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Weapon"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    float fire_rate = 5.f;

    tmt::Entity shooting_entity;
    tmt::Entity spawn_location_entity = entt::null;

   private:
    void on_shoot(const ShootEvent& e);
    float last_shot_time = 0.0f;
};

}  // namespace game
TMT_OBJECT(game::Weapon, (fire_rate, shooting_entity, spawn_location_entity));
