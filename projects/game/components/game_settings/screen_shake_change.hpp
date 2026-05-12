#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"

namespace game {

class ScreenShakeChangeComponent : public tmt::GameComponent<ScreenShakeChangeComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "ScreenShakeChangeComponent"; }
    void start() override;
    void on_value_changed(tmt::Slider::Context context);

    void update(const tmt::FrameData& time) override {};
    void end() override;
    ;
};

}  // namespace game
TMT_COMPONENT_DEPENDENCIES(game::ScreenShakeChangeComponent, tmt::Slider);

// TMT_OBJECT(game::ScreenShakeChangeComponent, (volume_control));
