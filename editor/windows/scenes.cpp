#include "scenes.hpp"

#include <imgui.h>

#include "engine/engine.hpp"
#include "engine/core/scenes.hpp"

#include "engine/tools/serializer/ecs.hpp"

namespace tmt {

void ScenesWindow::on_editor_start() {}

void ScenesWindow::on_editor_update(const FrameData&) {}

void ScenesWindow::on_editor_end() {}

void ScenesWindow::on_scene_modified() {
    /* Don't mark dirty if game is running */
    if (engine.game_controller.is_running()) return;

    dirty_scene = true;
}

void ScenesWindow::on_scene_serialized() {
    dirty_scene = false;
}

void ScenesWindow::on_post_load_scene() {
    dirty_scene = false;
}

}  // namespace tmt