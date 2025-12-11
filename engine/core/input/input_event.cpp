#include "input_event.hpp"

#include "engine/engine.hpp"
#include "engine/core/input/input.hpp"
bool tmt::InputEventKey::is_pressed() const { return engine.input.is_keyboard_button_pressed(key); }
bool tmt::InputEventKey::is_just_pressed() const { return engine.input.is_keyboard_button_just_pressed(key); }
bool tmt::InputEventKey::is_just_released() const { return engine.input.is_keyboard_button_released(key); }
bool tmt::InputEventMouseButton::is_pressed() const { return engine.input.is_mouse_button_pressed(button); }
bool tmt::InputEventMouseButton::is_just_pressed() const { return engine.input.is_mouse_button_just_pressed(button); }
bool tmt::InputEventMouseButton::is_just_released() const { return engine.input.is_mouse_button_just_released(button); }

bool tmt::InputEventMouseMotion::is_pressed() const { return false; }
bool tmt::InputEventMouseMotion::is_just_pressed() const { return false; }
bool tmt::InputEventMouseMotion::is_just_released() const { return false; }

float tmt::InputEventMouseMotion::get_position_x() const { return position_x; }
float tmt::InputEventMouseMotion::get_position_y() const { return position_y; }
float tmt::InputEventMouseMotion::get_relative_x() const { return relative_x; }
float tmt::InputEventMouseMotion::get_relative_y() const { return relative_y; }

bool tmt::InputEventGamepadButton::is_pressed() const { return engine.input.is_gamepad_button_pressed(button, device_id); }
bool tmt::InputEventGamepadButton::is_just_pressed() const { return engine.input.is_gamepad_button_just_pressed(button, device_id); }
bool tmt::InputEventGamepadButton::is_just_released() const { return engine.input.is_gamepad_button_just_released(button, device_id); }

bool tmt::InputEventGamepadMotion::is_pressed() const { return std::abs(get_axis_value()) > 0.5f; }
bool tmt::InputEventGamepadMotion::is_just_pressed() const { return false; }
bool tmt::InputEventGamepadMotion::is_just_released() const { return false; }
float tmt::InputEventGamepadMotion::get_axis_value() const { return engine.input.get_gamepad_axis(axis, device_id); }
