#pragma once
#include "engine/systems/gameplay/game_component.hpp"

#include "events.hpp"
namespace game {

enum class WeaponType { RIFLE, GRAVITY, MINING };
class WeaponManager : public tmt::GameComponent<WeaponManager> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "WeaponManager"; }

    WeaponType get_active_weapon() const { return current_weapon; }

    void switch_to(WeaponType weapon_slot);
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void subscribe_weapon(WeaponType slot);
    void unsubscribe_weapon(WeaponType slot);

    std::unordered_map<WeaponType, tmt::Entity> weapons;
    WeaponType starting_weapon { WeaponType::RIFLE };

    tmt::Entity shooting_entity;

    float overheat_time = .8f;

   private:
    float overheat_remaining_time = -0.1f;
    /// <summary>
    /// Handles the weapon overheat event when a weapon is fired.
    /// </summary>
    /// <param name="event">The weapon fired event containing information about the fired weapon.</param>
    void on_overheat(const WeaponFiredEvent& event);
    WeaponType current_weapon;
};

}  // namespace game
TMT_OBJECT(game::WeaponManager, (weapons, starting_weapon, shooting_entity, overheat_time));
