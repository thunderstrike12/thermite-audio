#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <array>
#include <glm/glm.hpp>
#include "keys.hpp"
#include "input_event.hpp"

// heavily inspired by Godot's input system
struct SDL_Gamepad;
namespace tmt {

struct FrameData;

struct GamepadState {
    SDL_Gamepad* handle { nullptr };
    std::string name {};
    std::array<bool, static_cast<int32_t>(GamepadButton::COUNT)> buttons {};
    std::array<bool, static_cast<int32_t>(GamepadButton::COUNT)> prev_buttons {};
    std::array<float, static_cast<int32_t>(GamepadAxis::COUNT)> axes {};
};

class Input {
   public:
    // Rule of 0

    /// <summary>
    /// Initializes the input system and retrieves the SDL keyboard state.
    /// </summary>
    void init();

    /// <summary>
    /// Updates input state. Should be called once per frame.
    /// Stores previous frame's key states and polls gamepad input.
    /// </summary>
    /// <param name="time">Frame timing data.</param>
    void update(const tmt::FrameData& time);

    /// <summary>
    /// Returns the strength of an action after applying deadzone and sensitivity.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>Value between 0.0 and sensitivity (typically 0.0-1.0). Returns 0.0 if action not found.</returns>
    float get_action_strength(const std::string& name);

    /// <summary>
    /// Returns the raw strength of an action without deadzone or sensitivity applied.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>Raw value between 0.0-1.0. Returns 0.0 if action not found.</returns>
    float get_action_raw_strength(const std::string& name);

    /// <summary>
    /// Returns a 1D axis value from two opposing actions.
    /// </summary>
    /// <param name="negative_action">Action that contributes negative values.</param>
    /// <param name="positive_action">Action that contributes positive values.</param>
    /// <returns>Value between -1.0 and 1.0.</returns>
    float get_axis(const std::string& negative_action, const std::string& positive_action);

    /// <summary>
    /// Constructs a 2D vector from four directional actions with circular deadzone normalization.
    /// </summary>
    /// <param name="negative_action_x">Left direction action.</param>
    /// <param name="positive_action_x">Right direction action.</param>
    /// <param name="negative_action_y">Down direction action.</param>
    /// <param name="positive_action_y">Up direction action.</param>
    /// <param name="deadzone">Custom deadzone. If negative, averages the deadzone from all four actions.</param>
    /// <returns>Normalized vector with magnitude between 0.0 and 1.0.</returns>
    glm::vec2 get_vector(
        const std::string& negative_action_x, const std::string& positive_action_x, const std::string& negative_action_y, const std::string& positive_action_y, float deadzone = -1.0f
    );

    /// <summary>
    /// Returns true only on the first frame a key is pressed.
    /// </summary>
    /// <param name="key">The keyboard key to check.</param>
    /// <returns>True if the key was just pressed this frame.</returns>
    bool is_keyboard_button_just_pressed(Key key) const;

    /// <summary>
    /// Returns true while a key is held down.
    /// </summary>
    /// <param name="key">The keyboard key to check.</param>
    /// <returns>True if the key is currently pressed.</returns>
    bool is_keyboard_button_pressed(Key key) const;

    /// <summary>
    /// Returns true only on the first frame a key is released.
    /// </summary>
    /// <param name="key">The keyboard key to check.</param>
    /// <returns>True if the key was just released this frame.</returns>
    bool is_keyboard_button_released(Key key) const;

    /// <summary>
    /// Returns true while a mouse button is held down.
    /// </summary>
    /// <param name="button">The mouse button to check.</param>
    /// <returns>True if the button is currently pressed.</returns>
    bool is_mouse_button_pressed(MouseButton button) const;

    /// <summary>
    /// Returns true only on the first frame a mouse button is pressed.
    /// </summary>
    /// <param name="button">The mouse button to check.</param>
    /// <returns>True if the button was just pressed this frame.</returns>
    bool is_mouse_button_just_pressed(MouseButton button) const;

    /// <summary>
    /// Returns true only on the first frame a mouse button is released.
    /// </summary>
    /// <param name="button">The mouse button to check.</param>
    /// <returns>True if the button was just released this frame.</returns>
    bool is_mouse_button_just_released(MouseButton button) const;

    /// <summary>
    /// Returns the mouse cursor's X position in window coordinates.
    /// </summary>
    /// <returns>X position in pixels.</returns>
    float get_mouse_x() const;

    /// <summary>
    /// Returns the mouse cursor's Y position in window coordinates.
    /// </summary>
    /// <returns>Y position in pixels.</returns>
    float get_mouse_y() const;

    /// <summary>
    /// Returns the mouse cursor's position as a 2D vector.
    /// </summary>
    /// <returns>Mouse position vector.</returns>
    glm::vec2 get_mouse_position() const { return glm::vec2 { mouse_x, mouse_y }; }

    /// <summary>
    /// Returns horizontal scroll wheel delta for this frame.
    /// </summary>
    /// <returns>Horizontal scroll amount.</returns>
    float get_mouse_wheel_x() const;

    /// <summary>
    /// Returns vertical scroll wheel delta for this frame.
    /// </summary>
    /// <returns>Vertical scroll amount.</returns>
    float get_mouse_wheel_y() const;

    /// <summary>
    /// Returns mouse movement delta X since last frame.
    /// </summary>
    /// <returns>Horizontal movement in pixels.</returns>
    float get_mouse_delta_x() const;

    /// <summary>
    /// Returns mouse movement delta Y since last frame.
    /// </summary>
    /// <returns>Vertical movement in pixels.</returns>
    float get_mouse_delta_y() const;

    /// <summary>
    /// Enables or disables relative mouse mode (cursor hidden, infinite movement).
    /// </summary>
    /// <param name="value">True to enable relative mode, false to disable.</param>
    void set_mouse_relative_to_window(bool value);

    /// <summary>
    /// Returns true if relative mouse mode is enabled.
    /// </summary>
    /// <returns>True if relative mouse mode is active.</returns>
    bool get_mouse_relative_to_window();

    /// <summary>
    /// Returns true while a gamepad button is held.
    /// </summary>
    /// <param name="button">The gamepad button to check.</param>
    /// <param name="device_id">Specific gamepad ID, or -1 for any connected gamepad.</param>
    /// <returns>True if the button is currently pressed.</returns>
    bool is_gamepad_button_pressed(GamepadButton button, int32_t device_id = -1) const;

    /// <summary>
    /// Returns true only on the first frame a gamepad button is pressed.
    /// </summary>
    /// <param name="button">The gamepad button to check.</param>
    /// <param name="device_id">Specific gamepad ID, or -1 for any connected gamepad.</param>
    /// <returns>True if the button was just pressed this frame.</returns>
    bool is_gamepad_button_just_pressed(GamepadButton button, int32_t device_id = -1) const;

    /// <summary>
    /// Returns true only on the first frame a gamepad button is released.
    /// </summary>
    /// <param name="button">The gamepad button to check.</param>
    /// <param name="device_id">Specific gamepad ID, or -1 for any connected gamepad.</param>
    /// <returns>True if the button was just released this frame.</returns>
    bool is_gamepad_button_just_released(GamepadButton button, int32_t device_id = -1) const;

    /// <summary>
    /// Returns the current value of a gamepad axis.
    /// </summary>
    /// <param name="axis">The axis to query.</param>
    /// <param name="device_id">Specific gamepad ID, or -1 for any connected gamepad.</param>
    /// <returns>Value between -1.0 and 1.0.</returns>
    float get_gamepad_axis(GamepadAxis axis, int32_t device_id = -1) const;

    /// <summary>
    /// Returns a human-readable name for a gamepad button.
    /// </summary>
    /// <param name="button">The button to get the name for.</param>
    /// <param name="device_id">Specific gamepad ID, or -1 for default.</param>
    /// <returns>Button name string (e.g., "A", "Cross").</returns>
    const char* get_gamepad_button_name(GamepadButton button, int32_t device_id = -1) const;

    /// <summary>
    /// Returns a human-readable name for a gamepad axis.
    /// </summary>
    /// <param name="axis">The axis to get the name for.</param>
    /// <returns>Axis name string (e.g., "Left Stick X").</returns>
    const char* get_gamepad_axis_name(GamepadAxis axis) const;

    /// <summary>
    /// Returns the device ID of the first connected gamepad.
    /// </summary>
    /// <returns>Device ID, or -1 if no gamepad is connected.</returns>
    int32_t get_default_gamepad_id() const;

    /// <summary>
    /// Locks or unlocks the mouse cursor to the window.
    /// </summary>
    /// <param name="value">True to lock, false to unlock.</param>
    void lock_mouse(bool value);

    /// <summary>
    /// Returns true if the mouse cursor is currently locked to the window.
    /// </summary>
    /// <returns>True if the mouse is locked.</returns>
    bool is_mouse_locked() const;

    /// <summary>
    /// Moves the cursor position to given position using the default window.
    /// </summary>
    /// <param name="pos">The position used to set the cursor.</param>
    /// <param name="relative">By default it will do it relatively to the window.</param>
    void warp_mouse(const glm::vec2& pos, bool relative = true);

    /// <summary>
    /// Adds a custom input event to an action. Creates the action if it doesn't exist.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="event">The input event. Ownership is transferred to the input system.</param>
    void add_action_event(const std::string& name, std::unique_ptr<InputEvent> event);

    /// <summary>
    /// Convenience method to bind a keyboard key to an action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="key">The keyboard key to bind.</param>
    void add_key_to_action(const std::string& name, Key key);

    /// <summary>
    /// Convenience method to bind a mouse button to an action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <param name="button">The mouse button to bind.</param>
    void add_action_mouse(const std::string& name, MouseButton button);

    /// <summary>
    /// Binds mouse motion to an action.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    void add_action_mouse_motion(const std::string& name);

    /// <summary>
    /// Removes an action and all its associated input events.
    /// </summary>
    /// <param name="name">The action identifier to remove.</param>
    void remove_action(const std::string& name);

    /// <summary>
    /// Returns true if any input bound to this action is currently active.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>True if the action is pressed.</returns>
    bool is_action_pressed(const std::string& name) const;

    /// <summary>
    /// Returns true only on the first frame any input bound to this action is pressed.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>True if the action was just pressed this frame.</returns>
    bool is_action_just_pressed(const std::string& name) const;

    /// <summary>
    /// Returns true only on the first frame all inputs bound to this action are released.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>True if the action was just released this frame.</returns>
    bool is_action_just_released(const std::string& name) const;

    /// <summary>
    /// Returns how long the action has been held.
    /// </summary>
    /// <param name="name">The action identifier.</param>
    /// <returns>Duration in seconds, or 0.0 if not pressed.</returns>
    float get_action_duration(const std::string& name) const;

   private:
    bool can_use_input_mouse() const;
    bool can_use_input_keyboard() const;
    bool can_capture_keyboard = false;
    bool can_capture_mouse = false;
    // non-owning pointer from SDL, do not free manually
    const bool* keys_sdl = nullptr;
    uint32_t mouse_buttons = 0u;
    uint32_t prev_mouse_buttons = 0u;
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    float mouse_dx = 0.0f;
    float mouse_dy = 0.0f;
    float scroll_dx = 0.0f;
    float scroll_dy = 0.0f;
    bool mouse_locked = false;
    std::vector<bool> prev_keys {};
    std::unordered_map<int32_t, GamepadState> gamepads {};
};

}  // namespace tmt
