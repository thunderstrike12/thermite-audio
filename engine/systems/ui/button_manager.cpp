#include "button_manager.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/components/button.hpp"
#include "engine/core/components/image_renderer.hpp"
#include "engine/core/components/ui_component.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/renderer/renderer.hpp"
#include <cmath>

namespace tmt {

void UIElementManager::on_start() {
    auto view = engine.ecs.view<UIInteractable>();
    for (auto [entity, interactable] : view.each()) {
        interactable.state = ButtonState::IDLE;
    }
    set_default_button();
}

void UIElementManager::on_update(const tmt::FrameData& time) {
    update_move_state(time.delta_time);
    update_states(time);
}

void UIElementManager::update_states(const tmt::FrameData& time) {
    /* group with buttons and image renderers */
    auto button_view = engine.ecs.view<Button, UIInteractable, ImageRenderer>();
    for (const auto& [entity, button, interactable, image_renderer] : button_view.each()) {
        image_renderer.color = button.colors.at(static_cast<uint8_t>(interactable.state));
        Button::Context context { entity, interactable.state, interactable.disabled };

        switch (interactable.state) {
            case ButtonState::IDLE:
                break;
            case ButtonState::ON_SELECT:
                interactable.on_select({ entity, interactable.state, interactable.disabled });
                interactable.state = ButtonState::SELECTED;
                break;
            case ButtonState::SELECTED:
                interactable.on_selected({ entity, interactable.state, interactable.disabled });
                break;
            case ButtonState::ON_DESELECT:
                interactable.on_deselect({ entity, interactable.state, interactable.disabled });
                interactable.state = ButtonState::IDLE;
                break;
            case ButtonState::DISABLED:
                break;
            case ButtonState::ON_CLICK:
                button.on_click(context);
                interactable.state = ButtonState::ON_HOLD;
                break;
            case ButtonState::ON_HOLD:
                button.on_hold(context, time.delta_time);
                hold_time += time.delta_time;
                break;
            case ButtonState::ON_RELEASE:
                button.on_release(context);
                interactable.state = ButtonState::IDLE;
                hold_time = 0.0f;
                break;
        }
    }

    const glm::vec2 screen_size = engine.renderer.viewport_size();
    const glm::vec2 mouse_pos = engine.input.get_mouse_position(); /* top-left origin, real screen pixels */

    auto slider_view = engine.ecs.view<Slider, UIInteractable, UIComponent, Transform>();
    for (const auto& [entity, slider, interactable, ui_component, transform] : slider_view.each()) {
        Slider::Context context { entity, interactable.state, interactable.disabled, slider.value };

        switch (interactable.state) {
            case ButtonState::IDLE:
                break;
            case ButtonState::ON_SELECT:
                interactable.on_select({ entity, interactable.state, interactable.disabled });
                interactable.state = ButtonState::SELECTED;
                break;
            case ButtonState::SELECTED:
                interactable.on_selected({ entity, interactable.state, interactable.disabled });
                break;
            case ButtonState::ON_DESELECT:
                interactable.on_deselect({ entity, interactable.state, interactable.disabled });
                interactable.state = ButtonState::IDLE;
                break;
            case ButtonState::DISABLED:
                break;
            case ButtonState::ON_CLICK:
            case ButtonState::ON_HOLD:
            case ButtonState::ON_RELEASE:
                break;
        }

        const float image_width = ui_component.real_size.x;
        const glm::uvec2 screen_res = engine.renderer.render_view.gpu_view.resolution;
        const glm::vec2 pos_scale_factor(
            static_cast<float>(screen_res.x) / UIComponent::REFERENCE_WIDTH,
            static_cast<float>(screen_res.y) / UIComponent::REFERENCE_HEIGHT
        );

        /* pivot_screen_x is the point on screen where the slider's pivot lands.
           The left edge depends on the pivot: left = pivot_pos - pivot.x * width */
        const glm::vec3 slider_position_raw = transform.get_world_position();
        const glm::vec2 slider_anchor_offset = AnchorHelper::calculate_anchor_offset(entity);
        const float pivot_screen_x = slider_position_raw.x * pos_scale_factor.x + slider_anchor_offset.x;
        const float slider_left_x  = pivot_screen_x - ui_component.pivot.x * image_width;

        /* Snap handle: screen X of the handle's position along the track. */
        const float handle_screen_x = slider_left_x
            + (slider.value - slider.min) / (slider.max - slider.min) * image_width;

        auto& handle_transform = engine.ecs.get_component<Transform>(slider.handle_entity);
        const glm::vec2 handle_anchor_offset = AnchorHelper::calculate_anchor_offset(slider.handle_entity);
        const float handle_authored_x = (handle_screen_x - handle_anchor_offset.x) / pos_scale_factor.x;
        const glm::vec3 handle_position = handle_transform.get_world_position();
        handle_transform.set_world_position({ handle_authored_x, handle_position.y, handle_position.z });

        const auto allowed_states = { ButtonState::ON_CLICK, ButtonState::ON_HOLD };
        if (std::find(allowed_states.begin(), allowed_states.end(), interactable.state) == allowed_states.end()) {
            continue;
        }

        /* mouse_pos and slider_left_x are both in real screen pixels (top-left origin). */
        const float relative_x = (mouse_pos.x - slider_left_x) / image_width;
        float new_value = std::clamp(relative_x * (slider.max - slider.min) + slider.min, slider.min, slider.max);
        /* Round to step */
        new_value = std::round(new_value / slider.step) * slider.step;

        slider.value = new_value;
        slider.on_value_changed(context);
    }
}

void UIElementManager::on_end() {}

bool UIElementManager::select_button(const Entity button) {
    if (selected_entity == button) return true;

    /* Deselect previous button */
    if (selected_entity != entt::null) {
        auto* prev_button = engine.ecs.try_get_component<UIInteractable>(selected_entity);
        if (prev_button && prev_button->state == ButtonState::ON_HOLD) {
            prev_button->state = ButtonState::ON_RELEASE;
        } else if (prev_button)
            prev_button->state = ButtonState::ON_DESELECT;
        previous_entity = selected_entity;
    }

    if (previous_entity == button) previous_entity = entt::null;
    selected_entity = button;

    /* Select new button */
    if (selected_entity == entt::null) return false;

    auto* new_button = engine.ecs.try_get_component<UIInteractable>(selected_entity);
    if (new_button) new_button->state = ButtonState::ON_SELECT;
    return true;
}

bool UIElementManager::next_button(Direction direction) {
    if (selected_entity == entt::null) {
        set_default_button();
        return false;
    }

    auto* interactable = engine.ecs.try_get_component<UIInteractable>(selected_entity);
    if (!interactable) {
        return set_default_button();
    }

    int dir = static_cast<int>(direction);
    if (dir < 0 || dir >= static_cast<int>(interactable->flow_direction.size())) return false;

    Entity next_entity = interactable->flow_direction[dir];
    if (next_entity == entt::null) return false;

    return select_button(next_entity);
}

bool UIElementManager::set_default_button() {
    if (previous_entity != entt::null) {
        return select_button(previous_entity);
    }

    auto view = engine.ecs.view<Button>();
    /* set top-left most button as first */
    Entity default_button = entt::null;
    float min_distance = std::numeric_limits<float>::max();
    for (auto [entity, button] : view.each()) {
        auto* transform = engine.ecs.try_get_component<Transform>(entity);
        if (!transform) continue;
        const auto& pos = transform->get_world_position();
        float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
        if (distance < min_distance) {
            min_distance = distance;
            default_button = entity;
        }
    }

    if (default_button != entt::null) {
        return select_button(default_button);
    }

    return false;
}

void UIElementManager::set_state(ButtonState state) {
    if (selected_entity == entt::null) return;

    auto* interactable = engine.ecs.try_get_component<UIInteractable>(selected_entity);
    if (!interactable) return;
    interactable->state = state;
}

void UIElementManager::update_move_state(float dt) {
    if (move_state == MoveState::INITIAL_MOVE || move_state == MoveState::REPEAT_MOVE) {
        hold_move_duration += dt;
    } else if (move_state == MoveState::IDLE) {
        hold_move_duration = 0.0f;
    }
}

void UIElementManager::on_mouse_move(MouseMoveEvent& event) {
    const auto mouse_pos = glm::vec2(event.x, event.y);
    auto view = engine.ecs.view<Button>();
    for (auto [entity, button] : view.each()) {
        if (AnchorHelper::is_inside(entity, mouse_pos)) {
            select_button(entity);
            return;
        }
    }

    auto slider_view = engine.ecs.view<Slider>();
    for (auto [entity, slider] : slider_view.each()) {
        if (AnchorHelper::is_inside(entity, mouse_pos)) {
            select_button(entity);
            return;
        }
        if (slider.handle_entity != entt::null && AnchorHelper::is_inside(slider.handle_entity, mouse_pos)) {
            select_button(entity);
            return;
        }
    }

    /* if it has a slider in drag state, don't deselect */
    if (selected_entity != entt::null) {
        auto* slider = engine.ecs.try_get_component<Slider>(selected_entity);
        if (slider) {
            auto* interactable = engine.ecs.try_get_component<UIInteractable>(selected_entity);
            if (interactable && (interactable->state == ButtonState::ON_HOLD || interactable->state == ButtonState::ON_CLICK)) {
                return;
            }
        }
    }
    select_button(entt::null);
}

void UIElementManager::on_mouse_button(MouseButtonEvent& event) {
    if (event.state == MouseButtonState::JUST_PRESSED) {
        switch (event.button) {
            case MouseButton::LEFT:
                set_state(ButtonState::ON_CLICK);
                break;
            default:
                break;
        }
    } else if (event.state == MouseButtonState::PRESSED) {
        switch (event.button) {
            case MouseButton::LEFT:
                set_state(ButtonState::ON_HOLD);
                break;
            default:
                break;
        }
    } else if (event.state == MouseButtonState::JUST_RELEASED) {
        switch (event.button) {
            case MouseButton::LEFT:
                set_state(ButtonState::ON_RELEASE);
                break;
            default:
                break;
        }
    }
}

void UIElementManager::on_keyboard(KeyboardEvent& event) {
    if (event.state == KeyState::JUST_PRESSED) {
        handle_just_pressed_keyboard(event);

    } else if (event.state == KeyState::PRESSED) {
        handle_pressed_keyboard(event);
    } else if (event.state == KeyState::JUST_RELEASED) {
        move_state = MoveState::IDLE;
        switch (event.key) {
            case Key::RETURN:
                set_state(ButtonState::ON_RELEASE);
                hold_time = 0.0f;
                event.handled = true;
                break;
            default:
                break;
        }
    }
}

void UIElementManager::handle_just_pressed_keyboard(tmt::KeyboardEvent& event) {
    static std::map<tmt::Key, tmt::Direction> key_direction_map = {
        { Key::UP, Direction::NORTH },
        { Key::DOWN, Direction::SOUTH },
        { Key::LEFT, Direction::WEST },
        { Key::RIGHT, Direction::EAST },
    };

    if (event.key == Key::RETURN) {
        set_state(ButtonState::ON_CLICK);
        event.handled = true;
        return;
    }

    auto it = key_direction_map.find(event.key);
    if (it == key_direction_map.end()) {
        return;
    }

    const tmt::Direction direction = it->second;
    const bool swapped = next_button(direction);
    if (swapped) {
        event.handled = true;
        return;
    }

    /* Handle component input... */
    if (selected_entity == entt::null) {
        return;
    }

    // const auto& interactable = engine.ecs.get_component<UIInteractable>(selected_entity);

    if (auto* slider = engine.ecs.try_get_component<Slider>(selected_entity)) {
        if (direction == Direction::EAST) {
            slider->value = std::min(slider->value + slider->step, slider->max);
            event.handled = true;
        } else if (direction == Direction::WEST) {
            slider->value = std::max(slider->value - slider->step, slider->min);
            event.handled = true;
        }
    }
}

void UIElementManager::handle_pressed_keyboard(tmt::KeyboardEvent& event) {
    if (move_state == MoveState::IDLE) {
        move_state = MoveState::INITIAL_MOVE;
    }

    bool call_next_button = false;
    if (move_state == MoveState::INITIAL_MOVE) {
        if (hold_move_duration > INITIAL_DELAY) {
            move_state = MoveState::REPEAT_MOVE;
            hold_move_duration = 0.0f;
            call_next_button = true;
        }
    } else if (move_state == MoveState::REPEAT_MOVE) {
        if (hold_move_duration > REPEAT_DELAY) {
            hold_move_duration = 0.0f;
            call_next_button = true;
        }
    }

    if (call_next_button) {
        static std::map<tmt::Key, tmt::Direction> key_direction_map = {
            { Key::UP, Direction::NORTH },
            { Key::DOWN, Direction::SOUTH },
            { Key::LEFT, Direction::WEST },
            { Key::RIGHT, Direction::EAST },
        };

        auto it = key_direction_map.find(event.key);
        if (it == key_direction_map.end()) {
            return;
        }

        const tmt::Direction direction = it->second;
        const bool swapped = next_button(direction);
        if (swapped) {
            event.handled = true;
            return;
        }
    }

    /* Handle component input... */
    if (selected_entity == entt::null || call_next_button == false) {
        return;
    }

    if (auto* slider = engine.ecs.try_get_component<Slider>(selected_entity)) {
        if (event.key == Key::RIGHT) {
            slider->value = std::min(slider->value + slider->step, slider->max);
            event.handled = true;
        } else if (event.key == Key::LEFT) {
            slider->value = std::max(slider->value - slider->step, slider->min);
            event.handled = true;
        }
    }
}

void UIElementManager::on_gamepad_button(GamepadButtonEvent& event) {
    if (event.state == GamepadButtonState::JUST_PRESSED) {
        handle_just_pressed_controller(event);
    } else if (event.state == GamepadButtonState::PRESSED) {
        handle_pressed_controller(event);
    } else if (event.state == GamepadButtonState::JUST_RELEASED) {
        move_state = MoveState::IDLE;
        switch (event.button) {
            case GamepadButton::SOUTH:
                set_state(ButtonState::ON_RELEASE);
                hold_time = 0.0f;
                event.handled = true;
                break;
            default:
                break;
        }
    }
}

void UIElementManager::handle_pressed_controller(tmt::GamepadButtonEvent& event) {
    if (move_state == MoveState::IDLE) {
        move_state = MoveState::INITIAL_MOVE;
    }

    bool call_next_button = false;
    if (move_state == MoveState::INITIAL_MOVE) {
        if (hold_move_duration > INITIAL_DELAY) {
            move_state = MoveState::REPEAT_MOVE;
            hold_move_duration = 0.0f;
            call_next_button = true;
        }
    } else if (move_state == MoveState::REPEAT_MOVE) {
        if (hold_move_duration > REPEAT_DELAY) {
            hold_move_duration = 0.0f;
            call_next_button = true;
        }
    }

    if (event.button == GamepadButton::SOUTH) {
        set_state(ButtonState::ON_HOLD);
        event.handled = true;
        return;
    }

    static std::map<tmt::GamepadButton, tmt::Direction> key_direction_map = {
        { GamepadButton::DPAD_UP, Direction::NORTH },
        { GamepadButton::DPAD_DOWN, Direction::SOUTH },
        { GamepadButton::DPAD_LEFT, Direction::WEST },
        { GamepadButton::DPAD_RIGHT, Direction::EAST },
    };

    auto it = key_direction_map.find(event.button);

    if (it == key_direction_map.end()) {
        return;
    }

    const tmt::Direction direction = it->second;
    const bool swapped = next_button(direction);
    if (swapped) {
        event.handled = true;
        return;
    }

    /* Handle component input... */
    if (selected_entity == entt::null) {
        return;
    }

    if (auto* slider = engine.ecs.try_get_component<Slider>(selected_entity)) {
        if (event.button == GamepadButton::DPAD_RIGHT) {
            slider->value = std::min(slider->value + slider->step, slider->max);
            event.handled = true;
        } else if (event.button == GamepadButton::DPAD_LEFT) {
            slider->value = std::max(slider->value - slider->step, slider->min);
            event.handled = true;
        }
    }
}

void UIElementManager::handle_just_pressed_controller(tmt::GamepadButtonEvent& event) {
    static std::map<tmt::GamepadButton, tmt::Direction> key_direction_map = {
        { GamepadButton::DPAD_UP, Direction::NORTH },
        { GamepadButton::DPAD_DOWN, Direction::SOUTH },
        { GamepadButton::DPAD_LEFT, Direction::WEST },
        { GamepadButton::DPAD_RIGHT, Direction::EAST },
    };

    if (event.button == GamepadButton::SOUTH) {
        set_state(ButtonState::ON_CLICK);
        event.handled = true;
        return;
    }

    auto it = key_direction_map.find(event.button);
    if (it == key_direction_map.end()) {
        return;
    }

    const tmt::Direction direction = it->second;
    const bool swapped = next_button(direction);
    if (swapped) {
        event.handled = true;
        return;
    }

    /* Handle component input... */
    if (selected_entity == entt::null) {
        return;
    }

    if (auto* slider = engine.ecs.try_get_component<Slider>(selected_entity)) {
        if (event.button == GamepadButton::DPAD_RIGHT) {
            slider->value = std::min(slider->value + slider->step, slider->max);
            event.handled = true;
        } else if (event.button == GamepadButton::DPAD_LEFT) {
            slider->value = std::max(slider->value - slider->step, slider->min);
            event.handled = true;
        }
    }
}

void UIElementManager::on_gamepad_axis(GamepadAxisEvent& event) {
    if (event.axis != GamepadAxis::LEFT_X && event.axis != GamepadAxis::LEFT_Y) return;

    // Get both axis values - we need to check the combined stick position
    float x_value = (event.axis == GamepadAxis::LEFT_X) ? event.value : 0.0f;
    float y_value = (event.axis == GamepadAxis::LEFT_Y) ? event.value : 0.0f;

    float distance = std::sqrt(x_value * x_value + y_value * y_value);
    if (std::abs(distance) < MIN_AXIS_VALUE) {
        move_state = MoveState::IDLE;
        return;
    }

    bool call_next_button = false;
    if (move_state == MoveState::IDLE) {
        move_state = MoveState::INITIAL_MOVE;
        hold_move_duration = 0.0f;
        call_next_button = true;
    }
    if (move_state == MoveState::INITIAL_MOVE) {
        if (hold_move_duration > INITIAL_DELAY) {
            move_state = MoveState::REPEAT_MOVE;
            hold_move_duration = 0.0f;
            call_next_button = true;
        }
    } else if (move_state == MoveState::REPEAT_MOVE) {
        if (hold_move_duration > REPEAT_DELAY) {
            hold_move_duration = 0.0f;
            call_next_button = true;
        }
    }

    if (call_next_button) {
        bool is_horizontal = std::abs(x_value) > std::abs(y_value);
        if (is_horizontal) {
            if (x_value > MIN_AXIS_VALUE) {
                next_button(Direction::EAST);
            } else if (x_value < -MIN_AXIS_VALUE) {
                next_button(Direction::WEST);
            }
        } else {
            if (y_value > MIN_AXIS_VALUE) {
                next_button(Direction::SOUTH);
            } else if (y_value < -MIN_AXIS_VALUE) {
                next_button(Direction::NORTH);
            }
        }

        /* Handle component input... */
        if (selected_entity == entt::null) {
            return;
        }

        if (auto* slider = engine.ecs.try_get_component<Slider>(selected_entity)) {
            if (is_horizontal) {
                if (x_value > MIN_AXIS_VALUE) {
                    slider->value = std::min(slider->value + slider->step, slider->max);
                } else if (x_value < -MIN_AXIS_VALUE) {
                    slider->value = std::max(slider->value - slider->step, slider->min);
                }
            } else {
                if (y_value > MIN_AXIS_VALUE) {
                    slider->value = std::min(slider->value + slider->step, slider->max);
                } else if (y_value < -MIN_AXIS_VALUE) {
                    slider->value = std::max(slider->value - slider->step, slider->min);
                }
            }
        }
    }
}

}  // namespace tmt