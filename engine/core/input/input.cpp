#include "input.hpp"

#include "input_map.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_gamepad.h>
#include <magic_enum/magic_enum.hpp>
#include <cmath>

#include "engine/engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"
#include "engine/tools/profiler.hpp"

#include "engine/events/sdl.hpp"
#include "engine/events/input.hpp"
#include "keys.hpp"

namespace tmt {

bool Input::can_use_input_mouse() const {
    return can_use_mouse_input;
}
bool Input::can_use_input_keyboard() const {
    return can_use_keyboard_input;
}

void Input::init() {
    int32_t number_keys;
    SDL_GetKeyboardState(&number_keys);
    curr_keys.resize(number_keys, false);
    prev_keys.resize(number_keys, false);

    engine.input_map.setup_default_actions();
}
void Input::update(const FrameData& time) {
    TMT_ZONE_SCOPED_N("Input")

    mouse_dx = 0;
    mouse_dx_engine = 0;
    mouse_dy = 0;
    mouse_dy_engine = 0;
    scroll_dx = 0;
    scroll_dy = 0;

    OnBlockInputEvent block_event {};
    OnBlockInputRequest::dispatch(block_event);
    const bool input_blocked = block_event.block;

    SDL_Event sdl_event {};
    while (SDL_PollEvent(&sdl_event)) {
        internal::SdlEvent sdl_event_wrapper(sdl_event);
        internal::OnSdlEvent::dispatch(sdl_event_wrapper);

        if (input_blocked == false) {
            switch (sdl_event.type) {
                case SDL_EVENT_MOUSE_MOTION: {
                    mouse_dx += sdl_event.motion.xrel;
                    mouse_dy += sdl_event.motion.yrel;
                    break;
                }
                case SDL_EVENT_MOUSE_WHEEL: {
                    scroll_dx = sdl_event.wheel.x;
                    scroll_dy = sdl_event.wheel.y;
                    break;
                }
                default:
                    break;
            }
        }

        /* Handle non-blocked events */
        switch (sdl_event.type) {
            case SDL_EVENT_MOUSE_MOTION: {
                mouse_dx_engine += sdl_event.motion.xrel;
                mouse_dy_engine += sdl_event.motion.yrel;
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED: {
                engine.window.width = sdl_event.window.data1;
                engine.window.height = sdl_event.window.data2;
                engine.window.resized = true;
                break;
            }
            case SDL_EVENT_WINDOW_RESTORED: {
                engine.window.resized = true;
                break;
            }
            case SDL_EVENT_QUIT: {
                engine.set_is_running(false);
                break;
            }
            case SDL_EVENT_GAMEPAD_ADDED: {
                int32_t id = sdl_event.gdevice.which;
                SDL_Gamepad* handle = SDL_OpenGamepad(id);
                if (handle) {
                    GamepadState state;
                    state.handle = handle;
                    state.name = SDL_GetGamepadName(handle) ? SDL_GetGamepadName(handle) : "Unknown";
                    gamepads[id] = state;
                    Log::info(Log::Scope::ENGINE, "Gamepad added: {}", state.name);

                    // Dispatch gamepad connected event
                    GamepadConnectedEvent conn_event;
                    conn_event.device_id = id;
                    conn_event.name = state.name.c_str();
                    OnGamepadConnected::dispatch(conn_event);

                    // Also dispatch AnyInputEvent
                    AnyInputEvent any_event;
                    any_event.type = InputType::GAMEPAD_CONNECTED;
                    any_event.data.gamepad_connection.device_id = id;
                    any_event.data.gamepad_connection.name = state.name.c_str();
                    OnAnyInput::dispatch(any_event);
                }
                break;
            }
            case SDL_EVENT_GAMEPAD_REMOVED: {
                int32_t id = sdl_event.gdevice.which;
                auto it = gamepads.find(id);
                if (it != gamepads.end()) {
                    Log::info(Log::Scope::ENGINE, "Gamepad removed: {}", it->second.name);

                    // Dispatch gamepad disconnected event before erasing
                    GamepadDisconnectedEvent disc_event;
                    disc_event.device_id = id;
                    disc_event.name = it->second.name.c_str();
                    OnGamepadDisconnected::dispatch(disc_event);

                    // Also dispatch AnyInputEvent
                    AnyInputEvent any_event;
                    any_event.type = InputType::GAMEPAD_DISCONNECTED;
                    any_event.data.gamepad_connection.device_id = id;
                    any_event.data.gamepad_connection.name = it->second.name.c_str();
                    OnAnyInput::dispatch(any_event);

                    SDL_CloseGamepad(it->second.handle);
                    gamepads.erase(it);
                }
                break;
            }
        }
    }

    if (input_blocked) {
        // If input was blocked by an event, we skip processing the rest of the input for this frame
        can_use_mouse_input = false;
        can_use_keyboard_input = false;
        return;
    } else {
        can_use_mouse_input = true;
        can_use_keyboard_input = true;
    }

    // update keys
    int32_t number_keys;
    const bool* keys_sdl = SDL_GetKeyboardState(&number_keys);
    prev_keys = curr_keys;
    std::copy(keys_sdl, keys_sdl + number_keys, curr_keys.begin());

    // reset hold timers
    for (auto& [action_name, input_action] : engine.input_map.actions) {
        for (auto& event : input_action.events) {
            if (event->is_just_released() == true) {
                input_action.time_since_being_pressed = 0.0f;
            }
        }
    }
    prev_mouse_buttons = mouse_buttons;

    {
        // Still get mouse position, but allow override via event
        mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);

        MouseOverride event;
        OnRetrieveMouseState::dispatch(event);
        if (event.handled) {
            mouse_x = event.x;
            mouse_y = event.y;
        }
    }

    // Update gamepad states
    for (auto& [id, state] : gamepads) {
        state.prev_buttons = state.buttons;
        state.prev_axes = state.axes;

        for (int32_t i = 0; i < static_cast<int32_t>(GamepadButton::COUNT); i++) {
            state.buttons[i] = SDL_GetGamepadButton(state.handle, static_cast<SDL_GamepadButton>(i));
        }

        for (int32_t i = 0; i < static_cast<int32_t>(GamepadAxis::COUNT); i++) {
            constexpr float INT16_MAX_F = 32767.0f;
            auto raw = SDL_GetGamepadAxis(state.handle, static_cast<SDL_GamepadAxis>(i));
            state.axes[i] = static_cast<float>(raw) / INT16_MAX_F;
        }
    }

    // ========================================================================
    // Dispatch Input Events
    // ========================================================================

    // Mouse move event (if there was any movement)
    if (mouse_dx != 0.0f || mouse_dy != 0.0f) {
        MouseMoveEvent move_event;
        move_event.x = mouse_x;
        move_event.y = mouse_y;
        move_event.delta_x = mouse_dx;
        move_event.delta_y = mouse_dy;
        OnMouseMove::dispatch(move_event);

        AnyInputEvent any_event;
        any_event.type = InputType::MOUSE_MOVE;
        any_event.data.mouse_move.x = mouse_x;
        any_event.data.mouse_move.y = mouse_y;
        any_event.data.mouse_move.delta_x = mouse_dx;
        any_event.data.mouse_move.delta_y = mouse_dy;
        OnAnyInput::dispatch(any_event);
    }

    // Mouse scroll event (if there was any scroll)
    if (scroll_dx != 0.0f || scroll_dy != 0.0f) {
        MouseScrollEvent scroll_event;
        scroll_event.delta_x = scroll_dx;
        scroll_event.delta_y = scroll_dy;
        scroll_event.mouse_x = mouse_x;
        scroll_event.mouse_y = mouse_y;
        OnMouseScroll::dispatch(scroll_event);

        AnyInputEvent any_event;
        any_event.type = InputType::MOUSE_SCROLL;
        any_event.data.mouse_scroll.delta_x = scroll_dx;
        any_event.data.mouse_scroll.delta_y = scroll_dy;
        any_event.data.mouse_scroll.mouse_x = mouse_x;
        any_event.data.mouse_scroll.mouse_y = mouse_y;
        OnAnyInput::dispatch(any_event);
    }

    // Mouse button events
    constexpr MouseButton mouse_button_list[] = { MouseButton::LEFT, MouseButton::MIDDLE, MouseButton::RIGHT, MouseButton::X1, MouseButton::X2 };

    for (auto button : mouse_button_list) {
        auto mask = SDL_BUTTON_MASK(static_cast<int32_t>(button));
        bool is_pressed = (mouse_buttons & mask) != 0;
        bool was_pressed = (prev_mouse_buttons & mask) != 0;

        if (is_pressed != was_pressed || is_pressed) {
            MouseButtonEvent btn_event;
            btn_event.button = button;
            btn_event.x = mouse_x;
            btn_event.y = mouse_y;

            if (is_pressed && !was_pressed) {
                btn_event.state = MouseButtonState::JUST_PRESSED;
            } else if (!is_pressed && was_pressed) {
                btn_event.state = MouseButtonState::JUST_RELEASED;
            } else if (is_pressed) {
                btn_event.state = MouseButtonState::PRESSED;
            } else {
                continue;  // No event needed
            }

            OnMouseButton::dispatch(btn_event);

            AnyInputEvent any_event;
            any_event.type = InputType::MOUSE_BUTTON;
            any_event.data.mouse_button.button = button;
            any_event.data.mouse_button.state = btn_event.state;
            any_event.data.mouse_button.x = mouse_x;
            any_event.data.mouse_button.y = mouse_y;
            OnAnyInput::dispatch(any_event);
        }
    }

    // Keyboard events - check modifier states
    bool shift_pressed = keys_sdl[static_cast<int32_t>(Key::LEFT_SHIFT)] || keys_sdl[static_cast<int32_t>(Key::RIGHT_SHIFT)];
    bool ctrl_pressed = keys_sdl[static_cast<int32_t>(Key::LEFT_CTRL)] || keys_sdl[static_cast<int32_t>(Key::RIGHT_CTRL)];
    bool alt_pressed = keys_sdl[static_cast<int32_t>(Key::LEFT_ALT)] || keys_sdl[static_cast<int32_t>(Key::RIGHT_ALT)];

    // Check all keys for state changes
    for (size_t i = 0; i < prev_keys.size(); i++) {
        bool is_pressed = keys_sdl[i];
        bool was_pressed = prev_keys[i];

        if (is_pressed != was_pressed || is_pressed) {
            KeyboardEvent key_event;
            key_event.key = static_cast<Key>(i);
            key_event.shift = shift_pressed;
            key_event.ctrl = ctrl_pressed;
            key_event.alt = alt_pressed;

            if (is_pressed && !was_pressed) {
                key_event.state = KeyState::JUST_PRESSED;
            } else if (!is_pressed && was_pressed) {
                key_event.state = KeyState::JUST_RELEASED;
            } else if (is_pressed) {
                key_event.state = KeyState::PRESSED;
            } else {
                continue;  // No event needed
            }

            OnKeyboard::dispatch(key_event);

            AnyInputEvent any_event;
            any_event.type = InputType::KEYBOARD;
            any_event.data.keyboard.key = key_event.key;
            any_event.data.keyboard.state = key_event.state;
            any_event.data.keyboard.shift = shift_pressed;
            any_event.data.keyboard.ctrl = ctrl_pressed;
            any_event.data.keyboard.alt = alt_pressed;
            OnAnyInput::dispatch(any_event);
        }
    }

    // Gamepad button and axis events
    for (auto& [id, state] : gamepads) {
        // Button events
        for (int32_t i = 0; i < static_cast<int32_t>(GamepadButton::COUNT); i++) {
            bool is_pressed = state.buttons[i];
            bool was_pressed = state.prev_buttons[i];

            if (is_pressed != was_pressed || is_pressed) {
                GamepadButtonEvent btn_event;
                btn_event.button = static_cast<GamepadButton>(i);
                btn_event.device_id = id;

                if (is_pressed && !was_pressed) {
                    btn_event.state = GamepadButtonState::JUST_PRESSED;
                } else if (!is_pressed && was_pressed) {
                    btn_event.state = GamepadButtonState::JUST_RELEASED;
                } else if (is_pressed) {
                    btn_event.state = GamepadButtonState::PRESSED;
                } else {
                    continue;
                }

                OnGamepadButton::dispatch(btn_event);

                AnyInputEvent any_event;
                any_event.type = InputType::GAMEPAD_BUTTON;
                any_event.data.gamepad_button.button = btn_event.button;
                any_event.data.gamepad_button.state = btn_event.state;
                any_event.data.gamepad_button.device_id = id;
                OnAnyInput::dispatch(any_event);
            }
        }

        // Axis events (only when value changes significantly)
        constexpr float AXIS_CHANGE_THRESHOLD = 0.001f;
        for (int32_t i = 0; i < static_cast<int32_t>(GamepadAxis::COUNT); i++) {
            float current = state.axes[i];
            float prev = state.prev_axes[i];

            if (std::abs(current - prev) > AXIS_CHANGE_THRESHOLD) {
                GamepadAxisEvent axis_event;
                axis_event.axis = static_cast<GamepadAxis>(i);
                axis_event.value = current;
                axis_event.prev_value = prev;
                axis_event.device_id = id;
                OnGamepadAxis::dispatch(axis_event);

                AnyInputEvent any_event;
                any_event.type = InputType::GAMEPAD_AXIS;
                any_event.data.gamepad_axis.axis = axis_event.axis;
                any_event.data.gamepad_axis.value = current;
                any_event.data.gamepad_axis.prev_value = prev;
                any_event.data.gamepad_axis.device_id = id;
                OnAnyInput::dispatch(any_event);
            }
        }
    }

    // update hold timers

    for (auto& [action_name, input_action] : engine.input_map.actions) {
        for (auto& event : input_action.events) {
            if (event->is_pressed() == true) {
                input_action.time_since_being_pressed += time.delta_time;
            }
        }
    }
}
bool Input::is_keyboard_button_pressed(Key key) const {
    return curr_keys[static_cast<SDL_Scancode>(key)];
}

bool Input::is_keyboard_button_just_pressed(Key key) const {
    const auto sdl_scancode = static_cast<SDL_Scancode>(key);
    return curr_keys[sdl_scancode] == true && prev_keys[sdl_scancode] == false;
}
bool Input::is_keyboard_button_released(Key key) const {
    auto sdl_scancode = static_cast<SDL_Scancode>(key);
    return curr_keys[sdl_scancode] == false && prev_keys[sdl_scancode] == true;
}

bool Input::is_mouse_button_pressed(MouseButton button) const {
    return mouse_buttons & SDL_BUTTON_MASK(static_cast<int>(button));
}
bool Input::is_mouse_button_just_pressed(MouseButton button) const {
    auto mask = SDL_BUTTON_MASK(static_cast<int32_t>(button));
    bool is_pressed = (mouse_buttons & mask) != 0;
    bool was_pressed = (prev_mouse_buttons & mask) != 0;
    return is_pressed == true && was_pressed == false;
}
bool Input::is_mouse_button_just_released(MouseButton button) const {
    auto mask = SDL_BUTTON_MASK(static_cast<int32_t>(button));
    bool is_pressed = (mouse_buttons & mask) != 0;
    bool was_pressed = (prev_mouse_buttons & mask) != 0;
    return is_pressed == false && was_pressed == true;
}

bool Input::is_action_pressed(const std::string& name) const {
    auto it = engine.input_map.actions.find(name);
    if (it == engine.input_map.actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_pressed() == true) return true;
    }
    return false;
}

bool Input::is_action_just_pressed(const std::string& name) const {
    auto it = engine.input_map.actions.find(name);
    if (it == engine.input_map.actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_just_pressed() == true) return true;
    }

    return false;
}

bool Input::is_action_just_released(const std::string& name) const {
    auto it = engine.input_map.actions.find(name);
    if (it == engine.input_map.actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);

        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_just_released() == true) return true;
    }

    return false;
}
float Input::get_action_duration(const std::string& name) const {
    auto it = engine.input_map.actions.find(name);
    if (it == engine.input_map.actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);

        return 0.0f;
    }
    return it->second.time_since_being_pressed;
}
float Input::get_action_strength(const std::string& name) {
    auto it = engine.input_map.actions.find(name);
    if (it == engine.input_map.actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);

        return 0.0f;
    }

    // TODO inneficient in principle, could cache it once, but first it should be a bottleneck through profiling
    float max_strength = 0.0f;
    for (const auto& event : it->second.events) {
        auto strength = event->get_action_strength();
        if (strength < it->second.deadzone) {
            continue;
        }

        max_strength = std::max(strength, max_strength);
    }

    return max_strength * it->second.sensitivity;
}
float Input::get_action_raw_strength(const std::string& name) {
    auto it = engine.input_map.actions.find(name);
    if (it == engine.input_map.actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);

        return 0.0f;
    }

    // TODO inneficient in principle, could cache it once
    float max_strength = 0.0f;
    for (const auto& event : it->second.events) {
        max_strength = std::max(event->get_action_strength(), max_strength);
    }
    return max_strength;
}
void Input::clear_input_state() {
    int32_t number_keys;
    SDL_GetKeyboardState(&number_keys);
    curr_keys.resize(number_keys, false);
    prev_keys.resize(number_keys, false);
    prev_mouse_buttons = 0;
    mouse_buttons = 0;
    mouse_dx = 0.0f;
    mouse_dy = 0.0f;
}
float Input::get_axis(const std::string& negative_action, const std::string& positive_action) {
    return get_action_strength(positive_action) - get_action_strength(negative_action);
}
// https://github.com/godotengine/godot/blob/79603b2f28fdd8b0dce14064e488a3783d51d1ff/core/input/input.cpp#L548C1-L572C2
glm::vec2 Input::get_vector(
    const std::string& negative_action_x, const std::string& positive_action_x, const std::string& negative_action_y, const std::string& positive_action_y, float deadzone
) {
    auto& input_map = engine.input_map;
    if (deadzone < 0.0f) {
        deadzone = 0.25f * (input_map.get_action_deadzone(negative_action_x) + input_map.get_action_deadzone(negative_action_y) + input_map.get_action_deadzone(positive_action_x) +
                            input_map.get_action_deadzone(positive_action_y));
    }
    auto filter = [deadzone](float value) { return value < deadzone ? 0.0f : value; };
    float pos_x = filter(get_action_raw_strength(positive_action_x));
    float neg_x = filter(get_action_raw_strength(negative_action_x));
    float pos_y = filter(get_action_raw_strength(positive_action_y));
    float neg_y = filter(get_action_raw_strength(negative_action_y));

    glm::vec2 vector { pos_x - neg_x, pos_y - neg_y };

    float length = glm::length(vector);
    if (length < deadzone) {
        return {};
    }
    if (length >= 1.0f) {
        return vector / length;
    }
    const float remapped = (length - deadzone) / (1.0f - deadzone);
    return vector * remapped / length;
}
float Input::get_mouse_x() const {
    if (!can_use_input_mouse()) return false;
    return mouse_x;
}

float Input::get_mouse_y() const {
    if (!can_use_input_mouse()) return 0.0f;
    return mouse_y;
}
float Input::get_mouse_wheel_x() const {
    if (!can_use_input_mouse()) return 0.0f;
    return scroll_dx;
}
float Input::get_mouse_wheel_y() const {
    if (!can_use_input_mouse()) return 0.0f;
    return scroll_dy;
}
float Input::get_mouse_delta_x() const {
    if (!can_use_input_mouse()) return 0.0f;
    return mouse_dx;
}
float Input::get_mouse_delta_y() const {
    if (!can_use_input_mouse()) return 0.0f;
    return mouse_dy;
}
glm::vec2 Input::get_mouse_delta() const {
    if (!can_use_input_mouse()) return {};
    return { mouse_dx, mouse_dy };
}
float Input::get_mouse_delta_engine_x() const {
    return mouse_dx_engine;
}
float Input::get_mouse_delta_engine_y() const {
    return mouse_dy_engine;
}
glm::vec2 Input::get_mouse_delta_engine() const {
    return { mouse_dx_engine, mouse_dy_engine };
}
void Input::set_mouse_relative_to_window(bool value) {
    SDL_SetWindowRelativeMouseMode(engine.window.window, value);
}
bool Input::get_mouse_relative_to_window() {
    return SDL_GetWindowRelativeMouseMode(engine.window.window);
}

void Input::lock_mouse(bool value) {
    if (value) {
        float temp_mouse_x = 0.0f;
        float temp_mouse_y = 0.0f;
        SDL_GetMouseState(&temp_mouse_x, &temp_mouse_y);

        const SDL_Rect rect { static_cast<int32_t>(temp_mouse_x), static_cast<int32_t>(temp_mouse_y), 1, 1 };
        SDL_SetWindowMouseRect(engine.window.window, &rect);
    } else {
        SDL_SetWindowMouseRect(engine.window.window, nullptr);
    }
    mouse_locked = value;
}

bool Input::is_mouse_locked() const {
    return mouse_locked;
}

void Input::set_game_preferred_mouse_lock(bool value) {
    game_mouse_lockstate = value;
}
bool Input::get_game_preferred_mouse_lock() const {
    return game_mouse_lockstate;
}

void Input::warp_mouse(const glm::vec2& pos, bool relative) {
    if (relative) {
        SDL_WarpMouseInWindow(engine.window.window, pos.x, pos.y);
    } else {
        SDL_WarpMouseGlobal(pos.x, pos.y);
    }
}

void Input::add_key_to_action(const std::string& name, Key key) {
    add_action_event(name, std::make_unique<InputEventKey>(key));
}

void Input::add_action_event(const std::string& name, std::unique_ptr<InputEvent> event) {
    engine.input_map.actions[name].events.push_back(std::move(event));
}

void Input::add_action_mouse(const std::string& name, MouseButton button) {
    add_action_event(name, std::make_unique<InputEventMouseButton>(button));
}

void Input::remove_action(const std::string& name) {
    engine.input_map.actions.erase(name);
}
void Input::add_action_mouse_motion(const std::string& name) {
    add_action_event(name, std::make_unique<InputEventMouseMotion>());
}

int32_t Input::get_default_gamepad_id() const {
    if (gamepads.empty()) {
        return -1;
    }
    return gamepads.begin()->first;
}

bool Input::is_gamepad_button_pressed(GamepadButton button, int32_t device_id) const {
    if (device_id == -1) device_id = get_default_gamepad_id();
    if (device_id == -1) return false;

    auto it = gamepads.find(device_id);
    if (it == gamepads.end()) {
        Log::warn(Log::Scope::ENGINE, "Gamepad with id {} not found", device_id);
        return false;
    }
    return it->second.buttons[static_cast<int32_t>(button)];
}

bool Input::is_gamepad_button_just_pressed(GamepadButton button, int32_t device_id) const {
    if (device_id == -1) device_id = get_default_gamepad_id();
    if (device_id == -1) return false;

    auto it = gamepads.find(device_id);
    if (it == gamepads.end()) {
        Log::warn(Log::Scope::ENGINE, "Gamepad with id {} not found", device_id);
        return false;
    }
    int32_t idx = static_cast<int32_t>(button);
    return it->second.buttons[idx] && !it->second.prev_buttons[idx];
}

bool Input::is_gamepad_button_just_released(GamepadButton button, int32_t device_id) const {
    if (device_id == -1) device_id = get_default_gamepad_id();
    if (device_id == -1) return false;

    auto it = gamepads.find(device_id);
    if (it == gamepads.end()) {
        Log::warn(Log::Scope::ENGINE, "Gamepad with id {} not found", device_id);
        return false;
    }
    int32_t idx = static_cast<int32_t>(button);
    return !it->second.buttons[idx] && it->second.prev_buttons[idx];
}

float Input::get_gamepad_axis(GamepadAxis axis, int32_t device_id) const {
    if (device_id == -1) device_id = get_default_gamepad_id();
    if (device_id == -1) return 0.0f;

    auto it = gamepads.find(device_id);
    if (it == gamepads.end()) {
        Log::warn(Log::Scope::ENGINE, "Gamepad with id {} not found", device_id);
        return 0.0f;
    }
    return it->second.axes[static_cast<int32_t>(axis)];
}

const char* Input::get_gamepad_button_name(GamepadButton button, int32_t device_id) const {
    if (device_id == -1) device_id = get_default_gamepad_id();
    if (device_id == -1) return magic_enum::enum_name(button).data();

    auto it = gamepads.find(device_id);
    if (it == gamepads.end()) return magic_enum::enum_name(button).data();

    return magic_enum::enum_name(button).data();
}

const char* Input::get_gamepad_axis_name(GamepadAxis axis) const {
    return magic_enum::enum_name(axis).data();
}

}  // namespace tmt
