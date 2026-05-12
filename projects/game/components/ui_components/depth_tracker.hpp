#pragma once
#include "engine/systems/gameplay/game_component.hpp"
namespace game {

enum class DepthTrackerDistanceType : uint8_t { TOTAL_DISTANCE, PATH_DISTANCE };

class DepthTracker : public tmt::GameComponent<DepthTracker> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Depth Tracker"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override {}

    tmt::Entity player_entity = entt::null;
    tmt::Entity barge_entity = entt::null;

    DepthTrackerDistanceType distance_type = DepthTrackerDistanceType::TOTAL_DISTANCE;

    int displayed_decimals = 2;

    void measure_distance();
    void change_text() const;

   private:
    float distance = 0.0f;
};

}  // namespace game
TMT_OBJECT(game::DepthTracker, (player_entity, barge_entity, distance_type, displayed_decimals));
