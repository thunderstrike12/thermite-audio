#include "debug_lines.hpp"
#include "debug_lines.hpp"

#include <imgui.h>

#include "editor/editor.hpp"
#include "engine/events/debug.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/component_collection.hpp"
#include "engine/systems/gameplay/game_component.hpp"

namespace tmt {

void DebugLines::on_inspect() {
    // Todo move this to menu bar at some point
    auto& enabled_debug_renderers = editor.save_data.enabled_debug_renderers;
    if (ImGui::Button("Toggle All")) {
        bool all_enabled = true;
        for (const auto& [name, enabled] : enabled_debug_renderers) {
            if (enabled == false) {
                all_enabled = false;
                break;
            }
        }
        for (auto& [name, enabled] : enabled_debug_renderers) {
            enabled = !all_enabled;
        }
    }

    ImGui::SeparatorText("Systems");
    for (const auto& listener : OnDrawLines::get_listeners()) {
        const auto& name = listener->get_name();

        if (enabled_debug_renderers.contains(name) == false) {
            enabled_debug_renderers[name] = listener->default_enabled();
        }
        bool& enabled = enabled_debug_renderers[name];
        ImGui::Checkbox(name.data(), &enabled);
    }

    ImGui::SeparatorText("Components");
    const auto& components = engine.component_registry.get_registered_components();
    for (const auto& [type_id, info] : components) {
        const auto& name = info.name;
        if (enabled_debug_renderers.contains(name) == false) {
            enabled_debug_renderers[name] = true;
        }

        bool& enabled = enabled_debug_renderers[name];
        ImGui::Checkbox(name.data(), &enabled);
    }
}

void DebugLines::on_editor_update(const tmt::FrameData&) {
    auto& enabled_debug_renderers = editor.save_data.enabled_debug_renderers;
    for (const auto& listener : OnDrawLines::get_listeners()) {
        const auto& name = listener->get_name();
        if (enabled_debug_renderers.contains(name) && enabled_debug_renderers[name]) {
            listener->on_draw_lines();
        }
    }

    auto view = engine.ecs.view<const ComponentCollection>();
    for (auto [entity, collection] : view.each()) {
        for (const auto& [type_id, component] : collection.get_all_components()) {
            const std::string name = (std::string)component->get_name();
            if (enabled_debug_renderers.contains(name) && enabled_debug_renderers[name]) {
                component->draw_debug_lines();
            }
        }
    }
}

}  // namespace tmt