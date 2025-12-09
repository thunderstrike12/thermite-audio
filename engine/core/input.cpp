#include "input.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>

#include "engine.hpp"
#include "keys.hpp"
#include "logger.hpp"
#include "window.hpp"

#include "engine/events/sdl.hpp"
#include "tools/profiler.hpp"

using namespace tmt;
void Input::init() {
    int32_t number_keys;
    keys = SDL_GetKeyboardState(&number_keys);
    prev_keys.resize(number_keys, false);
    setup_default_action();
}
void Input::update() {
    TMT_ZONE_SCOPED_N("Input")
    SDL_Event event {};

    std::copy_n(keys, prev_keys.size(), prev_keys.begin());
    prev_mouse_buttons = mouse_buttons;

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
            case SDL_EVENT_MOUSE_MOTION:
                mouse_dx += event.motion.xrel;
                mouse_dy += event.motion.yrel;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                engine.window.width = event.window.data1;
                engine.window.height = event.window.data2;
                engine.window.resized = true;
            case SDL_EVENT_MOUSE_WHEEL:
                scroll_dx = event.wheel.x;
                scroll_dy = event.wheel.y;
                break;
            default:
                break;
        }
    }
}
void Input::add_action(const std::string& name) { actions[name] = InputAction {}; }
bool Input::is_keyboard_button_pressed(Key key) const { return keys[static_cast<SDL_Scancode>(key)]; }
bool Input::is_keyboard_button_just_pressed(Key key) const {
    const auto sdl_scancode = static_cast<SDL_Scancode>(key);
    return keys[sdl_scancode] == true && prev_keys[sdl_scancode] == false;
}
bool Input::is_keyboard_button_released(Key key) const {
    auto sdl_scancode = static_cast<SDL_Scancode>(key);
    return keys[sdl_scancode] == false && prev_keys[sdl_scancode] == true;
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

bool Input::is_action_pressed(const std::string& name) const {
    auto it = actions.find(name);
    if (it == actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_pressed() == true) return true;
    }
    return false;
}

bool Input::is_action_just_pressed(const std::string& name) const {
    auto it = actions.find(name);
    if (it == actions.end()) {
        Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (const auto& event : it->second.events) {
        if (event->is_just_pressed() == true) return true;
    }

    return false;
}

bool Input::is_action_just_released(const std::string& name) const {
    auto it = actions.find(name);
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
void Input::setup_default_action() {
    add_action(action::CONFIRM);
    add_action_keys(action::CONFIRM, Key::SPACE, Key::RETURN);

    add_action(action::CANCEL);
    add_action_keys(action::CANCEL, Key::ESCAPE);

    add_action(action::LEFT_CLICK);
    add_action_mouse(action::LEFT_CLICK, MouseButton::LEFT);

    add_action(action::RIGHT_CLICK);
    add_action_mouse(action::RIGHT_CLICK, MouseButton::RIGHT);

    add_action(action::MIDDLE_CLICK);
    add_action_mouse(action::MIDDLE_CLICK, MouseButton::MIDDLE);

    add_action(action::MOUSE_MOTION);
    add_action_mouse_motion(action::MOUSE_MOTION);
}

void Input::add_key_to_action(const std::string& name, Key key) { add_action_event(name, std::make_unique<InputEventKey>(key)); }

void Input::add_action_event(const std::string& name, std::unique_ptr<InputEvent> event) { actions[name].events.push_back(std::move(event)); }

void Input::add_action_mouse(const std::string& name, MouseButton button) { add_action_event(name, std::make_unique<InputEventMouseButton>(button)); }

void Input::remove_action(const std::string& name) { actions.erase(name); }
void Input::add_action_mouse_motion(const std::string& name) { add_action_event(name, std::make_unique<InputEventMouse>()); }
