#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/renderer/renderer.hpp"

class DebugDrawing : public tmt::Application {
   public:
    DebugDrawing(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    tmt::Entity voxel {};
    float time_passed = 0.0f;

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Debug Drawing",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    return std::make_unique<DebugDrawing>(specs);
}

void DebugDrawing::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -16.0f));
    }
}

void DebugDrawing::on_update(const tmt::FrameData& time) {
    time_passed += time.delta_time;
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);

    tmt::engine.renderer.draw_line({-0.5f, 0.5f, 0.0f}, {0.0f, -0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, 0.0f);
    tmt::engine.renderer.draw_line({0.0f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, 0.0f);
    tmt::engine.renderer.draw_line({0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, 0.0f);

    tmt::engine.renderer.draw_circle({2.0f, 2.0f, 2.0f}, 4.0f, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, 32);

    tmt::engine.renderer.draw_sphere({5.0f, 0.0f, 0.0f}, 2.0f, {1.0f, 0.0f, 0.0f}, 8);

    tmt::engine.renderer.draw_arrow({1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f});

    tmt::engine.renderer.draw_cross({2.0f, 0.0f, 0.0f});

    tmt::engine.renderer.draw_obb({0.0f, -3.0f, 0.0f}, 1.0f, 1.0f, 1.0f, {1.0f, 0.0f, 0.0f}, glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 0, 1)));
}

void DebugDrawing::on_end() {}
