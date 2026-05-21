#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/events.hpp"
#include "engine/tools/types/bezier_curve.hpp"
namespace game {

class AttachTip : public tmt::GameComponent<AttachTip> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "AttachTip"; }
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    float time_to_fade = 1.0f;
    float percentage_for_starting_to_fade = .2f;
    tmt::Entity ui_entity = entt::null;

   private:
    void on_in_range(const InRangeEvent& e);
    float elapsed_time = 0.0f;
    bool in_range = false;
    bool first_contact = false;
};

}  // namespace game
TMT_GAME_COMPONENT(game::AttachTip, (time_to_fade, percentage_for_starting_to_fade, ui_entity));
