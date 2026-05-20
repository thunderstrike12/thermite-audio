#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/tools/tweening.hpp"
#include "engine/tools/tweening/transform.hpp"

namespace game {

class TransformTween : public tmt::GameComponent<TransformTween> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "TransformTween"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

   private:
    BEFRIEND_VISITABLE();

    std::optional<glm::vec3> move_offset;
    Tweening::TypedTween<tmt::Entity, tmt::Transform> move_tween;
    std::optional<glm::vec3> rotation_offset;
    Tweening::TypedTween<tmt::Entity, tmt::Transform> rotate_tween;
    std::optional<glm::vec3> scale_offset;
    Tweening::TypedTween<tmt::Entity, tmt::Transform> scale_tween;
};

}  // namespace game
TMT_GAME_COMPONENT(game::TransformTween, (move_offset, move_tween, rotation_offset, rotate_tween, scale_offset, scale_tween));