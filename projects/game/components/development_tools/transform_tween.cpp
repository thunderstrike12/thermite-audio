#include "transform_tween.hpp"

void game::TransformTween::start() {
    if (move_offset.has_value()) Tweening::tween(move_tween).owner(entity).move().offset(move_offset.value());
    if (rotation_offset.has_value()) {
        glm::vec3 radians = glm::radians(rotation_offset.value());
        Tweening::tween(rotate_tween).owner(entity).rotate().offset(radians);
    }
    if (scale_offset.has_value()) Tweening::tween(scale_tween).owner(entity).scale().offset(scale_offset.value());
}

void game::TransformTween::update(const tmt::FrameData& time) {}

void game::TransformTween::end() {}