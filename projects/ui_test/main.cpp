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
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"
#include "engine/tools/player_data.hpp"

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
    tmt::Entity emitter1 {};
    tmt::Entity emitter2 {};
    tmt::Entity ui_entity {};
    float elapsed_time = 0.0f;
};

class MyButton : public tmt::GameComponent<MyButton> {
    // Inherited via GameComponent
   public:
    using GameComponent::GameComponent;

    static std::string_view name() { return "MyButton"; }

    void start() override;
    void update(const tmt::FrameData& time) override {}
    void end() override {}

   private:
    void on_click(tmt::Button::Context context);
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Example Game",
        .organization = "Thermite",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    /* Register Systems */
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    /* Register Scenes */
    tmt::engine.scenes.register_scene<DragonScene>();

    /* Register game components */
    tmt::engine.component_registry.register_component<MyButton>();

    return std::make_unique<Game>(specs);
}

/* Dragon Scene */
void DragonScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);

        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -120.0f));
    }

    auto& health = tmt::engine.player_data.get<int>("Health");
    health = 42;
}

void DragonScene::on_update(const tmt::FrameData& /*time*/) {}

void DragonScene::on_end() {}

void MyButton::start() {
    if (tmt::engine.ecs.has_component<tmt::Button>(entity) == false) return;

    auto& button = tmt::engine.ecs.get_component<tmt::Button>(entity);
    button.on_click.add(this, &MyButton::on_click);
}

void MyButton::on_click(tmt::Button::Context context) {
    tmt::Log::info("Button clicked!");
}
