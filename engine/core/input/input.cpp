#include "input.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_gamepad.h>
#include <magic_enum/magic_enum.hpp>

#include "engine/engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"
#include "engine/tools/profiler.hpp"

#include "engine/events/sdl.hpp"
#include "keys.hpp"

using namespace tmt;

void Input::init() {
    int32_t number_keys;
    keys_sdl = SDL_GetKeyboardState(&number_keys);
    prev_keys.resize(number_keys, false);
    setup_default_action();
}
void Input::setup_default_action() {
    add_action(action::CONFIRM);
    add_action_keys(action::CONFIRM, Key::SPACE, Key::RETURN);
    add_action_gamepad_buttons(action::CONFIRM, GamepadButton::SOUTH);

    add_action(action::CANCEL);
    add_action_keys(action::CANCEL, Key::ESCAPE);
    add_action_gamepad_buttons(action::CANCEL, GamepadButton::EAST);

    add_action(action::LEFT_CLICK);
    add_action_mouse(action::LEFT_CLICK, MouseButton::LEFT);
    add_action_gamepad_buttons(action::LEFT_CLICK, GamepadButton::RIGHT_SHOULDER);

    add_action(action::RIGHT_CLICK);
    add_action_mouse(action::RIGHT_CLICK, MouseButton::RIGHT);
    add_action_gamepad_buttons(action::RIGHT_CLICK, GamepadButton::LEFT_SHOULDER);

    add_action(action::MIDDLE_CLICK);
    add_action_mouse(action::MIDDLE_CLICK, MouseButton::MIDDLE);
    add_action_gamepad_buttons(action::MIDDLE_CLICK, GamepadButton::RIGHT_STICK);

    add_action(action::MOUSE_MOTION);
    add_action_mouse_motion(action::MOUSE_MOTION);

    add_action(action::GAMEPAD_LEFT_STICK);
    add_action_gamepad_axes(action::GAMEPAD_LEFT_STICK, GamepadAxis::LEFT_X, GamepadAxis::LEFT_Y);

    add_action(action::GAMEPAD_RIGHT_STICK);
    add_action_gamepad_axes(action::GAMEPAD_RIGHT_STICK, GamepadAxis::RIGHT_X, GamepadAxis::RIGHT_Y);

    add_action(action::GAMEPAD_TRIGGERS);
    add_action_gamepad_axes(action::GAMEPAD_TRIGGERS, GamepadAxis::LEFT_TRIGGER, GamepadAxis::RIGHT_TRIGGER);
}
void Input::update() {
    TMT_ZONE_SCOPED_N("Input")
    SDL_Event event {};
    std::copy_n(keys_sdl, prev_keys.size(), prev_keys.begin());
    prev_mouse_buttons = mouse_buttons;

    // Update gamepad states
    for (auto& [id, state] : gamepads) {
        state.prev_buttons = state.buttons;

        for (int32_t i = 0; i < static_cast<int32_t>(GamepadButton::COUNT); i++) {
            state.buttons[i] = SDL_GetGamepadButton(state.handle, static_cast<SDL_GamepadButton>(i));
        }

        for (int32_t i = 0; i < static_cast<int32_t>(GamepadAxis::COUNT); i++) {
            constexpr float INT16_MAX_F = 32767.0f;
            auto raw = SDL_GetGamepadAxis(state.handle, static_cast<SDL_GamepadAxis>(i));
            state.axes[i] = static_cast<float>(raw) / INT16_MAX_F;
        }
    }

    mouse_dx = 0;
    mouse_dy = 0;
    scroll_dx = 0;
    scroll_dy = 0;
    // Get current mouse state
    mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    while (SDL_PollEvent(&event)) {
        internal::OnSdlEvent::dispatch(event);
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                engine.set_is_running(false);
                break;
            }
            case SDL_EVENT_GAMEPAD_ADDED: {
                int32_t id = event.gdevice.which;
                SDL_Gamepad* handle = SDL_OpenGamepad(id);
                if (handle) {
                    GamepadState state;
                    state.handle = handle;
                    state.name = SDL_GetGamepadName(handle) ? SDL_GetGamepadName(handle) : "Unknown";
                    gamepads[id] = state;
                    Log::info(Log::Scope::ENGINE, "Gamepad added: {}", state.name);
                }
                break;
            }
            case SDL_EVENT_GAMEPAD_REMOVED: {
                int32_t id = event.gdevice.which;
                auto it = gamepads.find(id);
                if (it != gamepads.end()) {
                    Log::info(Log::Scope::ENGINE, "Gamepad removed: {}", it->second.name);
                    SDL_CloseGamepad(it->second.handle);
                    gamepads.erase(it);
                }
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                mouse_dx += event.motion.xrel;
                mouse_dy += event.motion.yrel;
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED: {
                engine.window.width = event.window.data1;
                engine.window.height = event.window.data2;
                engine.window.resized = true;
                break;
            }
            case SDL_EVENT_WINDOW_RESTORED: {
                engine.window.resized = true;
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                scroll_dx = event.wheel.x;
                scroll_dy = event.wheel.y;
                break;
            }
            default:
                break;
        }
    }
}
void Input::add_action(const std::string& name) { actions[name] = InputAction {}; }
bool Input::is_keyboard_button_pressed(Key key) const { return keys_sdl[static_cast<SDL_Scancode>(key)]; }
bool Input::is_keyboard_button_just_pressed(Key key) const {
    const auto sdl_scancode = static_cast<SDL_Scancode>(key);
    return keys_sdl[sdl_scancode] == true && prev_keys[sdl_scancode] == false;
}
bool Input::is_keyboard_button_released(Key key) const {
    auto sdl_scancode = static_cast<SDL_Scancode>(key);
    return keys_sdl[sdl_scancode] == false && prev_keys[sdl_scancode] == true;
}

bool Input::is_mouse_button_pressed(MouseButton button) const { return mouse_buttons & SDL_BUTTON_MASK(static_cast<int>(button)); }
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

bool Input::is_action_pressed(std::string_view name) const {
    auto it = actions.find(std::string {name});
    if (it == actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_pressed() == true) return true;
    }
    return false;
}

bool Input::is_action_just_pressed(std::string_view name) const {
    auto it = actions.find(std::string {name});
    if (it == actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_just_pressed() == true) return true;
    }

    return false;
}

bool Input::is_action_just_released(std::string_view name) const {
    auto it = actions.find(std::string {name});
    if (it == actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);

        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_just_released() == true) return true;
    }

    return false;
}
void Input::set_mouse_relative_to_window(bool value) { SDL_SetWindowRelativeMouseMode(engine.window.window, value); }
bool Input::get_mouse_relative_to_window() { return SDL_GetWindowRelativeMouseMode(engine.window.window); }

void Input::lock_mouse(bool value) const {
    if (value) {
        const SDL_Rect rect {static_cast<int32_t>(mouse_x), static_cast<int32_t>(mouse_y), 1, 1};
        SDL_SetWindowMouseRect(engine.window.window, &rect);
    } else {
        SDL_SetWindowMouseRect(engine.window.window, nullptr);
    }
}

void Input::add_key_to_action(const std::string& name, Key key) { add_action_event(name, std::make_unique<InputEventKey>(key)); }

void Input::add_action_event(const std::string& name, std::unique_ptr<InputEvent> event) { actions[name].events.push_back(std::move(event)); }

void Input::add_action_mouse(const std::string& name, MouseButton button) { add_action_event(name, std::make_unique<InputEventMouseButton>(button)); }

void Input::remove_action(const std::string& name) { actions.erase(name); }
void Input::add_action_mouse_motion(const std::string& name) { add_action_event(name, std::make_unique<InputEventMouseMotion>()); }

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

const char* Input::get_gamepad_axis_name(GamepadAxis axis) const { return magic_enum::enum_name(axis).data(); }
