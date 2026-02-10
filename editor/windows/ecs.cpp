#include "ecs.hpp"

#include "imgui.h"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

void tmt::EcsInspector::display() {
    ImGui::BeginChild("ECS Inspector Child", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    const auto& registry = tmt::engine.ecs.get_registry();
    const size_t entity_count = registry.view<entt::entity>().size();
    ImGui::Text("Entities: %zu", entity_count);
    ImGui::Separator();
    ImGui::Text("Systems:");
    for (const auto& system : tmt::engine.ecs.systems) {
        ImGui::BulletText("%s", system->get_name().c_str());
    }
    ImGui::EndChild();
}