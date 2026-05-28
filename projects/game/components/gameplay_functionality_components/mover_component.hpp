#pragma once

#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/events.hpp"
#include "projects/game/components/development_tools/debug_line_helper.hpp"
#include "engine/core/components/audio_emitter.hpp"

namespace game {

class MoverComponent : public tmt::GameComponent<MoverComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "MoverComponent"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override {};
    void draw_debug_lines() const;
    void update_movement(const TriggerMovementEvent& event);
    void stop_movement(const TriggerMovementStopEvent& event);
    void on_game_paused(const game::GamePausedEvent&);
    void on_game_unpaused(const game::GameUnpausedEvent&);

    float movement_speed = 1.0f;
    float acceleration = 1.f;
    float deceleration = 1.f;
    bool can_move = true;
    DebugLineConfig cfg;

    // this has to be the same entity that triggered the event
    tmt::Entity trigger_entity = entt::null;

    tmt::Entity move_flair_entity = entt::null;

    // audio
    tmt::AudioEvent jet_propulsion;
    tmt::AudioInstance jet_sound_instance;

   private:
    float velocity = 0.f;
    bool stop = false;
    bool paused = false;
};

}  // namespace game
TMT_GAME_COMPONENT(game::MoverComponent, (movement_speed, can_move, cfg, trigger_entity, acceleration, deceleration, move_flair_entity, jet_propulsion));
