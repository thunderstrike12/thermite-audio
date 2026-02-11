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
    float deadzone = 0.5f;
    float time_since_being_pressed = 0.0f;
    float sensitivity = 1.0f;
    std::vector<std::unique_ptr<InputEvent>> events;
};

class InputMap {
   public:
    /// <summary>
    /// Creates a new empty action with the given name.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    void add_action(const std::string& name);

    /// <summary>
    /// Creates an action and binds multiple keyboard keys to it.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="keys">Variadic list of Key values to bind.</param>
    template <typename... Keys>
    void add_action_keys(const std::string& name, Keys... keys) {
        auto& action = actions[name];
        (action.events.push_back(std::make_unique<InputEventKey>(keys)), ...);
    }

    /// <summary>
    /// Creates an action and binds multiple gamepad buttons to it.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="buttons">Variadic list of GamepadButton values to bind.</param>
    template <typename... Buttons>
    void add_action_gamepad_buttons(const std::string& name, Buttons... buttons) {
        auto& action = actions[name];
        (action.events.push_back(std::make_unique<InputEventGamepadButton>(buttons)), ...);
    }

    /// <summary>
    /// Creates an action and binds multiple gamepad axes to it.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="axes">Variadic list of GamepadAxisDirection values specifying axis and direction.</param>
    template <std::same_as<GamepadAxisDirection>... Args>
    void add_action_gamepad_axes(const std::string& name, Args... axes) {
        auto& action = actions[name];
        (action.events.push_back(std::make_unique<InputEventGamepadMotion>(axes.axis, axes.negative)), ...);
    }

    /// <summary>
    /// Adds a custom input event to an existing action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="event">The input event. Ownership is transferred to the InputMap.</param>
    void add_action_event(const std::string& name, std::unique_ptr<InputEvent> event);

    /// <summary>
    /// Adds a keyboard key binding to an existing action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="key">The keyboard key to bind.</param>
    void add_key_to_action(const std::string& name, Key key);

    /// <summary>
    /// Adds a mouse button binding to an existing action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="button">The mouse button to bind.</param>
    void add_action_mouse(const std::string& name, MouseButton button);

    /// <summary>
    /// Adds a mouse motion event to an existing action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    void add_action_mouse_motion(const std::string& name);

    /// <summary>
    /// Removes an action and all its associated input events.
    /// </summary>
    /// <param name="name">The action identifier to remove.</param>
    void remove_action(const std::string& name);

    /// <summary>
    /// Returns a const pointer to the action with the given name.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>Pointer to the action, or nullptr if not found.</returns>
    const InputAction* get_action(const std::string& name) const;

    /// <summary>
    /// Returns a mutable pointer to the action with the given name.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>Pointer to the action, or nullptr if not found.</returns>
    InputAction* get_action(const std::string& name);

    /// <summary>
    /// Checks whether an action with the given name exists.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>True if the action exists.</returns>
    bool has_action(const std::string& name) const;

    /// <summary>
    /// Gets the deadzone value for an action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>The deadzone value, or 0.0 if action not found.</returns>
    float get_action_deadzone(const std::string& name) const;

    /// <summary>
    /// Sets the deadzone value for an action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="deadzone">The deadzone threshold (0.0-1.0).</param>
    void set_action_deadzone(const std::string& name, float deadzone);

    /// <summary>
    /// Gets the sensitivity value for an action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>The sensitivity multiplier, or 1.0 if action not found.</returns>
    float get_action_sensitivity(const std::string& name) const;

    /// <summary>
    /// Sets the sensitivity value for an action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="sensitivity">The sensitivity multiplier.</param>
    void set_action_sensitivity(const std::string& name, float sensitivity);

   private:
    friend class Input;
    void setup_default_actions();
    std::unordered_map<std::string, InputAction> actions {};
};

}  // namespace tmt
