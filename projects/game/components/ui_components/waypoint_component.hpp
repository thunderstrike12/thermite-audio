#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/tools/types/bezier_curve.hpp"
namespace game {

class Waypoint : public tmt::GameComponent<Waypoint> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Waypoint"; }
    void start() override;
    void update(const tmt::FrameData& time) override;

    void end() override {}

    float hide_radius { 30.0f };
    float distance_to_fade_out { 50.0f };
    float distance_to_min_size { 75.0f };

    float max_size_icon { 200.0f };
    float min_size_icon { 50.0f };
    tmt::BezierCurve size_curve {};
    tmt::BezierCurve fade_curve {};
    tmt::Entity entity_waypoint { entt::null };
};

}  // namespace game
TMT_GAME_COMPONENT(game::Waypoint, (distance_to_fade_out, hide_radius, distance_to_min_size, max_size_icon, min_size_icon, size_curve, fade_curve, entity_waypoint));
