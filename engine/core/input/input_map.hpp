#pragma once
#include "input_event.hpp"

#include <memory>
#include <string>

namespace tmt {
namespace action {
constexpr auto CONFIRM = "confirm";
constexpr auto CANCEL = "cancel";
constexpr auto LEFT_CLICK = "left_click";
constexpr auto RIGHT_CLICK = "right_click";
constexpr auto MIDDLE_CLICK = "middle_click";
constexpr auto MOUSE_MOTION = "mouse_motion";
constexpr auto MOVE_UP = "move_up";
constexpr auto MOVE_DOWN = "move_down";
constexpr auto MOVE_RIGHT = "move_right";
constexpr auto MOVE_LEFT = "move_left";
constexpr auto LOOK_UP = "Look_up";
constexpr auto LOOK_DOWN = "Look_down";
constexpr auto LOOK_RIGHT = "Look_right";
constexpr auto LOOK_LEFT = "Look_left";

constexpr auto LEFT_TRIGGER = "left_trigger";
constexpr auto RIGHT_TRIGGER = "right_trigger";
}  // namespace action
struct InputAction {
    std::vector<std::unique_ptr<InputEvent>> events;
    float deadzone = 0.5f;
};
class InputMap {
   public:
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
    template <std::same_as<GamepadAxisDirection>... Args>
    void add_action_gamepad_axes(const std::string& name, Args... axes) {
        auto& action = actions[name];
        (action.events.push_back(std::make_unique<InputEventGamepadMotion>(axes.axis, axes.negative)), ...);
    }
    void add_action_event(const std::string& name, std::unique_ptr<InputEvent> event);
    void add_key_to_action(const std::string& name, Key key);
    void add_action_mouse(const std::string& name, MouseButton button);
    void add_action_mouse_motion(const std::string& name);
    void remove_action(const std::string& name);

    const InputAction* get_action(std::string_view name) const;
    bool has_action(std::string_view name) const;
    float get_action_deadzone(std::string_view name) const;
    void set_action_deadzone(std::string_view name, float deadzone);

   private:
    // to not break the whole codebase, these are still tighly coupled
    friend class Input;
    void setup_default_actions();

    std::unordered_map<std::string, InputAction> actions {};
};
}  // namespace tmt
