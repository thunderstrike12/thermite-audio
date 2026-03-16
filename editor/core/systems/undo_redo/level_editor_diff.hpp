#pragma once
#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"
#include "editor/windows/level_editor.hpp"

namespace tmt {

class LevelEditorDiff : public IUndoRedo {
   public:
    // Inherited via IUndoRedo
    void undo() override;
    void redo() override;
    void inspect() override;

    void commit(const std::string& message);

    std::unordered_map<glm::ivec2, LevelEditor::Cell> removed_placements;
    std::unordered_map<glm::ivec2, LevelEditor::Cell> added_placements;
    std::unordered_map<glm::ivec2, std::pair<LevelEditor::Cell, LevelEditor::Cell>> replaced_placements;
};

}  // namespace tmt