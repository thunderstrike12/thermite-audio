#include "input_event.hpp"

#include "engine/engine.hpp"
#include "engine/core/input/input.hpp"

bool tmt::InputEventKey::is_pressed() const { return engine.input.is_keyboard_button_pressed(key); }
bool tmt::InputEventKey::is_just_pressed() const { return engine.input.is_keyboard_button_just_pressed(key); }
bool tmt::InputEventKey::is_just_released() const { return engine.input.is_keyboard_button_released(key); }
float tmt::InputEventKey::get_action_strength() const { return is_pressed() == true ? 1.0f : 0.0f; }

bool tmt::InputEventMouseButton::is_pressed() const { return engine.input.is_mouse_button_pressed(button); }
bool tmt::InputEventMouseButton::is_just_pressed() const { return engine.input.is_mouse_button_just_pressed(button); }
bool tmt::InputEventMouseButton::is_just_released() const { return engine.input.is_mouse_button_just_released(button); }
float tmt::InputEventMouseButton::get_action_strength() const { return is_pressed() ? 1.0f : 0.0f; }

float tmt::InputEventMouseMotion::get_position_x() const { return position_x; }
float tmt::InputEventMouseMotion::get_position_y() const { return position_y; }
float tmt::InputEventMouseMotion::get_relative_x() const { return relative_x; }
float tmt::InputEventMouseMotion::get_relative_y() const { return relative_y; }
float tmt::InputEventMouseMotion::get_action_strength() const { return 0.0f; }

bool tmt::InputEventGamepadButton::is_pressed() const { return engine.input.is_gamepad_button_pressed(button, device_id); }
bool tmt::InputEventGamepadButton::is_just_pressed() const { return engine.input.is_gamepad_button_just_pressed(button, device_id); }
bool tmt::InputEventGamepadButton::is_just_released() const { return engine.input.is_gamepad_button_just_released(button, device_id); }
float tmt::InputEventGamepadButton::get_action_strength() const { return is_pressed() ? 1.0f : 0.0f; }

bool tmt::InputEventGamepadMotion::is_pressed() const { return get_action_strength() > 0.0f; }
bool tmt::InputEventGamepadMotion::is_just_pressed() const { return false; }
bool tmt::InputEventGamepadMotion::is_just_released() const { return false; }
float tmt::InputEventGamepadMotion::get_action_strength() const {
    float raw = engine.input.get_gamepad_axis(axis, device_id);
    if (negative) {
        return std::max(0.0f, -raw);
    }
    return std::max(0.0f, raw);
}

float tmt::InputEventGamepadMotion::get_axis_value() const { return std::abs(engine.input.get_gamepad_axis(axis, device_id)); }
