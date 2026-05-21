#include "ecs.hpp"

#include "imgui.h"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/all.hpp"

void tmt::EcsInspector::on_inspect() {
    ImGui::BeginChild("ECS Inspector Child", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    auto view = tmt::engine.ecs.get_registry().view<entt::entity>();

    const size_t entity_count = view.size();
    ImGui::Text("Entities: %zu", entity_count);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Entities")) {
        if (ImGui::BeginTable("EntitiesTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Entity ID");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Transform");
            ImGui::TableSetupColumn("Prefab");
            ImGui::TableSetupColumn("Debug");
            ImGui::TableHeadersRow();

            for (const auto& entity : view) {
                ImGui::TableNextRow();
                ImGui::PushID(static_cast<int>(entity));

                const volatile auto* transform = engine.ecs.try_get_component<Transform>(entity);
                const volatile auto* name = engine.ecs.try_get_component<Name>(entity);
                const volatile auto* prefab = engine.ecs.try_get_component<Prefab>(entity);

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", EntityHelper::to_string(entity).c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", name ? ICON_MS_CHECK_BOX : ICON_MS_CHECK_BOX_OUTLINE_BLANK);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", transform ? ICON_MS_CHECK_BOX : ICON_MS_CHECK_BOX_OUTLINE_BLANK);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", prefab ? ICON_MS_CHECK_BOX : ICON_MS_CHECK_BOX_OUTLINE_BLANK);

                ImGui::TableSetColumnIndex(4);
                if (ImGui::Button("Debug")) {
                    /* Call break point */
                    IM_DEBUG_BREAK();
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("(Debug Only)");

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    if (ImGui::CollapsingHeader("Component Counts")) {
        InspectComponents::for_each([](auto type) {
            using ComponentType = typename decltype(type)::type;
            size_t count = tmt::engine.ecs.get_registry().template view<ComponentType>().size();
            /* Quick and dirty way of getting the name */
            ImGui::BulletText("%s: %zu", typeid(ComponentType).name(), count);
        });
    }

    ImGui::Separator();
    ImGui::Text("Systems:");
    for (const auto& system : tmt::engine.ecs.systems) {
        ImGui::BulletText("%s", system->get_name().c_str());
    }
    ImGui::EndChild();
}