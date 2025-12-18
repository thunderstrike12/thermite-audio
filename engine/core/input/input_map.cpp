#include "input_map.hpp"

using namespace tmt;
void InputMap::setup_default_actions() {
    add_action(action::CONFIRM);
    add_action_keys(action::CONFIRM, Key::SPACE, Key::RETURN);
    add_action_gamepad_buttons(action::CONFIRM, GamepadButton::SOUTH);

    add_action(action::CANCEL);
    add_action_keys(action::CANCEL, Key::ESCAPE);
    add_action_gamepad_buttons(action::CANCEL, GamepadButton::EAST);

    add_action(action::LEFT_CLICK);
    add_action_mouse(action::LEFT_CLICK, MouseButton::LEFT);
    add_action_gamepad_buttons(action::LEFT_CLICK, GamepadButton::RIGHT_SHOULDER);

    add_action(action::RIGHT_CLICK);
    add_action_mouse(action::RIGHT_CLICK, MouseButton::RIGHT);
    add_action_gamepad_buttons(action::RIGHT_CLICK, GamepadButton::LEFT_SHOULDER);

    add_action(action::MIDDLE_CLICK);
    add_action_mouse(action::MIDDLE_CLICK, MouseButton::MIDDLE);
    add_action_gamepad_buttons(action::MIDDLE_CLICK, GamepadButton::RIGHT_STICK);

    add_action(action::MOUSE_MOTION);
    add_action_mouse_motion(action::MOUSE_MOTION);

    // SDL has inputs in opposite direction
    add_action(action::MOVE_LEFT);
    add_action_gamepad_axes(action::MOVE_LEFT, GamepadAxisDirection {.axis = GamepadAxis::LEFT_X, .negative = true});
    add_action_keys(action::MOVE_LEFT, Key::A);

    add_action(action::MOVE_RIGHT);
    add_action_gamepad_axes(action::MOVE_RIGHT, GamepadAxisDirection {.axis = GamepadAxis::LEFT_X});
    add_action_keys(action::MOVE_RIGHT, Key::D);

    add_action(action::MOVE_DOWN);
    add_action_gamepad_axes(action::MOVE_DOWN, GamepadAxisDirection {.axis = GamepadAxis::LEFT_Y});
    add_action_keys(action::MOVE_DOWN, Key::S);

    add_action(action::MOVE_UP);
    add_action_gamepad_axes(action::MOVE_UP, GamepadAxisDirection {.axis = GamepadAxis::LEFT_Y, .negative = true});
    add_action_keys(action::MOVE_UP, Key::W);

    add_action(action::LOOK_LEFT);
    add_action_gamepad_axes(action::LOOK_LEFT, GamepadAxisDirection {.axis = GamepadAxis::RIGHT_X, .negative = true});

    add_action(action::LOOK_RIGHT);
    add_action_gamepad_axes(action::LOOK_RIGHT, GamepadAxisDirection {.axis = GamepadAxis::RIGHT_X});

    add_action(action::LOOK_DOWN);
    add_action_gamepad_axes(action::LOOK_DOWN, GamepadAxisDirection {.axis = GamepadAxis::RIGHT_Y});

    add_action(action::LOOK_UP);
    add_action_gamepad_axes(action::LOOK_UP, GamepadAxisDirection {.axis = GamepadAxis::RIGHT_Y, .negative = true});

    add_action(action::LEFT_TRIGGER);
    add_action_gamepad_axes(action::LEFT_TRIGGER, GamepadAxisDirection {.axis = GamepadAxis::LEFT_TRIGGER});

    add_action(action::RIGHT_TRIGGER);
    add_action_gamepad_axes(action::RIGHT_TRIGGER, GamepadAxisDirection {.axis = GamepadAxis::RIGHT_TRIGGER});
}

void InputMap::add_action(const std::string& name) { actions[name] = InputAction {}; }

void InputMap::add_action_event(const std::string& name, std::unique_ptr<InputEvent> event) { actions[name].events.push_back(std::move(event)); }

void InputMap::add_key_to_action(const std::string& name, Key key) { add_action_event(name, std::make_unique<InputEventKey>(key)); }

void InputMap::add_action_mouse(const std::string& name, MouseButton button) { add_action_event(name, std::make_unique<InputEventMouseButton>(button)); }

void InputMap::add_action_mouse_motion(const std::string& name) { add_action_event(name, std::make_unique<InputEventMouseMotion>()); }

void InputMap::remove_action(const std::string& name) { actions.erase(name); }

const InputAction* InputMap::get_action(std::string_view name) const {
    auto it = actions.find(std::string {name});
    if (it == actions.end()) return nullptr;
    return &it->second;
}

bool InputMap::has_action(std::string_view name) const { return actions.find(std::string {name}) != actions.end(); }

float InputMap::get_action_deadzone(std::string_view name) const {
    const auto* action = get_action(name);
    return action ? action->deadzone : 0.0f;
}
