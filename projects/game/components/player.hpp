#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"
#include "events.hpp"
namespace game {

struct PlayerStat {
    float max_value = 100.f;
    float value = 100.f;
    float increase_multiplier = 1.0f;
};
enum class PlayerState { FREEMOVING, ATTACHED, PAUSED };

class Player : public tmt::GameComponent<Player> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Player"; }

    void start() override;
    void look_camera() const;
    void move_player();
    void update(const tmt::FrameData& time) override;
    void attempt_attach(tmt::Input& input);
    void on_attach(const AttachEvent& event);
    void end() override;

    float camera_sensitivity = 0.1f;

    // Movement parameters
    float acceleration = 20.0f;
    float deceleration = 40.0f;
    float drag = 5.0f;
    float max_speed = 10.0f;

    // Player stats
    PlayerStat health;
    PlayerStat energy;

    // Helper functions
    tmt::Transform& get_transform() const { return tmt::engine.ecs.get_component<tmt::Transform>(entity); }
    tmt::Camera& get_camera() const { return tmt::engine.ecs.get_component<tmt::Camera>(entity); }
    void set_state(PlayerState new_state) { state = new_state; };

    tmt::Entity hp_bar_max_entity = entt::null;
    tmt::Entity hp_bar_current_entity = entt::null;

   private:
    void refill(float delta);
    PlayerState state = PlayerState::FREEMOVING;
    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
};

}  // namespace game
TMT_OBJECT(game::PlayerStat, (max_value, value, increase_multiplier));
TMT_OBJECT(game::Player, (camera_sensitivity, acceleration, deceleration, drag, max_speed, health, energy, hp_bar_max_entity, hp_bar_current_entity));
