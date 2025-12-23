#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/camera/camera_system.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class DragonScene : public tmt::Scene<DragonScene> {
   public:
    static constexpr std::string_view scene_name() { return "DragonScene"; }

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;

    tmt::Entity voxel {};
    float elapsed_time = 0.0f;
};

class TableScene : public tmt::Scene<TableScene> {
   public:
    static constexpr std::string_view scene_name() { return "TableScene"; }

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

    /* Register Systems */
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    /* Register Scenes */
    tmt::engine.scenes.register_scene<DragonScene>();
    tmt::engine.scenes.register_scene<TableScene>();

    return std::make_unique<Game>(specs);
}

void generate_random_entities(tmt::ResourceRef<tmt::VoxelVolume>& voxel_volume);

/* Dragon Scene */
void DragonScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);

        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -5.0f));
    }

    tmt::ResourceRef<tmt::VoxelScene> voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "dragon128.vengi"});
    tmt::ResourceRef<tmt::VoxelVolume> voxel_volume = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);

    { /* Voxel entity */
        voxel = tmt::engine.ecs.create_entity("Moving Voxel");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel);
        renderer.resource = voxel_volume;
        transform.set_world_position(glm::vec3(0.0f, 0.0f, 0.0f));
        transform.set_world_scale(glm::vec3(1.0f, 1.0f, 1.0f));
    }

    generate_random_entities(voxel_volume);
}

void DragonScene::on_update(const tmt::FrameData& time) {
    /* Animate the voxel */
    elapsed_time += time.delta_time;
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
    transform.set_world_position(glm::vec3(1.0f, sinf(elapsed_time), 1.0f));
    transform.set_world_rotation(glm::vec3(sinf(elapsed_time), cosf(elapsed_time), 0.0f));
    transform.set_world_scale(glm::vec3(1.0f, 1.5f + sinf(elapsed_time), 1.0f));

    if (tmt::engine.input.is_keyboard_button_released(tmt::Key::SPACE)) {
        tmt::engine.scenes.enqueue_scene<TableScene>();
    }
}

void DragonScene::on_end() {}

/* Sphere Scene */
void TableScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -5.0f));
    }

    auto voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "table.vengi"});
    auto voxel_volume = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);

    generate_random_entities(voxel_volume);
}

void TableScene::on_update(const tmt::FrameData&) {
    if (tmt::engine.input.is_keyboard_button_released(tmt::Key::SPACE)) {
        tmt::engine.scenes.enqueue_scene<DragonScene>();
    }
}

void TableScene::on_end() {}

/* Helper function */
void generate_random_entities(tmt::ResourceRef<tmt::VoxelVolume>& voxel_volume) {
    srand(0);  // Fixed seed for consistent results
    constexpr float PRIM_RANGE = 256.0f;
    for (int i = 0; i < 255; ++i) {
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
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