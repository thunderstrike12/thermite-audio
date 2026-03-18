#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/entity.hpp"
#include "../../../editor/all.hpp"

struct Missile;

namespace game {

class MediumEnemy : public tmt::GameComponent<MediumEnemy> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Medium Enemy"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void kite_player() const;

    tmt::Entity player = entt::null;
    tmt::Entity walkable_asteroid = entt::null;
    glm::vec3 velocity = glm::vec3(0, 0, 0);
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    float rotation_speed = 2.5f;
    float height_above_ground = 0.2f;
    float height_above_ground_offset = 0.0f;
    float walk_speed = 5.0f;
    float back_off_distance = 5.0f;

    float aggro_range = 50.0f;
    float laser_range = 30.0f;
    float stomp_range = 5.0f;

    std::vector<Missile> missiles;
    float stop_launching_after = 0.5f;
    float start_homing_after = 0.5f;
    float launch_speed = 0.1f;
    float home_speed = 0.2f;
    float life_time = 0.2f;

    float missile_cooldown = 2.0f;
    float missile_timer = 0.0f;
    int missile_burst = 3;
    float burst_interval = 0.2f;
    float missile_max_randomness = 0.1f;

    float laser_cooldown = 5.0f;
    float laser_timer = 0.0f;
    float laser_sitting_down_time = 0.5f;
    float laser_winding_up_time = 0.5f;
    float laser_sitting_down_height_offset = 0.2f;
    float laser_firing_time = 1.0f;
    float laser_linear_speed = 0.0f;
    float laser_exponential_speed = 0.0f;
    float laser_linear_threshold = 0.5f;
    float laser_max_randomness = 1.0f;
    float laser_prediction_length = 0.0f;

    float stomp_cooldown = 3.0f;
    float stomp_timer = 0.0f;
    float stomp_windup = 0.5f;
    float stomp_radius = 1.0f;
};

}  // namespace game

struct Missile {
    bool update(float dt, glm::vec3 ground_up, glm::vec3 player_pos);
    enum State { LAUNCHED, HOMING } state = LAUNCHED;
    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::vec3 velocity = glm::vec3(0, 0, 0);

    float max_life_time = 0.0f;
    float life_time = 0.0f;
    float start_homing_after = 0.0f;
    float stop_launching_after = 0.0f;
    float home_speed = 0.0f;
    float launch_speed = 0.0f;
    glm::vec3 offset = glm::vec3(0, 0, 0);
    float damage = 10.0f;

    //= operator for medium enemy to update missile data in-place
    Missile& operator=(const game::MediumEnemy& other) {
        max_life_time = other.life_time;
        start_homing_after = other.start_homing_after;
        stop_launching_after = other.stop_launching_after;
        home_speed = other.home_speed;
        launch_speed = other.launch_speed;
        return *this;
    }
};

TMT_OBJECT(
    game::MediumEnemy, (walkable_asteroid, rotation_speed, height_above_ground, walk_speed, aggro_range, laser_range, stomp_range, missile_cooldown, stop_launching_after, start_homing_after,
                        launch_speed, home_speed, life_time, missile_burst, burst_interval, missile_max_randomness, laser_cooldown, laser_firing_time, laser_sitting_down_time,
                        laser_winding_up_time, laser_linear_speed, laser_exponential_speed, laser_linear_threshold, laser_max_randomness, laser_prediction_length)
);
