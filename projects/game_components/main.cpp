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
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/systems/gameplay/game_component_registry.hpp"

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

class MoveUp : public tmt::GameComponent<MoveUp> {
   public:
    using GameComponent::GameComponent;

    static constexpr std::string_view name() { return "Move Up"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    float speed = 1.0f;
};
TMT_OBJECT(MoveUp, (speed));

class MoveSide : public tmt::GameComponent<MoveSide> {
   public:
    using GameComponent::GameComponent;

    static constexpr std::string_view name() { return "Move Side"; }

    void start() override {};
    void update(const tmt::FrameData& time) override;
    void end() override {};

    float speed = 1.0f;
};
TMT_OBJECT(MoveSide, (speed));

class MoveForward : public tmt::GameComponent<MoveForward> {
   public:
    using GameComponent::GameComponent;

    static constexpr std::string_view name() { return "Move Forward"; }

    void start() override {};
    void update(const tmt::FrameData& time) override;
    void end() override {};

    float speed = 1.0f;
};
TMT_OBJECT(MoveForward, (speed));

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Game Components",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    /* Register Systems */
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    /* Register Scenes */
    tmt::engine.scenes.register_scene<DragonScene>();

    /* Register Components */
    tmt::engine.component_registry.register_component<MoveUp>();
    tmt::engine.component_registry.register_component<MoveSide>();
    tmt::engine.component_registry.register_component<MoveForward>();

    return std::make_unique<Game>(specs);
}

/* Dragon Scene */
void DragonScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);

        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -5.0f));

        tmt::engine.ecs.add_component<MoveUp>(entity);
    }
}

void DragonScene::on_update(const tmt::FrameData&) {}

void DragonScene::on_end() {}

void MoveUp::start() { printf("MoveUp component started on entity %u\n", entity); }

void MoveUp::update(const tmt::FrameData&) {
    const float elapsed_time = tmt::engine.get_elapsed_time();
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto position = transform.get_local_position();
    position.y = std::sin(elapsed_time * speed);
    transform.set_local_position(position);
}

void MoveUp::end() { printf("MoveUp component ended on entity %u\n", entity); }

void MoveSide::update(const tmt::FrameData&) {
    const float elapsed_time = tmt::engine.get_elapsed_time();
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto position = transform.get_local_position();
    position.x = std::sin(elapsed_time * speed);
    transform.set_local_position(position);
}

void MoveForward::update(const tmt::FrameData&) {
    const float elapsed_time = tmt::engine.get_elapsed_time();
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    auto position = transform.get_local_position();
    position.z = std::cos(elapsed_time * speed);
    transform.set_local_position(position);
}
