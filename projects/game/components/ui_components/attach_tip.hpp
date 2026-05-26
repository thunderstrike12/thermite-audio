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
    void update(const tmt::FrameData& time) override {};
    void end() override;

    float time_to_fade_in { 0.5f };
    float time_to_fade_out { 0.5f };
    float percentage_for_starting_to_fade = .2f;
    std::vector<tmt::Entity> ui_entities {};

   private:
    Tweening::TypedTween<float> fade_tween {};

    void on_in_range(const InRangeEvent& e);
    void show();
    void hide();
    float elapsed_time = 0.0f;
    bool in_range = false;
    bool first_contact = false;
    float current_alpha { 0.0f };
};

}  // namespace game
TMT_GAME_COMPONENT(game::AttachTip, (time_to_fade_in, time_to_fade_out, ui_entities, percentage_for_starting_to_fade));
