#include "inspector.hpp"

#include <imgui.h>
#include <ImReflect.hpp>

#include "engine/engine.hpp"

#include "engine/core/system_collection.hpp"
#include "engine/core/components/all.hpp"

#include "engine/tools/serialize_defines.hpp"

#include "editor/editor.hpp"

#include "editor/imgui/types/glm.hpp"
#include "editor/imgui/types/entt.hpp"

#include "editor/windows/hierarchy.hpp"

#include "editor/imgui/components/all.hpp"
#include "editor/imgui/types/all.hpp"

namespace tmt {

void Inspector::display() {
    const auto& hierarchy = editor.windows.get<Hierarchy>();
    const Entity selected_entity = hierarchy.get_first_selected_entity();
    const std::unordered_set<Entity>& selected_entities = hierarchy.get_selected_entities();

    AllComponents::for_each([selected_entity, selected_entities](auto type_tag) {
        using T = typename decltype(type_tag)::type;  // Extract type from tag

        const bool has_component = tmt::engine.ecs.has_component<T>(selected_entity);
        if (has_component == false) return;

        const auto& name = tmt::Component<T>::get_name();
        const bool is_header_open = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
        if (is_header_open == false) return;

        T& component_instance = tmt::engine.ecs.get_component<T>(selected_entity);

        const json before = tmt::Serializer::serialize(component_instance);

        const ImResponse response = ImReflect::Input("", component_instance);
        const bool changed = response.get<T>().is_changed();

        if (changed == false) return;
        if (selected_entities.size() == 1) return;

        auto after = tmt::Serializer::serialize(component_instance);
        const json diff = nlohmann::json::diff(before, after);

        for (const Entity& entity : selected_entities) {
            if (entity == selected_entity) continue;

            const bool has_component = tmt::engine.ecs.has_component<T>(entity);
            if (has_component == false) continue;

            T& other_instance = tmt::engine.ecs.get_component<T>(entity);
            json other_json = tmt::Serializer::serialize(other_instance);
            other_json.patch_inplace(diff);
            tmt::Serializer::deserialize(other_json, other_instance);
        }
    });
}

void Inspector::on_editor_start() {}

void Inspector::on_editor_update(const tmt::FrameData& time) {}

void Inspector::on_editor_end() {}

}  // namespace tmt