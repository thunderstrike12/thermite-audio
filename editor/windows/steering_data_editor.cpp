#include "steering_data_editor.hpp"
#include "engine/systems/ai/steering/steering_system.hpp"
#include "engine/systems/ai/steering/components/steering_mode.hpp"

#include "engine/engine.hpp"
#include "engine/core/logger.hpp"

#include <imgui.h>
#include <string>
#include <vector>

#include <extern/imgui-node-editor/imgui_node_editor.h>

namespace tmt {

void SteeringDataEditor::on_editor_start() {
    auto* steering = engine.ecs.systems.try_get<SteeringSystem>();

    if (!steering) {
        Log::warn("Steering system not active.");
        return;
    }

    steering->overrides().load();
}

void SteeringDataEditor::on_editor_end() {
    auto* steering = engine.ecs.systems.try_get<SteeringSystem>();

    if (!steering) {
        Log::warn("Steering system not active.");
        return;
    }

    steering->overrides().save();
}

void SteeringDataEditor::on_inspect() {
    auto* steering = engine.ecs.systems.try_get<SteeringSystem>();

    if (!steering) {
        ImGui::Text("Steering system not active.");
        return;
    }

    auto& params = steering->overrides().params;

    ImGui::SeparatorText("Steering Parameters");

    ImGui::DragFloat("Max Speed", &params.max_speed, 0.1f, 0.f, 50.f);
    ImGui::DragFloat("Max Force", &params.max_force, 0.1f, 0.f, 100.f);
    ImGui::DragFloat("Arrive Radius", &params.arrive_radius, 0.1f, 0.f, 50.f);
    ImGui::DragFloat("Activation Range", &params.activation_range, 0.5f, 0.f, 100.f);
    ImGui::DragFloat("Min Explosion Range", &params.min_explosion_range, 0.1f, 0.f, 50.f);
    ImGui::DragFloat("Max Explosion Range", &params.max_explosion_range, 0.1f, 0.f, 50.f);

    if (ImGui::Button("Reset Defaults")) {
        params = SteeringParams {};
    }

    // --- Save button ---
    if (ImGui::Button("Save Steering Paramaters")) {
        steering->overrides().save();
        Log::info("Steering override params saved");
    }
}

}  // namespace tmt
