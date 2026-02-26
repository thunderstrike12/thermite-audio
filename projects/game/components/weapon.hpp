#pragma once
#include "events.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "weapon_manager.hpp"
namespace game {

struct FireRate {
    float shots_per_second = 5.0f;

   private:
    friend class Weapon;
    float last_shot_time = 0.0f;
};
class Weapon : public tmt::GameComponent<Weapon> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Weapon"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    bool update_fire_rate(FireRate& fire_rate);

    FireRate primary_fire_rate;
    FireRate secondary_fire_rate;

    tmt::Entity shooting_entity;
    tmt::Entity spawn_location_entity = entt::null;

   private:
    friend class WeaponManager;
    void on_shoot(const ShootEvent& e);
};

}  // namespace game
TMT_OBJECT(game::FireRate, (shots_per_second));

TMT_OBJECT(game::Weapon, (primary_fire_rate, secondary_fire_rate, shooting_entity, spawn_location_entity));
