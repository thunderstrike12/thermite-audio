#include "menu_sounds_editor.hpp"

#include <imgui.h>
#include <ImReflect.hpp>

#include "engine/engine.hpp"
#include "engine/systems/ui/ui.hpp"

namespace tmt {

void MenuSoundsEditor::on_editor_start() {
    auto* ui = engine.ecs.systems.try_get<UI>();

    if (!ui) {
        Log::warn("UI system not active.");
        return;
    }

    ui->menu_sounds.load();
}

void MenuSoundsEditor::on_editor_end() {}

void MenuSoundsEditor::on_inspect() {
    auto* ui = engine.ecs.systems.try_get<UI>();
    if (!ui) return;

    ImGui::Begin("Menu Sounds");

    if (ImGui::Button("Save")) {
        ui->menu_sounds.save();
        Log::info("Menu sounds saved.");
    }

    ImGui::Separator();

    ImReflect::Input("", ui->menu_sounds.sounds);

    ImGui::End();
}

}  // namespace tmt