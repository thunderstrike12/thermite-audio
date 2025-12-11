#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"

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
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
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

    // AI generated for testing inputs
    //  Testing code here, please move when scenes can be added elegantly
    if (tmt::engine.input.is_action_just_pressed("confirm")) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Confirm action pressed!");
        tmt::engine.input.set_mouse_relative_to_window(true);
        tmt::Log::info(tmt::Log::Scope::GAME, "Cursor locked");
    }
    if (tmt::engine.input.is_action_just_pressed("cancel")) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Cancel action pressed!");
        tmt::engine.input.set_mouse_relative_to_window(false);
        tmt::Log::info(tmt::Log::Scope::GAME, "Cursor unlocked");
    }
    if (tmt::engine.input.is_action_pressed("confirm")) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Confirm pressed continuously!");
    }
    if (tmt::engine.input.is_action_just_released("confirm")) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Confirm released!");
    }
    if (tmt::engine.input.is_action_just_pressed("left_click")) {
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Left click!");
    }
    if (tmt::engine.input.is_action_just_released("right_click")) {
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Right click!");
    }
    if (tmt::engine.input.is_action_pressed("middle_click")) {
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Middle click!");
    }
    // Mouse motion test
    if (tmt::engine.input.is_action_pressed("left_click")) {
        tmt::Log::info(
            tmt::Log::Scope::ENGINE, "Mouse moved: pos({}, {}) delta({}, {})", tmt::engine.input.get_mouse_x(), tmt::engine.input.get_mouse_y(), tmt::engine.input.get_mouse_delta_x(),
            tmt::engine.input.get_mouse_delta_y()
        );
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Mouse scroll wheel: wheel({}, {})", tmt::engine.input.get_mouse_wheel_x(), tmt::engine.input.get_mouse_wheel_y());
    }

    // Gamepad tests

    // Gamepad axes (log when stick is moved past deadzone)
    float lx = tmt::engine.input.get_gamepad_axis(tmt::GamepadAxis::LEFT_X);
    float ly = tmt::engine.input.get_gamepad_axis(tmt::GamepadAxis::LEFT_Y);
    float rx = tmt::engine.input.get_gamepad_axis(tmt::GamepadAxis::RIGHT_X);
    float ry = tmt::engine.input.get_gamepad_axis(tmt::GamepadAxis::RIGHT_Y);
    float lt = tmt::engine.input.get_gamepad_axis(tmt::GamepadAxis::LEFT_TRIGGER);
    float rt = tmt::engine.input.get_gamepad_axis(tmt::GamepadAxis::RIGHT_TRIGGER);

    constexpr float DEADZONE = 0.15f;
    if (std::abs(lx) > DEADZONE || std::abs(ly) > DEADZONE) {
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Left stick: ({:.2f}, {:.2f})", lx, ly);
    }
    if (std::abs(rx) > DEADZONE || std::abs(ry) > DEADZONE) {
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Right stick: ({:.2f}, {:.2f})", rx, ry);
    }
    if (lt > DEADZONE) {
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Left trigger: {:.2f}", lt);
    }
    if (rt > DEADZONE) {
        tmt::Log::info(tmt::Log::Scope::ENGINE, "Right trigger: {:.2f}", rt);
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
