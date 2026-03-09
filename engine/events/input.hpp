#pragma once
#include "event.hpp"
#include "../core/input/keys.hpp"
#include <glm/glm.hpp>

namespace tmt {

// ============================================================================
// Mouse Override Event (existing)
// ============================================================================

struct MouseOverride : public EventBase {
    float x;
    float y;
};

class OnRetrieveMouseState : public EventListenerBase<OnRetrieveMouseState, MouseOverride> {
   public:
    OnRetrieveMouseState() : EventListenerBase() {}
    virtual void on_retrieve_mouse_state(MouseOverride& event) = 0;
    void on_event(MouseOverride& event) final override { on_retrieve_mouse_state(event); }
    void on_event(const MouseOverride& event) final override { (void)event; /* Empty */ }
};

// ============================================================================
// Mouse Move Event
// ============================================================================

struct MouseMoveEvent : public EventBase {
    float x;        // Current X position
    float y;        // Current Y position
    float delta_x;  // Movement delta X since last frame
    float delta_y;  // Movement delta Y since last frame
};

class OnMouseMove : public EventListenerBase<OnMouseMove, MouseMoveEvent> {
   public:
    OnMouseMove() : EventListenerBase() {}
    virtual void on_mouse_move(MouseMoveEvent& event) = 0;
    void on_event(MouseMoveEvent& event) final override { on_mouse_move(event); }
    void on_event(const MouseMoveEvent& event) final override { (void)event; }
};

// ============================================================================
// Mouse Button Events
// ============================================================================

enum class MouseButtonState {
    PRESSED,       // Button is being held down
    JUST_PRESSED,  // Button was just pressed this frame
    JUST_RELEASED  // Button was just released this frame
};

struct MouseButtonEvent : public EventBase {
    MouseButton button;
    MouseButtonState state;
    float x;  // Mouse position when event occurred
    float y;
};

class OnMouseButton : public EventListenerBase<OnMouseButton, MouseButtonEvent> {
   public:
    OnMouseButton() : EventListenerBase() {}
    virtual void on_mouse_button(MouseButtonEvent& event) = 0;
    void on_event(MouseButtonEvent& event) final override { on_mouse_button(event); }
    void on_event(const MouseButtonEvent& event) final override { (void)event; }
};

// ============================================================================
// Mouse Scroll Event
// ============================================================================

struct MouseScrollEvent : public EventBase {
    float delta_x;  // Horizontal scroll
    float delta_y;  // Vertical scroll
    float mouse_x;  // Mouse position when event occurred
    float mouse_y;
};

class OnMouseScroll : public EventListenerBase<OnMouseScroll, MouseScrollEvent> {
   public:
    OnMouseScroll() : EventListenerBase() {}
    virtual void on_mouse_scroll(MouseScrollEvent& event) = 0;
    void on_event(MouseScrollEvent& event) final override { on_mouse_scroll(event); }
    void on_event(const MouseScrollEvent& event) final override { (void)event; }
};

// ============================================================================
// Keyboard Events
// ============================================================================

enum class KeyState {
    PRESSED,       // Key is being held down
    JUST_PRESSED,  // Key was just pressed this frame
    JUST_RELEASED  // Key was just released this frame
};

struct KeyboardEvent : public EventBase {
    Key key;
    KeyState state;
    bool shift;  // Modifier states at time of event
    bool ctrl;
    bool alt;
};

class OnKeyboard : public EventListenerBase<OnKeyboard, KeyboardEvent> {
   public:
    OnKeyboard() : EventListenerBase() {}
    virtual void on_keyboard(KeyboardEvent& event) = 0;
    void on_event(KeyboardEvent& event) final override { on_keyboard(event); }
    void on_event(const KeyboardEvent& event) final override { (void)event; }
};

// ============================================================================
// Gamepad Button Events
// ============================================================================

enum class GamepadButtonState {
    PRESSED,       // Button is being held down
    JUST_PRESSED,  // Button was just pressed this frame
    JUST_RELEASED  // Button was just released this frame
};

struct GamepadButtonEvent : public EventBase {
    GamepadButton button;
    GamepadButtonState state;
    int32_t device_id;
};

class OnGamepadButton : public EventListenerBase<OnGamepadButton, GamepadButtonEvent> {
   public:
    OnGamepadButton() : EventListenerBase() {}
    virtual void on_gamepad_button(GamepadButtonEvent& event) = 0;
    void on_event(GamepadButtonEvent& event) final override { on_gamepad_button(event); }
    void on_event(const GamepadButtonEvent& event) final override { (void)event; }
};

// ============================================================================
// Gamepad Axis Events
// ============================================================================

struct GamepadAxisEvent : public EventBase {
    GamepadAxis axis;
    float value;       // Current value (-1.0 to 1.0 for sticks, 0.0 to 1.0 for triggers)
    float prev_value;  // Previous frame's value
    int32_t device_id;
};

class OnGamepadAxis : public EventListenerBase<OnGamepadAxis, GamepadAxisEvent> {
   public:
    OnGamepadAxis() : EventListenerBase() {}
    virtual void on_gamepad_axis(GamepadAxisEvent& event) = 0;
    void on_event(GamepadAxisEvent& event) final override { on_gamepad_axis(event); }
    void on_event(const GamepadAxisEvent& event) final override { (void)event; }
};

// ============================================================================
// Gamepad Connection Events
// ============================================================================

struct GamepadConnectedEvent : public EventBase {
    int32_t device_id;
    const char* name;
};

class OnGamepadConnected : public EventListenerBase<OnGamepadConnected, GamepadConnectedEvent> {
   public:
    OnGamepadConnected() : EventListenerBase() {}
    virtual void on_gamepad_connected(GamepadConnectedEvent& event) = 0;
    void on_event(GamepadConnectedEvent& event) final override { on_gamepad_connected(event); }
    void on_event(const GamepadConnectedEvent& event) final override { (void)event; }
};

struct GamepadDisconnectedEvent : public EventBase {
    int32_t device_id;
    const char* name;
};

class OnGamepadDisconnected : public EventListenerBase<OnGamepadDisconnected, GamepadDisconnectedEvent> {
   public:
    OnGamepadDisconnected() : EventListenerBase() {}
    virtual void on_gamepad_disconnected(GamepadDisconnectedEvent& event) = 0;
    void on_event(GamepadDisconnectedEvent& event) final override { on_gamepad_disconnected(event); }
    void on_event(const GamepadDisconnectedEvent& event) final override { (void)event; }
};

// ============================================================================
// Any Input Event (catch-all for custom handling)
// ============================================================================

enum class InputType { MOUSE_MOVE, MOUSE_BUTTON, MOUSE_SCROLL, KEYBOARD, GAMEPAD_BUTTON, GAMEPAD_AXIS, GAMEPAD_CONNECTED, GAMEPAD_DISCONNECTED };

struct AnyInputEvent : public EventBase {
    InputType type;
    union {
        struct {
            float x, y, delta_x, delta_y;
        } mouse_move;
        struct {
            MouseButton button;
            MouseButtonState state;
            float x, y;
        } mouse_button;
        struct {
            float delta_x, delta_y, mouse_x, mouse_y;
        } mouse_scroll;
        struct {
            Key key;
            KeyState state;
            bool shift, ctrl, alt;
        } keyboard;
        struct {
            GamepadButton button;
            GamepadButtonState state;
            int32_t device_id;
        } gamepad_button;
        struct {
            GamepadAxis axis;
            float value, prev_value;
            int32_t device_id;
        } gamepad_axis;
        struct {
            int32_t device_id;
            const char* name;
        } gamepad_connection;
    } data;
};

class OnAnyInput : public EventListenerBase<OnAnyInput, AnyInputEvent> {
   public:
    OnAnyInput() : EventListenerBase() {}
    virtual void on_any_input(AnyInputEvent& event) = 0;
    void on_event(AnyInputEvent& event) final override { on_any_input(event); }
    void on_event(const AnyInputEvent& event) final override { (void)event; }
};

struct OnBlockInputEvent : public EventBase {
    bool block = false;
};

class OnBlockInputRequest : public EventListenerBase<OnBlockInputRequest, OnBlockInputEvent> {
   public:
    OnBlockInputRequest() : EventListenerBase() {}
    virtual void on_block_input_request(OnBlockInputEvent& event) = 0;
    void on_event(OnBlockInputEvent& event) final override { on_block_input_request(event); }
    void on_event(const OnBlockInputEvent& event) final override { (void)event; }
};

}  // namespace tmt
