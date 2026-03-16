#include "goap_action_editor.hpp"
#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/goap/components/goap_action.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"

#include "engine/engine.hpp"
#include "engine/core/logger.hpp"

#include <imgui.h>
#include <string>
#include <vector>

#include <extern/imgui-node-editor/imgui_node_editor.h>

namespace tmt {

void GoapActionEditor::on_editor_start() {
    Goap* goap = engine.ecs.systems.try_get<Goap>();

    if (!goap) {
        Log::warn("GOAP system not active.");
        return;
    }

    auto& overrides = goap->overrides();

    overrides.load();
}

void GoapActionEditor::on_editor_end() {
    Goap* goap = engine.ecs.systems.try_get<Goap>();

    if (!goap) {
        Log::warn("GOAP system not active.");
        return;
    }

    auto& overrides = goap->overrides();

    overrides.save();
}

/**
 * Display the editor UI for editing action overrides.
 *
 * Features:
 *   - Action selector combo
 *   - Cost editor
 *   - Preconditions editor
 *   - Effects editor
 * All of these can also be reset to their origional value
 */
void GoapActionEditor::on_inspect() {
    Goap* goap = engine.ecs.systems.try_get<Goap>();

    if (!goap) {
        Log::warn("GOAP system not active.");
        return;
    }

    auto& registry = goap->actions();
    auto& overrides = goap->overrides();

    const auto& actions_map = registry.get_all();
    if (actions_map.empty()) {
        ImGui::Text("No actions registered.");
        return;
    }

    static int selected_index = 0;
    static std::vector<std::string> action_ids;
    static std::vector<const char*> action_labels;

    if (action_ids.empty()) {
        for (const auto& [id, action] : actions_map) {
            action_ids.push_back(id);
            action_labels.push_back(id.c_str());
        }
    }

    ImGui::Combo("Select Action", &selected_index, action_labels.data(), (int)action_labels.size());
    const std::string& action_id = action_ids[selected_index];
    GoapAction* action = registry.get(action_id);
    if (!action) return;

    auto& data = overrides.get(action_id);

    // --- Cost ---
    ImGui::SeparatorText("Cost");
    float cost = (data.cost >= 0.f) ? data.cost : action->cost;
    if (ImGui::DragFloat("Cost", &cost, 0.1f)) {
        data.cost = cost;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Cost")) data.cost = -1.f;

    // --- Preconditions ---
    if (ImGui::TreeNode("Preconditions")) {
        if (ImGui::Button("Reset Preconditions")) data.preconditions.clear();

        auto merged = action->preconditions;
        for (auto& [k, v] : data.preconditions) merged[k] = v;

        for (auto it = merged.begin(); it != merged.end();) {
            bool val = it->second;
            ImGui::PushID(it->first.c_str());
            if (ImGui::Checkbox(it->first.c_str(), &val)) data.preconditions[it->first] = val;
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                data.preconditions.erase(it->first);
                it = merged.erase(it);
            } else
                ++it;
            ImGui::PopID();
        }

        static char newFact[64] {};
        ImGui::InputText("Add Fact", newFact, 64);
        ImGui::SameLine();
        if (ImGui::Button("Add")) {
            if (newFact[0]) {
                data.preconditions[newFact] = true;
                newFact[0] = '\0';
            }
        }
        ImGui::TreePop();
    }

    // --- Effects ---
    if (ImGui::TreeNode("Effects")) {
        if (ImGui::Button("Reset Effects")) data.effects.clear();

        auto merged = action->effects;
        for (auto& [k, v] : data.effects) merged[k] = v;

        for (auto it = merged.begin(); it != merged.end();) {
            bool val = it->second;
            ImGui::PushID(it->first.c_str());
            if (ImGui::Checkbox(it->first.c_str(), &val)) data.effects[it->first] = val;
            ImGui::SameLine();
            if (ImGui::Button("X")) {
                data.effects.erase(it->first);
                it = merged.erase(it);
            } else
                ++it;
            ImGui::PopID();
        }

        static char newEffect[64] {};
        ImGui::InputText("Add Effect", newEffect, 64);
        ImGui::SameLine();
        if (ImGui::Button("Add")) {
            if (newEffect[0]) {
                data.effects[newEffect] = true;
                newEffect[0] = '\0';
            }
        }
        ImGui::TreePop();
    }
}

}  // namespace tmt
