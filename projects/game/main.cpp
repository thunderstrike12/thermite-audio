#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input.hpp"
#include "engine/core/logger.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    tmt::Entity voxel {};
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

    { /* Voxel entity */
        voxel = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(voxel);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel);
        renderer.size = glm::uvec3(20u, 10u, 10u);
        transform.set_world_position(glm::vec3(2.0f, 2.0f, 0.0f));
        transform.set_world_rotation(glm::vec3(glm::radians(45.0f), glm::radians(45.0f), 0.0f));
        transform.set_world_scale(glm::vec3(1.0f, 2.0f, 1.0f));
    }

    constexpr float PRIM_RANGE = 128.0f;
    for (int i = 0; i < 255; ++i) {
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.size = glm::uvec3(10u + (rand() % 90u), 10u + (rand() % 90u), 10u + (rand() % 90u));
        float rx = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
        float ry = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
        float rz = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
        float rrx = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 180.0f;
        float rry = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 180.0f;
        transform.set_world_position(glm::vec3(rx, ry, rz));
        transform.set_world_rotation(glm::vec3(glm::radians(rrx), glm::radians(rry), 0.0f));
        transform.set_world_scale(glm::vec3(1.0f, 1.0f, 1.0f));
    }
}

void Game::on_update(const tmt::FrameData& time) {
    time_passed += time.delta_time;
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
    transform.set_world_position(glm::vec3(1.0f, sinf(time_passed), 1.0f));
    transform.set_world_rotation(glm::vec3(0.0f, cosf(time_passed), 0.0f));
    transform.set_world_scale(glm::vec3(1.0f, 1.5f + sinf(time_passed), 1.0f));

    // Testing code here, please move when scenes can be added elegantly
    if (tmt::engine.input.is_action_just_pressed("confirm")) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Confirm action pressed!");
    }
    if (tmt::engine.input.is_action_just_pressed("cancel")) {
        tmt::Log::info(tmt::Log::Scope::GAME, "Cancel action pressed!");
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
    }
}

void Game::on_end() {}
