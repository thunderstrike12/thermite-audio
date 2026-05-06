#include "ui.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/ui_component.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

void UI::on_start() {}

void UI::on_update(const FrameData&) {
    const glm::uvec2 screen_size = engine.renderer.viewport_size();

    auto view = engine.ecs.get_registry().view<UIComponent, const Transform>();
    for (const auto& [entity, ui_component, transform] : view.each()) {
        if (transform.has_parent()) {
            const Entity parent_entity = transform.get_parent();
            if (engine.ecs.has_component<UIComponent>(parent_entity)) {
                /* Parent UI will drive this one, so skip it. */
                continue;
            }
        }
        set_size(ui_component, transform, screen_size);
    }
}

void UI::set_size(tmt::UIComponent& ui_component, const tmt::Transform& transform, const glm::uvec2& screen_size) {
    const bool scale_x = ui_component.scale_with_parent_x;
    const bool scale_y = ui_component.scale_with_parent_y;

    if (scale_x || scale_y) {
        const bool has_parent = transform.has_parent();
        const Entity parent_entity = transform.get_parent();
        const bool has_parent_ui = has_parent && engine.ecs.has_component<UIComponent>(parent_entity);

        if (has_parent_ui) {
            /* Use parent UI, but first do parent */
            UIComponent& parent_ui = engine.ecs.get_component<UIComponent>(parent_entity);
            if (scale_x) ui_component.size.x = parent_ui.size.x;
            if (scale_y) ui_component.size.y = parent_ui.size.y;
        } else {
            /* Use screen */
            if (scale_x) ui_component.size.x = static_cast<float>(screen_size.x);
            if (scale_y) ui_component.size.y = static_cast<float>(screen_size.y);
        }

        if (ui_component.aspect_mode != AspectMode::STRETCH) {
            const float aspect_ratio = ui_component.aspect_ratio.x / ui_component.aspect_ratio.y;
            const float current_ratio = ui_component.size.x / ui_component.size.y;

            // Only one axis is driven, derive the other from it unconditionally.
            if (scale_x && !scale_y) {
                ui_component.size.y = ui_component.size.x / aspect_ratio;
            } else if (!scale_x && scale_y) {
                ui_component.size.x = ui_component.size.y * aspect_ratio;
            } else if (scale_x && scale_y) {
                // Both axes are driven, use the aspect mode to decide which wins.
                if (ui_component.aspect_mode == AspectMode::FIT) {
                    if (current_ratio > aspect_ratio) {
                        ui_component.size.x = ui_component.size.y * aspect_ratio;
                    } else {
                        ui_component.size.y = ui_component.size.x / aspect_ratio;
                    }
                } else if (ui_component.aspect_mode == AspectMode::FILL) {
                    if (current_ratio > aspect_ratio) {
                        ui_component.size.y = ui_component.size.x / aspect_ratio;
                    } else {
                        ui_component.size.x = ui_component.size.y * aspect_ratio;
                    }
                }
            }
        }
    }

    /* First do parents then children */
    for (const auto& child_entity : transform.get_children()) {
        if (engine.ecs.has_component<UIComponent>(child_entity)) {
            UIComponent& child_ui = engine.ecs.get_component<UIComponent>(child_entity);
            Transform& child_transform = engine.ecs.get_component<Transform>(child_entity);
            set_size(child_ui, child_transform, screen_size);
        }
    }
}

void UI::on_end() {}

}  // namespace tmt