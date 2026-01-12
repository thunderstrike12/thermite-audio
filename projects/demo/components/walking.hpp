#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/entity.hpp"

class Walking : public tmt::GameComponent<Walking> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Walking"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    tmt::Entity walkable_asteroid = entt::null;
    float walk_speed = 5.0f;
    float attack_range = 20.0f;
    float max_chase_distance = 100.0f;
};

TMT_OBJECT(Walking, (walkable_asteroid, walk_speed, attack_range, max_chase_distance));