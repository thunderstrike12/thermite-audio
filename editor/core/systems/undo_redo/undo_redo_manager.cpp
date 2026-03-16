#include "undo_redo_manager.hpp"

#include <imgui.h>

#include "engine/core/logger.hpp"
#include "editor/editor.hpp"
#include "engine/engine.hpp"
#include "engine/core/input/input.hpp"

namespace tmt {

UndoRedoManager& IUndoRedo::get_manager() {
    auto* result = editor.systems[editor.editor_mode].try_get<UndoRedoManager>();
    if (result == nullptr) {
        throw std::runtime_error("UndoRedoManager not found in current editor mode");
    }
    return *result;
}

void UndoRedoCollection::commit(const std::string& message) {
    send_to_manager(std::move(*this), message);
}

void UndoRedoCollection::undo() {
    for (auto& action : actions) {
        action->undo();
    }
}

void UndoRedoCollection::redo() {
    for (auto& action : actions) {
        action->redo();
    }
}

void UndoRedoCollection::inspect() {
    ImGui::Text("UndoRedoCollection with %zu actions", actions.size());
}

void UndoRedoManager::commit_action(const std::shared_ptr<IUndoRedo>& action, const std::string& message) {
    CommitAction entry { message, action };
    undo_stack.push_back(entry);
    redo_stack.clear();
}

void UndoRedoManager::on_inspect() {
    if (ImGui::Button("Undo")) {
        undo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Redo")) {
        redo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) clear();

    ImGui::BeginChild("UndoRedoManager_UndoStack", ImVec2(0, 200), true);
    ImGui::Text("Undo Stack:");
    for (int i = static_cast<int>(undo_stack.size()) - 1; i >= 0; i--) {
        const auto& entry = undo_stack[i];
        ImGui::BulletText("%s", entry.message.c_str());
    }
    ImGui::EndChild();

    ImGui::BeginChild("UndoRedoManager_RedoStack", ImVec2(0, 200), true);
    ImGui::Text("Redo Stack:");
    for (int i = static_cast<int>(redo_stack.size()) - 1; i >= 0; i--) {
        const auto& entry = redo_stack[i];
        ImGui::BulletText("%s", entry.message.c_str());
    }
    ImGui::EndChild();
}

void UndoRedoManager::on_editor_start() {}

void UndoRedoManager::on_editor_update(const FrameData&) {
    if (engine.game_controller.is_running()) return;

    const bool ctrl_down = ImGui::GetIO().KeyCtrl;
    const bool is_shift_down = ImGui::GetIO().KeyShift;
    const bool is_z_just_pressed = ImGui::IsKeyPressed(ImGuiKey_Z, false);
    const bool is_y_just_pressed = ImGui::IsKeyPressed(ImGuiKey_Y, false);

    if (ctrl_down && is_z_just_pressed) {
        if (is_shift_down) {
            redo();
        } else {
            undo();
        }
    } else if (ctrl_down && is_y_just_pressed) {
        redo();
    }
}

void UndoRedoManager::on_editor_end() {}

void UndoRedoManager::undo() {
    if (undo_stack.empty()) {
        Log::warn(Log::Scope::EDITOR, "Undo stack is empty");
        return;
    }

    auto entry = std::move(undo_stack.back());
    undo_stack.pop_back();

    entry.action->undo();
    redo_stack.push_back(std::move(entry));
}
void UndoRedoManager::redo() {
    if (redo_stack.empty()) {
        Log::warn(Log::Scope::EDITOR, "Redo stack is empty");
        return;
    }

    auto entry = std::move(redo_stack.back());
    redo_stack.pop_back();

    entry.action->redo();
    undo_stack.push_back(std::move(entry));
}

}  // namespace tmt
