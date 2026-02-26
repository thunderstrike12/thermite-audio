#include "button_manager.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/components/button.hpp"
#include "engine/core/components/image_renderer.hpp"
#include "engine/core/components/ui_component.hpp"
#include "engine/core/components/transform.hpp"
#include <cmath>

namespace tmt {

void ButtonManager::on_start() {
    auto view = engine.ecs.get_registry().view<Button>();
    for (auto [entity, button] : view.each()) {
        button.state = ButtonState::IDLE;
    }
    set_default_button();
}

void ButtonManager::on_update(const tmt::FrameData& time) {
    update_move_state(time.delta_time);
    update_states(time);
}

void ButtonManager::update_states(const tmt::FrameData& time) {
    /* group with buttons and image renderers */
    auto group = engine.ecs.get_registry().group<Button>(entt::get<ImageRenderer>);
    for (const auto& [entity, button, image_renderer] : group.each()) {
        image_renderer.color = button.colors.at(static_cast<uint8_t>(button.state));

        switch (button.state) {
            case ButtonState::IDLE:
                break;
            case ButtonState::ON_SELECT:
                button.on_select();
                button.state = ButtonState::SELECTED;
                break;
            case ButtonState::SELECTED:
                button.on_selected();
                break;
            case ButtonState::ON_DESELECT:
                button.on_deselect();
                button.state = ButtonState::IDLE;
                break;
            case ButtonState::DISABLED:
                break;
            case ButtonState::ON_CLICK:
                button.on_click();
                button.state = ButtonState::ON_HOLD;
                break;
            case ButtonState::ON_HOLD:
                button.on_hold(time.delta_time);
                hold_time += time.delta_time;
                break;
            case ButtonState::ON_RELEASE:
                button.on_release();
                button.state = ButtonState::SELECTED;
                hold_time = 0.0f;
                break;
        }
    }
}

void ButtonManager::on_end() {}

void ButtonManager::select_button(const Entity button) {
    if (selected_button == button) return;

    /* Deselect previous button */
    if (selected_button != entt::null) {
        auto* prev_button = engine.ecs.get_registry().try_get<Button>(selected_button);
        if (prev_button) prev_button->state = ButtonState::ON_DESELECT;
        previous_button = selected_button;
    }

    if (previous_button == button) previous_button = entt::null;
    selected_button = button;

    /* Select new button */
    if (selected_button == entt::null) return;

    auto* new_button = engine.ecs.get_registry().try_get<Button>(selected_button);
    if (new_button) new_button->state = ButtonState::ON_SELECT;
}

void ButtonManager::next_button(Direction direction) {
    if (selected_button == entt::null) {
        set_default_button();
        return;
    }

    auto* button = engine.ecs.get_registry().try_get<Button>(selected_button);
    if (!button) {
        set_default_button();
        return;
    }

    int dir = static_cast<int>(direction);
    if (dir < 0 || dir >= static_cast<int>(button->flow_direction.size())) return;

    Entity next_entity = button->flow_direction[dir];
    if (next_entity == entt::null) return;

    select_button(next_entity);
}

void ButtonManager::set_default_button() {
    if (previous_button != entt::null) {
        select_button(previous_button);
        return;
    }

    auto view = engine.ecs.get_registry().view<Button>();
    /* set top-left most button as first */
    Entity default_button = entt::null;
    float min_distance = std::numeric_limits<float>::max();
    for (auto [entity, button] : view.each()) {
        auto* transform = engine.ecs.get_registry().try_get<Transform>(entity);
        if (!transform) continue;
        const auto& pos = transform->get_world_position();
        float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
        if (distance < min_distance) {
            min_distance = distance;
            default_button = entity;
        }
    }

    if (default_button != entt::null) {
        select_button(default_button);
    }
}

void ButtonManager::set_state(ButtonState state) {
    if (selected_button == entt::null) return;

    auto* button = engine.ecs.get_registry().try_get<Button>(selected_button);
    if (!button) return;
    button->state = state;
}

void ButtonManager::update_move_state(float dt) {
    if (move_state == MoveState::INITIAL_MOVE || move_state == MoveState::REPEAT_MOVE) {
        hold_move_duration += dt;
    } else if (move_state == MoveState::IDLE) {
        hold_move_duration = 0.0f;
    }
}

void ButtonManager::on_mouse_move(MouseMoveEvent& event) {
    const auto mouse_pos = glm::vec2(event.x, event.y);
    auto view = engine.ecs.get_registry().view<Button>();
    for (auto [entity, button] : view.each()) {
        if (AnchorHelper::is_inside(entity, mouse_pos)) {
            select_button(entity);
            return;
        }
    }
    select_button(entt::null);
}

void ButtonManager::on_mouse_button(MouseButtonEvent& event) {
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

void ButtonManager::on_keyboard(KeyboardEvent& event) {
    if (event.state == KeyState::JUST_PRESSED) {
        switch (event.key) {
            case Key::UP:
                next_button(Direction::NORTH);
                event.handled = true;
                break;
            case Key::DOWN:
                next_button(Direction::SOUTH);
                event.handled = true;
                break;
            case Key::LEFT:
                next_button(Direction::WEST);
                event.handled = true;
                break;
            case Key::RIGHT:
                next_button(Direction::EAST);
                event.handled = true;
                break;
            case Key::RETURN:
                set_state(ButtonState::ON_CLICK);
                event.handled = true;
                break;
            default:
                break;
        }
    } else if (event.state == KeyState::PRESSED) {
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
            switch (event.key) {
                case Key::UP:
                    next_button(Direction::NORTH);
                    event.handled = true;
                    break;
                case Key::DOWN:
                    next_button(Direction::SOUTH);
                    event.handled = true;
                    break;
                case Key::LEFT:
                    next_button(Direction::WEST);
                    event.handled = true;
                    break;
                case Key::RIGHT:
                    next_button(Direction::EAST);
                    event.handled = true;
                    break;
                case Key::RETURN:
                    set_state(ButtonState::ON_HOLD);
                    event.handled = true;
                    break;
                default:
                    break;
            }
        }
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

void ButtonManager::on_gamepad_button(GamepadButtonEvent& event) {
    if (event.state == GamepadButtonState::JUST_PRESSED) {
        switch (event.button) {
            case GamepadButton::DPAD_UP:
                next_button(Direction::NORTH);
                event.handled = true;
                break;
            case GamepadButton::DPAD_DOWN:
                next_button(Direction::SOUTH);
                event.handled = true;
                break;
            case GamepadButton::DPAD_LEFT:
                next_button(Direction::WEST);
                event.handled = true;
                break;
            case GamepadButton::DPAD_RIGHT:
                next_button(Direction::EAST);
                event.handled = true;
                break;
            case GamepadButton::SOUTH:
                set_state(ButtonState::ON_CLICK);
                event.handled = true;
                break;
            default:
                break;
        }
    } else if (event.state == GamepadButtonState::PRESSED) {
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
            switch (event.button) {
                case GamepadButton::DPAD_UP:
                    next_button(Direction::NORTH);
                    event.handled = true;
                    break;
                case GamepadButton::DPAD_DOWN:
                    next_button(Direction::SOUTH);
                    event.handled = true;
                    break;
                case GamepadButton::DPAD_LEFT:
                    next_button(Direction::WEST);
                    event.handled = true;
                    break;
                case GamepadButton::DPAD_RIGHT:
                    next_button(Direction::EAST);
                    event.handled = true;
                    break;
                case GamepadButton::SOUTH:
                    set_state(ButtonState::ON_HOLD);
                    event.handled = true;
                    break;
                default:
                    break;
            }
        }
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

void ButtonManager::on_gamepad_axis(GamepadAxisEvent& event) {
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
    }
}

}  // namespace tmt