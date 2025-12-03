#pragma once

#include "engine/core/keys.hpp"

namespace tmt {
class Input;

class InputEvent {
   public:
    virtual ~InputEvent() = default;
    virtual bool is_pressed() const = 0;
    virtual bool is_just_pressed() const = 0;
    virtual bool is_just_released() const = 0;
};

class InputEventKey : public InputEvent {
   public:
    explicit InputEventKey(Key key) : key(key) {}

    bool is_pressed() const override;
    bool is_just_pressed() const override;
    bool is_just_released() const override;

    Key key {Key::UNKNOWN};
};

enum class MouseButton : int32_t {
    LEFT = 1,
    MIDDLE = 2,
    RIGHT = 3,
    X1 = 4,
    X2 = 5,
};

class InputEventMouse : public InputEvent {
   public:
    // These will be overriden by InputEventMouseButton

    bool is_pressed() const override;
    bool is_just_pressed() const override;
    bool is_just_released() const override;

    float get_position_x() const;
    float get_position_y() const;

    float get_relative_x() const;
    float get_relative_y() const;

    void set_position(float x, float y) {
        position_x = x;
        position_y = y;
    }
    void set_relative(float x, float y) {
        relative_x = x;
        relative_y = y;
    }

   private:
    float position_x {0};
    float position_y {0};

    float relative_x {0};
    float relative_y {0};
};

class InputEventMouseButton : public InputEventMouse {
   public:
    explicit InputEventMouseButton(MouseButton button) : button(button) {}

    bool is_pressed() const override;
    bool is_just_pressed() const override;
    bool is_just_released() const override;

    MouseButton button;
};
}  // namespace tmt
