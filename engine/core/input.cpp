#include "input.hpp"

#include "engine.hpp"
#include "keys.hpp"
#include "logger.hpp"
#include "extern/SDL3/include/SDL3/SDL_events.h"
#include "extern/SDL3/include/SDL3/SDL_keyboard.h"
#include "extern/imgui/backends/imgui_impl_sdl3.h"
void tmt::Input::init() {
    int32_t number_keys;
    keys = SDL_GetKeyboardState(&number_keys);
    prev_keys.resize(number_keys, false);
    setup_default_action();
}
void tmt::Input::update() {
    SDL_Event event {};

    std::copy_n(keys, prev_keys.size(), prev_keys.begin());

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type) {
            case SDL_EVENT_QUIT: {
                engine.set_is_running(false);
                break;
            }
        }
    }
}
bool tmt::Input::is_keyboard_button_pressed(const Key key) const { return keys[to_sdl(key)]; }
void tmt::Input::add_key_to_action(const std::string& name, Key key) {
    if (auto it = actions.find(name); it != actions.end()) {
        it->second.keys.push_back(key);
    }
}

void tmt::Input::remove_action(const std::string& name) { actions.erase(name); }

bool tmt::Input::is_action_pressed(const std::string& name) const {
    auto it = actions.find(name);
    if (it == actions.end()) return false;

    for (Key key : it->second.keys) {
        if (keys[to_sdl(key)]) return true;
    }
    return false;
}

bool tmt::Input::is_action_just_pressed(const std::string& name) const {
    auto it = actions.find(name);
    if (it == actions.end()) {
        tmt::Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);
        return false;
    }

    for (Key key : it->second.keys) {
        auto scancode = to_sdl(key);
        if (keys[scancode] && !prev_keys[scancode]) {
            return true;
        }
    }

    return false;
}

bool tmt::Input::is_action_just_released(const std::string& name) const {
    auto it = actions.find(name);
    if (it == actions.end()) {
        tmt::Log::warn(Log::Scope::ENGINE, "No input action found with this name {}", name);

        return false;
    }

    for (Key key : it->second.keys) {
        int scancode = to_sdl(key);
        if (!keys[scancode] && prev_keys[scancode]) {
            return true;
        }
    }

    return false;
}
void tmt::Input::setup_default_action() {
    add_action("confirm", Key::RETURN, Key::SPACE);
    add_action("cancel", Key::ESCAPE);
}
