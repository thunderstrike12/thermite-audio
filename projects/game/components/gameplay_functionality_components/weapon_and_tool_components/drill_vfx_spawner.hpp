#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/events.hpp"

namespace game {

class DrillVFXSpawner : public tmt::GameComponent<DrillVFXSpawner> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "DrillVFX"; };
    void start() override;
    void update(const tmt::FrameData& time) override;

    void end() override;
    bool activate_emitter(bool active) const;
    float active_rate_per_second { 1.0f };
    tmt::Entity emitter_entity { entt::null };

   private:
    void on_shoot(const MineVoxelEvent& e);
    void on_stop_shoot(const MineNothingEvent& e);
    bool is_active { false };
    float last_shot_time { 0.0f };
};

}  // namespace game
TMT_GAME_COMPONENT(game::DrillVFXSpawner, (active_rate_per_second, emitter_entity));
