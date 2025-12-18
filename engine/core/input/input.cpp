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

    engine.input_map.setup_default_actions();
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
    auto it = engine.input_map.actions.find(std::string {name});
    if (it == engine.input_map.actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_pressed() == true) return true;
    }
    return false;
}

bool Input::is_action_just_pressed(std::string_view name) const {
    auto it = engine.input_map.actions.find(std::string {name});
    if (it == engine.input_map.actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_just_pressed() == true) return true;
    }

    return false;
}

bool Input::is_action_just_released(std::string_view name) const {
    auto it = engine.input_map.actions.find(std::string {name});
    if (it == engine.input_map.actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);

        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_just_released() == true) return true;
    }

    return false;
}
float Input::get_action_strength(std::string_view name) {
    auto it = engine.input_map.actions.find(std::string {name});
    if (it == engine.input_map.actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);

        return 0.0f;
    }

    // TODO inneficient in principle, could cache it once
    float max_strength = 0.0f;
    for (const auto& event : it->second.events) {
        auto strength = event->get_action_strength();
        if (strength < it->second.deadzone) {
            continue;
        }
        max_strength = std::max(strength, max_strength);
    }
    return max_strength;
}
float Input::get_action_raw_strength(std::string_view name) {
    auto it = engine.input_map.actions.find(std::string {name});
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
float Input::get_axis(std::string_view negative_action, std::string_view positive_action) { return get_action_strength(positive_action) - get_action_strength(negative_action); }
// https://github.com/godotengine/godot/blob/79603b2f28fdd8b0dce14064e488a3783d51d1ff/core/input/input.cpp#L548C1-L572C2
glm::vec2 Input::get_vector(std::string_view negative_action_x, std::string_view positive_action_x, std::string_view negative_action_y, std::string_view positive_action_y, float deadzone) {
    glm::vec2 vector {
        get_action_raw_strength(positive_action_x) - get_action_raw_strength(negative_action_x), get_action_raw_strength(positive_action_y) - get_action_raw_strength(negative_action_y)
    };
    auto& input_map = engine.input_map;
    if (deadzone < 0.0f) {
        deadzone = 0.25f * (input_map.get_action_deadzone(negative_action_x) + input_map.get_action_deadzone(negative_action_y) + input_map.get_action_deadzone(positive_action_x) +
                            input_map.get_action_deadzone(positive_action_y));
    }
    float length = glm::length(vector);
    if (length < deadzone) {
        return {};
    }
    if (length > 1.0f) {
        return vector / length;
    }
    const float remapped = (length - deadzone) / (1.0f - deadzone);
    return vector * remapped / length;
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

void Input::add_action_event(const std::string& name, std::unique_ptr<InputEvent> event) { engine.input_map.actions[name].events.push_back(std::move(event)); }

void Input::add_action_mouse(const std::string& name, MouseButton button) { add_action_event(name, std::make_unique<InputEventMouseButton>(button)); }

void Input::remove_action(const std::string& name) { engine.input_map.actions.erase(name); }
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
