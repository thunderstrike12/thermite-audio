#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <array>

#include "keys.hpp"
#include "input_event.hpp"
struct SDL_Gamepad;

namespace tmt {
namespace action {
constexpr const char* CONFIRM = "confirm";
constexpr const char* CANCEL = "cancel";
constexpr const char* LEFT_CLICK = "left_click";
constexpr const char* RIGHT_CLICK = "right_click";
constexpr const char* MIDDLE_CLICK = "middle_click";
constexpr const char* MOUSE_MOTION = "mouse_motion";
constexpr auto GAMEPAD_LEFT_STICK = "gamepad_left_stick";
constexpr auto GAMEPAD_RIGHT_STICK = "gamepad_right_stick";
constexpr auto GAMEPAD_TRIGGERS = "gamepad_triggers";
}  // namespace action
struct InputAction {
    std::vector<std::unique_ptr<InputEvent>> events;
};
struct GamepadState {
    SDL_Gamepad* handle {nullptr};
    std::string name {};
    std::array<bool, static_cast<int32_t>(GamepadButton::COUNT)> buttons {};
    std::array<bool, static_cast<int32_t>(GamepadButton::COUNT)> prev_buttons {};
    std::array<float, static_cast<int32_t>(GamepadAxis::COUNT)> axes {};
};
class Input {
   public:
    // Rule of 0

    void init();
    void update();

    void add_action(const std::string& name);
    template <typename... Keys>
    void add_action_keys(const std::string& name, Keys... keys) {
        auto& action = actions[name];
        (action.events.push_back(std::make_unique<InputEventKey>(keys)), ...);
    }
    template <typename... Buttons>
    void add_action_gamepad_buttons(const std::string& name, Buttons... buttons) {
        auto& action = actions[name];
        (action.events.push_back(std::make_unique<InputEventGamepadButton>(buttons)), ...);
    }
    template <typename... Axes>
    void add_action_gamepad_axes(const std::string& name, Axes... axes) {
        auto& action = actions[name];
        (action.events.push_back(std::make_unique<InputEventGamepadMotion>(axes)), ...);
    }
    void add_action_event(const std::string& name, std::unique_ptr<InputEvent> event);

    void add_key_to_action(const std::string& name, Key key);
    void add_action_mouse(const std::string& name, MouseButton button);
    void remove_action(const std::string& name);
    void add_action_mouse_motion(const std::string& name);

    bool is_action_pressed(std::string_view name) const;
    bool is_action_just_pressed(std::string_view name) const;
    bool is_action_just_released(std::string_view name) const;

    bool is_keyboard_button_just_pressed(Key key) const;
    bool is_keyboard_button_pressed(Key key) const;
    bool is_keyboard_button_released(Key key) const;

    bool is_mouse_button_pressed(MouseButton button) const;
    bool is_mouse_button_just_pressed(MouseButton button) const;
    bool is_mouse_button_just_released(MouseButton button) const;

    float get_mouse_x() const { return mouse_x; }
    float get_mouse_y() const { return mouse_y; }
    float get_mouse_wheel_x() const { return scroll_dx; }
    float get_mouse_wheel_y() const { return scroll_dy; }
    float get_mouse_delta_x() const { return mouse_dx; }
    float get_mouse_delta_y() const { return mouse_dy; }

    void set_mouse_relative_to_window(bool value);
    bool get_mouse_relative_to_window();

    // Gamepad
    bool is_gamepad_button_pressed(GamepadButton button, int32_t device_id = -1) const;
    bool is_gamepad_button_just_pressed(GamepadButton button, int32_t device_id = -1) const;
    bool is_gamepad_button_just_released(GamepadButton button, int32_t device_id = -1) const;
    float get_gamepad_axis(GamepadAxis axis, int32_t device_id = -1) const;
    const char* get_gamepad_button_name(GamepadButton button, int32_t device_id = -1) const;
    const char* get_gamepad_axis_name(GamepadAxis axis) const;
    int32_t get_default_gamepad_id() const;
    void lock_mouse(bool value) const;

   private:
    void setup_default_action();
    // do not free this manually
    const bool* keys_sdl = nullptr;
    uint32_t mouse_buttons = 0u;
    uint32_t prev_mouse_buttons = 0u;
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;

    // relative
    float mouse_dx = 0.0f;
    float mouse_dy = 0.0f;

    float scroll_dx = 0.0f;
    float scroll_dy = 0.0f;

    std::vector<bool> prev_keys {};
    std::unordered_map<std::string, InputAction> actions {};
    std::unordered_map<int32_t, GamepadState> gamepads {};
};
}  // namespace tmt
