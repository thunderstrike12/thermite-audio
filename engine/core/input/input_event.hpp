#pragma once

#include "engine/core/input/keys.hpp"

namespace tmt {

class Input;

class InputEvent {
   public:
    virtual ~InputEvent() = default;
    virtual bool is_pressed() const = 0;
    virtual bool is_just_pressed() const = 0;
    virtual bool is_just_released() const = 0;
    virtual float get_action_strength() const = 0;
};

class InputEventKey : public InputEvent {
   public:
    explicit InputEventKey(Key key) : key(key) {}

    bool is_pressed() const override;
    bool is_just_pressed() const override;
    bool is_just_released() const override;
    float get_action_strength() const override;

    Key get_key() const { return key; }
    void set_key(Key new_key) { key = new_key; }

   private:
    Key key { Key::UNKNOWN };
};

class InputEventMouseMotion : public InputEvent {
   public:
    // These will be overriden by InputEventMouseButton
    bool is_pressed() const { return false; }
    bool is_just_pressed() const { return false; }
    bool is_just_released() const { return false; }
    float get_action_strength() const override;

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
    float position_x = 0.0f;
    float position_y = 0.0f;

    float relative_x = 0.0f;
    float relative_y = 0.0f;
};

class InputEventMouseButton : public InputEventMouseMotion {
   public:
    explicit InputEventMouseButton(MouseButton button) : button(button) {}

    bool is_pressed() const override;
    bool is_just_pressed() const override;
    bool is_just_released() const override;
    float get_action_strength() const override;

    MouseButton get_button() const { return button; }
    void set_button(MouseButton new_button) { button = new_button; }

   private:
    MouseButton button;
};
class InputEventGamepadButton : public InputEvent {
   public:
    explicit InputEventGamepadButton(GamepadButton button, int32_t device_id = -1) : button(button), device_id(device_id) {}

    bool is_pressed() const override;
    bool is_just_pressed() const override;
    bool is_just_released() const override;
    float get_action_strength() const override;

    GamepadButton get_button() const { return button; }
    void set_button(GamepadButton new_button) { button = new_button; }

    int32_t get_device_id() const { return device_id; }
    void set_device_id(int32_t new_device_id) { device_id = new_device_id; }

   private:
    GamepadButton button;
    int32_t device_id = -1;
};

class InputEventGamepadMotion : public InputEvent {
   public:
    explicit InputEventGamepadMotion(GamepadAxis axis, bool negative = false, int32_t device_id = -1) : axis(axis), negative(negative), device_id(device_id) {}

    // TODO add a configurable way to define when an axis is pressed
    bool is_pressed() const override;
    bool is_just_pressed() const override;
    bool is_just_released() const override;
    float get_action_strength() const override;

    float get_axis_value() const;
    GamepadAxis get_axis() const { return axis; }
    void set_axis(GamepadAxis new_axis) { axis = new_axis; }

    int32_t get_device_id() const { return device_id; }
    void set_device_id(int32_t id) { device_id = id; }

   private:
    GamepadAxis axis;
    bool negative = false;
    int32_t device_id = -1;
};

}  // namespace tmt
