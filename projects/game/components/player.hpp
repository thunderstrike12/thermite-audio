#pragma once
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class Player : public tmt::GameComponent<Player> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Player"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void trigger_shoot();

    float camera_sensitivity = 0.1f;

    // Movement parameters
    float acceleration = 20.0f;
    float deceleration = 40.0f;
    float drag = 5.0f;
    float max_speed = 10.0f;

    // Player stats
    float health = 100.0f;
    float battery = 100.0f;
    float max_health = 100.0f;
    float max_battery = 100.0f;

   private:
    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
};

}  // namespace game
TMT_OBJECT(game::Player, (camera_sensitivity, acceleration, deceleration, drag, max_speed, health, battery, max_health, max_battery));
