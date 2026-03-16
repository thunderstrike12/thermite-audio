#include "ui.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/ui/ui.hpp"

namespace tmt {

void UIEditor::on_editor_update(const FrameData& time) {
    auto* ui_system = engine.ecs.systems.try_get<UI>();
    if (ui_system == nullptr) return;

    /* Update so we can modify ui in editor */
    ui_system->on_update(time);
}

}  // namespace tmt