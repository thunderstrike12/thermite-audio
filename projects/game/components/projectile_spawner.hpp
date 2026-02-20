#pragma once
#include "events.hpp"
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class ProjectileSpawner : public tmt::GameComponent<ProjectileSpawner> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name();

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

   private:
    void on_shoot(const WeaponFiredEvent& e) const;
};

}  // namespace game
