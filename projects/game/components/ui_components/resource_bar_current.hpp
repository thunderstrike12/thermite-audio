#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"
#include "engine/core/components/camera.hpp"
#include "projects/game/data_headers/events.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"

namespace game {

enum class DisplayTypeResourceBar : uint8_t { HEALTH, ENERGY, FUEL };

class ResourceBarCurrentComponent : public tmt::GameComponent<ResourceBarCurrentComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Resource Bar Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    tmt::Entity player_entity = entt::null;
    tmt::Entity barge_fuel_entity = entt::null;
    int resource_per_segment = 5;
    std::string rendered_string = "/";
    DisplayTypeResourceBar resource = DisplayTypeResourceBar::HEALTH;

   private:
    void health_changed(PlayerHealthChanged);
    void energy_changed(PlayerEnergyChanged);
    void fuel_changed(BargeFuelChanged);
    void resource_bar_update(int, float);
};

}  // namespace game
TMT_GAME_COMPONENT(game::ResourceBarCurrentComponent, (player_entity, barge_fuel_entity, resource, resource_per_segment, rendered_string));
