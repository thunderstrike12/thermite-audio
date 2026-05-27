#include "unpause_button_component.hpp"
#include "projects/game/components/managers/menu_controller.hpp"

namespace game {

void UnpauseButtonComponent::start() {
    // menu controller entity set
    auto menu_controller_view = tmt::engine.ecs.view<MenuController>();
    if (menu_controller_view.empty()) {
        tmt::Log::error("Menu controller not found. Please ensure a menu controller is present to use the unpause button component.");
        return;
    }
    menu_controller_entity = menu_controller_view.front().entity;

    // Guard for button
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        tmt::Log::info("Found button component, adding self destruct to on click.");
        button_component->on_click.add(this, &UnpauseButtonComponent::unpause);
    } else {
        tmt::Log::warn("No button found for self destruct component!");
    }
}

void UnpauseButtonComponent::update(const tmt::FrameData& time) {}

void UnpauseButtonComponent::end() {
    // Guard for button
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_click.clear();
    }
}

void UnpauseButtonComponent::unpause(tmt::Button::Context context) {
    MenuController* menu_controller_component = tmt::engine.ecs.try_get_component<MenuController>(menu_controller_entity);
    if (!menu_controller_component) {
        tmt::Log::error("Unpause button couldnt fetch menu controller");
        return;
    }
    menu_controller_component->disable_pause_menu();
}

}  // namespace game