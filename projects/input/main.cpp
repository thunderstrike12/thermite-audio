#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input_map.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    float time_passed = 0.0f;

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Example Game",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    return std::make_unique<Game>(specs);
}

void Game::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -2.0f));
    }

    // { /* Voxel entity */
    //     auto voxel = tmt::engine.ecs.create_entity();
    //     auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(voxel);
    //     auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel);
    //     renderer.size = glm::uvec3(20u, 10u, 10u);
    //     transform.set_world_position(glm::vec3(2.0f, 2.0f, 0.0f));
    //     transform.set_world_rotation(glm::vec3(glm::radians(45.0f), glm::radians(45.0f), 0.0f));
    //     transform.set_world_scale(glm::vec3(1.0f, 2.0f, 1.0f));
    // }
}

void Game::on_update(const tmt::FrameData& time) {
    time_passed += time.delta_time;

    // Button action tests using constexpr names
    if (tmt::engine.input.is_action_just_pressed(tmt::action::CONFIRM)) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Confirm action pressed!");
        tmt::engine.input.set_mouse_relative_to_window(true);
        tmt::Log::info(tmt::Log::Scope::GAME, "Cursor locked");
    }
    if (tmt::engine.input.is_action_just_pressed(tmt::action::CANCEL)) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Cancel action pressed!");
        tmt::engine.input.set_mouse_relative_to_window(false);
        tmt::Log::info(tmt::Log::Scope::GAME, "Cursor unlocked");
    }
    if (tmt::engine.input.is_action_just_released(tmt::action::CANCEL)) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Cancel was held for {:.2f} seconds", tmt::engine.input.get_action_duration(tmt::action::CANCEL));
    }
    if (tmt::engine.input.is_action_pressed(tmt::action::CONFIRM)) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Confirm pressed continuously!");
    }
    if (tmt::engine.input.is_action_just_released(tmt::action::CONFIRM)) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Confirm released!");
    }

    // Mouse button tests
    if (tmt::engine.input.is_action_just_pressed(tmt::action::LEFT_CLICK)) {
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Left click!");
    }
    if (tmt::engine.input.is_action_just_released(tmt::action::RIGHT_CLICK)) {
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Right click!");
        float action_duration = tmt::engine.input.get_action_duration(tmt::action::RIGHT_CLICK);
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Right click was held for {:.2f} seconds", action_duration);
    }

    if (tmt::engine.input.is_action_pressed(tmt::action::MIDDLE_CLICK)) {
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Middle click!");
    }

    // Mouse motion test
    if (tmt::engine.input.is_action_pressed(tmt::action::LEFT_CLICK)) {
        tmt::Log::info(
            tmt::Log::Scope::ENGINE, "Mouse moved: pos({}, {}) delta({}, {})", tmt::engine.input.get_mouse_x(), tmt::engine.input.get_mouse_y(), tmt::engine.input.get_mouse_delta_x(),
            tmt::engine.input.get_mouse_delta_y()
        );
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Mouse scroll wheel: wheel({}, {})", tmt::engine.input.get_mouse_wheel_x(), tmt::engine.input.get_mouse_wheel_y());
    }
    float throttle = tmt::engine.input.get_axis(tmt::action::LEFT_TRIGGER, tmt::action::RIGHT_TRIGGER);
    bool display_triggers = true;
    if (std::abs(throttle) > 0.0f) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Throttle: {:.2f}", throttle);
        display_triggers = false;
    }

    glm::vec2 move_dir = tmt::engine.input.get_vector(tmt::action::MOVE_LEFT, tmt::action::MOVE_RIGHT, tmt::action::MOVE_DOWN, tmt::action::MOVE_UP, 0.2f);
    if (glm::length(move_dir) > 0.0f) {
        float speed = 1.0f + throttle;  // 0 to 2 range (brake to sprint)
        tmt::Log::info(tmt::Log::Scope::GAME, "Move: ({:.2f}, {:.2f}) speed: {:.2f}", move_dir.x, move_dir.y, speed);
    }
    glm::vec2 look_dir = tmt::engine.input.get_vector(tmt::action::LOOK_LEFT, tmt::action::LOOK_RIGHT, tmt::action::LOOK_DOWN, tmt::action::LOOK_UP, 0.2f);
    if (glm::length(look_dir) > 0.0f) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Look: ({:.2f}, {:.2f})", look_dir.x, look_dir.y);
    }
    float left_trigger = tmt::engine.input.get_action_strength(tmt::action::LEFT_TRIGGER);
    float right_trigger = tmt::engine.input.get_action_strength(tmt::action::RIGHT_TRIGGER);
    if (display_triggers && (left_trigger > 0.0f || right_trigger > 0.0f)) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Triggers: L={:.2f} R={:.2f}", left_trigger, right_trigger);
    }

    // Gamepad buttons
    auto log_button = [](tmt::GamepadButton btn) {
        if (tmt::engine.input.is_gamepad_button_just_pressed(btn)) {
            tmt::Log::info(tmt::Log::Scope::ENGINE, "Gamepad: {} pressed", tmt::engine.input.get_gamepad_button_name(btn));
        }
    };

    log_button(tmt::GamepadButton::SOUTH);
    log_button(tmt::GamepadButton::EAST);
    log_button(tmt::GamepadButton::WEST);
    log_button(tmt::GamepadButton::NORTH);
    log_button(tmt::GamepadButton::LEFT_SHOULDER);
    log_button(tmt::GamepadButton::RIGHT_SHOULDER);
    log_button(tmt::GamepadButton::BACK);
    log_button(tmt::GamepadButton::START);
    log_button(tmt::GamepadButton::LEFT_STICK);
    log_button(tmt::GamepadButton::RIGHT_STICK);
    log_button(tmt::GamepadButton::DPAD_UP);
    log_button(tmt::GamepadButton::DPAD_DOWN);
    log_button(tmt::GamepadButton::DPAD_LEFT);
    log_button(tmt::GamepadButton::DPAD_RIGHT);
}
void Game::on_end() {}
