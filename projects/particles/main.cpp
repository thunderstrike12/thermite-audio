#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/emitter.hpp"
#include "engine/core/components/ui_component.hpp"
#include "engine/core/components/image_renderer.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/camera/camera_system.hpp"

#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/renderer/voxel_object.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class ParticleScene : public tmt::Scene<ParticleScene> {
   public:
    static constexpr std::string_view scene_name() { return "ParticleScene"; }

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;

    tmt::Entity voxel {};
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Particles Test",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    /* Register Systems */
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    /* Register Scenes */
    tmt::engine.scenes.register_scene<ParticleScene>();

    return std::make_unique<Game>(specs);
}

/* Dragon Scene */
void ParticleScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);

        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -120.0f));
    }

    auto asteroid_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "test_asteroid_3.vengi"});
    auto asteroid_volume = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(asteroid_file);

    auto entity = tmt::engine.ecs.create_entity();
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    renderer.resource = asteroid_volume;
}

void ParticleScene::on_update(const tmt::FrameData& /*time*/) {}

void ParticleScene::on_end() {}
