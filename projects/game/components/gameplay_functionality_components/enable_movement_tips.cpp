#include "enable_movement_tips.hpp"

#include "projects/game/components/ui_components/movement_tip.hpp"
void game::EnableMovementTips::enable_entities_in_order() const {
    bool first_to_enable { true };
    for (tmt::Entity entity_tip : movement_tips_entities) {
        if (auto* tip { tmt::engine.ecs.try_get_component<MovementTip>(entity_tip) }) {
            tip->try_to_enable(first_to_enable);
        }
    }
}
