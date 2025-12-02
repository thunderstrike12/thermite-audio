#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

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
    tmt::Entity entity = tmt::engine.ecs.create_entity();
    auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
    auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
    transform.set_world_position(glm::vec3(0.0f, 0.0f, -2.0f));
}

void Game::on_update(const tmt::FrameData& time) {}

void Game::on_end() {}
