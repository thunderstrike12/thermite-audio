#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/components/managers/weapon_manager.hpp"
namespace game {

class WeaponTip : public tmt::GameComponent<WeaponTip> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "WeaponTip"; }
    void start() override;
    ;
    void update(const tmt::FrameData& time) override {};
    void end() override {}
    void on_entity_enabled() override;

    float time_to_fade_in { 0.5f };
    float time_to_fade_out { 0.5f };
    bool never_fade_out = true;
    std::vector<tmt::Entity> ui_entities {};
    WeaponType weapon_type;
    void on_weapon_switched(WeaponType new_weapon);
    float time_to_hold { 2.0f };

   private:
    void show();
    void hide();

    Tweening::TypedTween<float> fade_tween;
    float current_alpha { 1.0f };
    bool started { false };
    bool pending_show { false };
};

}  // namespace game
TMT_GAME_COMPONENT(game::WeaponTip, (time_to_fade_in, time_to_fade_out, never_fade_out, ui_entities, weapon_type, time_to_hold));
