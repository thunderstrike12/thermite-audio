#include "input_event.hpp"

#include "engine/engine.hpp"
#include "engine/core/input.hpp"
bool tmt::InputEventKey::is_pressed() const { return engine.input.is_keyboard_button_pressed(key); }
bool tmt::InputEventKey::is_just_pressed() const { return engine.input.is_keyboard_button_just_pressed(key); }
bool tmt::InputEventKey::is_just_released() const { return engine.input.is_keyboard_button_released(key); }
bool tmt::InputEventMouseButton::is_pressed() const { return engine.input.is_mouse_button_pressed(button); }
bool tmt::InputEventMouseButton::is_just_pressed() const { return engine.input.is_mouse_button_just_pressed(button); }
bool tmt::InputEventMouseButton::is_just_released() const { return engine.input.is_mouse_button_just_released(button); }

bool tmt::InputEventMouse::is_pressed() const { return false; }
bool tmt::InputEventMouse::is_just_pressed() const { return false; }
bool tmt::InputEventMouse::is_just_released() const { return false; }

float tmt::InputEventMouse::get_position_x() const { return position_x; }
float tmt::InputEventMouse::get_position_y() const { return position_y; }
float tmt::InputEventMouse::get_relative_x() const { return relative_x; }
float tmt::InputEventMouse::get_relative_y() const { return relative_y; }
