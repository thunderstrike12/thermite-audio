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
#include "engine/core/renderer/voxel_object.hpp"
#include "engine/systems/gameplay/game_component.hpp"

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

class EntityRef : public tmt::GameComponent<EntityRef> {
   public:
    using GameComponent::GameComponent;

    tmt::Entity entity_ref = entt::null;
    std::set<tmt::Entity> entity_set {};

    static constexpr std::string_view get_name() { return "EntityRef"; }

    // Inherited via GameComponent
    void start() override {
        const auto& name = tmt::engine.ecs.get_component<tmt::Name>(entity_ref);
        tmt::Log::info("EntityRef Component started! Referenced entity name: {}", name.name);
    }
    void update(const tmt::FrameData& time) override {}
    void end() override {}
};
TMT_OBJECT(EntityRef, (entity_ref, entity_set));

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

    /* Register Game Components */
    tmt::engine.component_registry.register_component<EntityRef>();

    return std::make_unique<Game>(specs);
}

#include "engine/core/resources/texture_2d.hpp"

/* Dragon Scene */
void DragonScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);

        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -120.0f));
    }
}

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/render_view.hpp"
#include "engine/shared/ray.hpp"

void DragonScene::on_update(const tmt::FrameData& /*time*/) {}

void DragonScene::on_end() {}
