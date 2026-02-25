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

void DebugLines::display() {
    // Todo move this to menu bar at some point
    if (ImGui::BeginMenu("Debug Lines")) {
        auto& enabled_debug_renderers = editor.save_data.enabled_debug_renderers;
        for (const auto& listener : OnDrawLines::get_listeners()) {
            const auto& name = listener->get_name();

            if (enabled_debug_renderers.contains(name) == false) {
                enabled_debug_renderers[name] = listener->default_enabled();
            }
            bool& enabled = enabled_debug_renderers[name];

            ImGui::MenuItem(name.data(), nullptr, &enabled);
        }
        ImGui::EndMenu();
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
            component->draw_debug_lines();
        }
    }
}

}  // namespace tmt