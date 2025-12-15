#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
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
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -16.0f));
    }

    auto voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "dragon128.vengi"});
    auto voxel_volume = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);

    { /* Voxel entity */
        voxel = tmt::engine.ecs.create_entity("Moving Voxel");
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(voxel);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel);
        renderer.resource = voxel_volume;
        transform.set_world_position(glm::vec3(0.0f, 0.0f, 0.0f));
        transform.set_world_scale(glm::vec3(1.0f, 1.0f, 1.0f));
    }

    constexpr float PRIM_RANGE = 256.0f;
    for (int i = 0; i < 255; ++i) {
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume;
        float s = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 3.0f;
        float rx = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
        float ry = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
        float rz = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
        float rrx = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 180.0f;
        float rry = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 180.0f;
        transform.set_world_position(glm::vec3(rx, ry, rz));
        transform.set_world_rotation(glm::vec3(glm::radians(rrx), glm::radians(rry), 0.0f));
        transform.set_world_scale(1.0f + glm::vec3(s, s, s));
    }
}

void Game::on_update(const tmt::FrameData& time) {
    time_passed += time.delta_time;
    // auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
    // transform.set_world_position(glm::vec3(1.0f, sinf(time_passed), 1.0f));
    // transform.set_world_rotation(glm::vec3(sinf(time_passed), cosf(time_passed), 0.0f));
    // transform.set_world_scale(glm::vec3(1.0f, 1.5f + sinf(time_passed), 1.0f));
}

void Game::on_end() {}
