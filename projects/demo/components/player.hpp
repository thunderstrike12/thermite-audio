#pragma once
#include "engine/systems/gameplay/game_component.hpp"

class Player : public tmt::GameComponent<Player> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Player"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    float camera_sensitivity = 0.1f;

    float acceleration = 20.0f;
    float drag = 5.0f;
    float max_speed = 10.0f;

   private:
    glm::vec3 velocity = {0.0f, 0.0f, 0.0f};
};
TMT_OBJECT(Player, (camera_sensitivity, acceleration, drag, max_speed));