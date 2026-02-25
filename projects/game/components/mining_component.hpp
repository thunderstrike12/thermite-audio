#pragma once
#include "events.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"

namespace game {

class MiningComponent : public tmt::GameComponent<MiningComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Mining Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    tmt::ResourceRef<tmt::Stencil> stencil;  // currently active stencil
    float cooldown = 1.0f;          // mining cooldown
    bool active = true;
    bool input_active_on_non_player = false;              // if needs to react to player input but is not on player entity

   private:
    std::vector<tmt::ResourceRef<tmt::Stencil>> stencils; // might be used in the future when we have different stencils to randomly select from, for now unused
    
    void mine(glm::vec3 origin, glm::vec3 dir); // base mining function, do not overload if using events, create new function instead
    void mine_on_weapon_fired(const WeaponFiredEvent& e);
    float cooldown_counter = 0.0f;
};

}  // namespace game
TMT_OBJECT(game::MiningComponent, (stencil, active, input_active_on_non_player));