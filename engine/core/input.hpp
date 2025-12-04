#pragma once
#include <string>
#include <memory>
#include <unordered_map>

#include "keys.hpp"
#include "engine/events/input_event.hpp"

namespace tmt {
namespace action {
constexpr const char* CONFIRM = "confirm";
constexpr const char* CANCEL = "cancel";
constexpr const char* LEFT_CLICK = "left_click";
constexpr const char* RIGHT_CLICK = "right_click";
constexpr const char* MIDDLE_CLICK = "middle_click";
constexpr const char* MOUSE_MOTION = "mouse_motion";
}  // namespace action
struct InputAction {
    std::vector<std::unique_ptr<InputEvent>> events;
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
    void add_action_event(const std::string& name, std::unique_ptr<InputEvent> event);

    void add_key_to_action(const std::string& name, Key key);
    void add_action_mouse(const std::string& name, MouseButton button);
    void remove_action(const std::string& name);
    void add_action_mouse_motion(const std::string& name);

    bool is_action_pressed(const std::string& name) const;
    bool is_action_just_pressed(const std::string& name) const;
    bool is_action_just_released(const std::string& name) const;

    bool is_keyboard_button_just_pressed(Key key) const;
    bool is_keyboard_button_pressed(Key key) const;
    bool is_keyboard_button_released(Key key) const;

    bool is_mouse_button_pressed(MouseButton button) const;
    bool is_mouse_button_just_pressed(MouseButton button) const;
    bool is_mouse_button_just_released(MouseButton button) const;

    float get_mouse_x() const { return mouse_x; }
    float get_mouse_y() const { return mouse_y; }
    float get_mouse_delta_x() const { return mouse_dx; }
    float get_mouse_delta_y() const { return mouse_dy; }

    void set_mouse_relative_to_window(bool value);
    bool get_mouse_relative_to_window();

   private:
    void setup_default_action();
    // do not free this manually
    const bool* keys = nullptr;
    uint32_t mouse_buttons = 0u;
    uint32_t prev_mouse_buttons = 0u;
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    // relative positions
    float mouse_dx = 0.0f;
    float mouse_dy = 0.0f;
    std::vector<bool> prev_keys {};
    std::unordered_map<std::string, InputAction> actions {};
};
}  // namespace tmt
