#ifdef THERMITE_EDITOR

    #include "editor/editor.hpp"
    #include "level_editor_diff.hpp"

void tmt::LevelEditorDiff::undo() {
    auto level_editor = tmt::editor.systems[Editor::Mode::SCENE].try_get<LevelEditor>();
    auto& cells = level_editor->get_cells();

    for (auto& [coord, cell] : added_placements) {
        cells.erase(coord);
    }
    for (auto& [coord, cell] : removed_placements) {
        cells.emplace(coord, cell);
    }
    for (auto& [coord, cell_pair] : replaced_placements) {
        cells[coord] = cell_pair.first;
    }
}

void tmt::LevelEditorDiff::redo() {
    auto level_editor = tmt::editor.systems[Editor::Mode::SCENE].try_get<LevelEditor>();
    auto& cells = level_editor->get_cells();

    for (auto& [coord, cell] : added_placements) {
        cells.emplace(coord, cell);
    }
    for (auto& [coord, cell] : removed_placements) {
        cells.erase(coord);
    }
    for (auto& [coord, cell_pair] : replaced_placements) {
        cells[coord] = cell_pair.second;
    }
}

void tmt::LevelEditorDiff::inspect() {}

void tmt::LevelEditorDiff::commit(const std::string& message) {
    if (added_placements.empty() && removed_placements.empty() && replaced_placements.empty()) return;

    send_to_manager(std::move(*this), message);
}

#endif