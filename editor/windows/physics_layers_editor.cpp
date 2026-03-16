#include "physics_layers_editor.hpp"
#include "engine/engine.hpp"
#include "engine/systems/physics/physics_system.hpp"

#include "engine/core/logger.hpp"

#include <imgui.h>

namespace tmt {

void PhysicsLayersEditor::on_editor_start() {
    // Load layers from disk when editor opens
    auto* physics = engine.ecs.systems.try_get<Physics>();
    if (physics) {
        physics->layers().load();
    }
}

void PhysicsLayersEditor::on_editor_end() {
    // Save on close
    auto* physics = engine.ecs.systems.try_get<Physics>();
    if (physics) {
        physics->layers().save();
    }
}

// Editor-local state for selection and adding/renaming layers
struct PhysicsLayerEditorState {
    char new_layer_name[64] {};  // Buffer for creating new layer
    char rename_buffer[64] {};   // Buffer for renaming selected layer
    int selected_layer = -1;     // Currently selected layer index
};

static PhysicsLayerEditorState g_state;

/**
 * Main ImGui display function for the Physics Layers editor.
 *
 * Responsibilities:
 *   - Split UI into left panel (layer list) and right panel (collision matrix)
 *   - Render interactive controls for creating, renaming, and removing layers
 */
void PhysicsLayersEditor::on_inspect() {
    auto* physics = engine.ecs.systems.try_get<Physics>();
    if (!physics) return;

    PhysicsLayers& layers = physics->layers();

    ImGui::Begin("Physics Layers");

    if (ImGui::Button("Save Physics Layers")) physics->layers().save();

    ImGui::BeginChild("LayerList", ImVec2(250, 0), true);
    draw_layer_list(layers);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("CollisionMatrix", ImVec2(0, 0), true);
    draw_collision_matrix(layers);
    ImGui::EndChild();

    ImGui::End();
}

/**
 * Draws the list of physics layers in the left panel.
 *
 * Responsibilities:
 *   - Render selectable list of layers
 *   - Enable renaming of selected layer
 *   - Enable safe removal of selected layer (does not shift indices)
 *   - Add new layers
 */
void PhysicsLayersEditor::draw_layer_list(PhysicsLayers& layers) {
    ImGui::Text("Layers");
    ImGui::Separator();

    for (uint32_t i = 0; i < layers.get_layer_count(); ++i) {
        const std::string& name = layers.get_layer_name(i);
        if (name.empty()) continue;

        if (ImGui::Selectable(name.c_str(), g_state.selected_layer == (int)i)) {
            g_state.selected_layer = i;

            // Copy current name into rename buffer
            strncpy_s(g_state.rename_buffer, sizeof(g_state.rename_buffer), name.c_str(), _TRUNCATE);
        }
    }

    ImGui::Separator();

    // --- Rename Selected ---
    if (g_state.selected_layer >= 0) {
        ImGui::InputText("Rename", g_state.rename_buffer, 64);

        if (ImGui::Button("Apply Rename")) {
            if (g_state.rename_buffer[0]) {
                layers.set_layer_name(g_state.selected_layer, g_state.rename_buffer);
            }
        }

        if (ImGui::Button("Remove Selected")) {
            layers.remove_layer(g_state.selected_layer);
            g_state.selected_layer = -1;
        }

        ImGui::Separator();
    }

    // --- Add New ---
    ImGui::InputText("New Layer", g_state.new_layer_name, 64);

    if (ImGui::Button("Add Layer")) {
        if (g_state.new_layer_name[0]) {
            layers.add_layer(g_state.new_layer_name);
            g_state.new_layer_name[0] = '\0';
        }
    }
}

/**
 * Draws the collision matrix for all physics layers.
 *
 * Responsibilities:
 *   - Render table of layer collisions
 *   - Prevent editing self-collisions
 *   - Only draw upper triangle to avoid duplicate checkboxes
 *   - Update PhysicsLayers matrix when checkboxes are toggled
 */
void PhysicsLayersEditor::draw_collision_matrix(PhysicsLayers& layers) {
    uint32_t count = layers.get_layer_count();

    if (count == 0) {
        ImGui::Text("No layers defined.");
        return;
    }

    ImGui::Text("Collision Matrix");
    ImGui::Separator();

    if (ImGui::BeginTable("CollisionTable", count + 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame)) {
        // --- header row ---
        ImGui::TableNextRow();

        // --- Index header ---
        ImGui::TableNextColumn();
        ImGui::Text("Index");

        // --- Name header ---
        ImGui::TableNextColumn();
        ImGui::Text("Name");

        // Layer name headers
        for (uint32_t col = 0; col < count; ++col) {
            ImGui::TableNextColumn();
            ImGui::Text("%s", layers.get_layer_name(col).c_str());
        }

        // --- data rows ---
        for (uint32_t row = 0; row < count; ++row) {
            ImGui::TableNextRow();

            // --- Index column ---
            ImGui::TableNextColumn();
            ImGui::Text("%u", row);

            // --- Name column ---
            ImGui::TableNextColumn();
            ImGui::Text("%s", layers.get_layer_name(row).c_str());

            // --- Collision cells ---
            for (uint32_t col = 0; col < count; ++col) {
                ImGui::TableNextColumn();

                // Self-collision
                if (row == col) {
                    ImGui::Text("X");
                    continue;
                }

                // Lower triangle (avoid duplicate editing)
                if (col < row) {
                    ImGui::Text("-");
                    continue;
                }

                bool canCollide = layers.can_collide(row, col);

                ImGui::PushID(row * 100 + col);
                if (ImGui::Checkbox("", &canCollide)) {
                    layers.enable_collision(row, col, canCollide);
                }
                ImGui::PopID();
            }
        }

        ImGui::EndTable();
    }
}

}  // namespace tmt
