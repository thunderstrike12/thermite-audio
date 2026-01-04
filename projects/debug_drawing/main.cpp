#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/camera/camera_system.hpp"

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

    /* Register Systems */
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    return std::make_unique<DebugDrawing>(specs);
}

void DebugDrawing::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -16.0f));
    }
}

void DebugDrawing::on_update(const tmt::FrameData& time) {
    time_passed += time.delta_time;
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);

    /* Lines */
    tmt::engine.polyline.use_line_width(0.25f);
    tmt::engine.polyline.use_color(1.0f, 0.3f, 0.3f);
    tmt::engine.polyline.draw_line({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
    tmt::engine.polyline.use_color(0.3f, 1.0f, 0.3f);
    tmt::engine.polyline.draw_line({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    tmt::engine.polyline.use_color(0.3f, 0.3f, 1.0f);
    tmt::engine.polyline.draw_line({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});

    /* Circle */
    tmt::engine.polyline.use_color(0.9f, 0.9f, 0.9f);
    tmt::engine.polyline.draw_circle({2.0f, 0.5f, 0.5f}, 0.5f, 64);

    /* Arrow */
    tmt::engine.polyline.use_color(0.9f, 0.9f, 0.9f);
    tmt::engine.polyline.use_line_width(2.5f, true);
    tmt::engine.polyline.draw_arrow({3.25f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}, 1.0f);

    /* AABB */
    tmt::engine.polyline.use_line_width(0.25f);
    tmt::engine.polyline.use_color(1.0f, 0.3f, 0.3f);
    tmt::engine.polyline.draw_aabb({4.0f, 0.0f, 0.0f}, {5.0f, 1.0f, 1.0f});

    /* OBB */
    tmt::engine.polyline.use_color(0.3f, 1.0f, 0.3f);
    tmt::engine.polyline.draw_obb({6.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, glm::angleAxis(0.4f, glm::vec3(1.0f, 0.0f, 0.0f)));

    /* Sphere */
    tmt::engine.polyline.use_color(0.3f, 0.3f, 1.0f);
    tmt::engine.polyline.draw_sphere({8.0f, 0.5f, 0.5f}, 0.5f, 64);

    /* Cone */
    tmt::engine.polyline.use_color(0.9f, 0.9f, 0.9f);
    tmt::engine.polyline.draw_cone({9.0f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, 0.2f, 2.0f, 64);

    /* Tube */
    tmt::engine.polyline.use_color(0.9f, 0.9f, 0.9f);
    tmt::engine.polyline.draw_tube({10.0f, 0.5f, 0.0f}, {10.0f, 0.5f, 1.0f}, 0.1f, 32);
}

void DebugDrawing::on_end() {}
