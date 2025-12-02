#pragma once
#include "keys.hpp"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_scancode.h"

#include <memory>
#include <string>

namespace tmt {
class InputEvent {
   public:
    virtual ~InputEvent() = default;
    virtual void trigger() = 0;
};

struct InputAction {
    std::vector<Key> keys;
};
class Input {
   public:
    // Rule of 0

    void init();
    void update();

    bool is_keyboard_button_pressed(Key key) const;

    template <typename... Keys>
    void add_action(const std::string& name, Keys... keys) {
        actions[name] = InputAction {{keys...}};
    }
    void add_key_to_action(const std::string& name, Key key);
    void remove_action(const std::string& name);

    bool is_action_pressed(const std::string& name) const;
    bool is_action_just_pressed(const std::string& name) const;
    bool is_action_just_released(const std::string& name) const;

   private:
    void setup_default_action();
    // do not free this manually
    const bool* keys {nullptr};
    std::vector<bool> prev_keys;
    std::unordered_map<std::string, InputAction> actions;
};
}  // namespace tmt
