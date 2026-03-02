#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"
#include "events.hpp"
namespace game {

class Player : public tmt::GameComponent<Player> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Player"; }

    void start() override;
    void look_camera() const;
    void move_player();
    void update(const tmt::FrameData& time) override;
    void on_attach(const AttachEvent& event);
    void end() override;

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

    // Helper functions
    tmt::Transform& get_transform() const { return tmt::engine.ecs.get_component<tmt::Transform>(entity); }
    tmt::Camera& get_camera() const { return tmt::engine.ecs.get_component<tmt::Camera>(entity); }

    void toggle_camera_movement() {
        if (camera_movement) {
            camera_movement = false;
        } else {
            camera_movement = true;
        }
    }
    void toggle_player_movement() {
        if (player_movement) {
            player_movement = false;
        } else {
            player_movement = true;
        }
    }

   private:
    bool can_move = true;
    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
    bool camera_movement = true;
    bool player_movement = true;
};

}  // namespace game
TMT_OBJECT(game::Player, (camera_sensitivity, acceleration, deceleration, drag, max_speed, health, battery, max_health, max_battery));
