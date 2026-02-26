#pragma once
#include "engine/core/system.hpp"
#include "engine/events/input.hpp"
#include "engine/core/entity.hpp"
#include "engine/tools/types/direction.hpp"
#include "engine/core/components/button.hpp"

namespace tmt {

enum class MoveState {
    IDLE,
    INITIAL_MOVE,
    REPEAT_MOVE,
};

class ButtonManager : public ISystem, public OnMouseMove, public OnMouseButton, public OnKeyboard, public OnGamepadButton, public OnGamepadAxis {
   public:
    std::string get_name() override { return "ButtonManager"; }
    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void update_states(const tmt::FrameData& time);
    void on_end() override;

    void select_button(const Entity button);

   private:
    void next_button(Direction direction);
    void set_default_button();
    void set_state(ButtonState state);
    void update_move_state(float dt);

    Entity selected_button = entt::null;
    Entity previous_button = entt::null;
    float hold_time = 0.0f;

    MoveState move_state = MoveState::IDLE;
    float hold_move_duration = 0.0f;

    /* Settings */
    static constexpr float INITIAL_DELAY = 0.5f;
    static constexpr float REPEAT_DELAY = 0.1f;
    static constexpr float MIN_AXIS_VALUE = 0.3f;

    // Inherited via OnMouseMove
    void on_mouse_move(MouseMoveEvent& event) override;

    // Inherited via OnMouseButton
    void on_mouse_button(MouseButtonEvent& event) override;

    // Inherited via OnKeyboard
    void on_keyboard(KeyboardEvent& event) override;

    // Inherited via OnGamepadButton
    void on_gamepad_button(GamepadButtonEvent& event) override;

    // Inherited via OnGamepadAxis
    void on_gamepad_axis(GamepadAxisEvent& event) override;
};

}  // namespace tmt