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

// Game Components
#include "components/player.hpp"
#include "components/wallet.hpp"
#include "components/animation_player.hpp"
#include "components/projectile_spawner.hpp"
#include "components/rifle_projectile.hpp"
#include "components/spawner.hpp"
#include "components/weapon.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class MainMenuScene : public tmt::Scene<MainMenuScene> {
   public:
    static constexpr std::string_view scene_name() { return "MainMenuScene"; }
};

class MainGameScene : public tmt::Scene<MainGameScene> {
   public:
    static constexpr std::string_view scene_name() { return "MainGameScene"; }
};

class EntityRef : public tmt::GameComponent<EntityRef> {
   public:
    using GameComponent::GameComponent;

    tmt::Entity entity_ref = entt::null;

    static constexpr std::string_view get_name() { return "EntityRef"; }

    // Inherited via GameComponent
    void start() override {
        const auto& name = tmt::engine.ecs.get_component<tmt::Name>(entity_ref);
        tmt::Log::info("EntityRef Component started! Referenced entity name: {}", name.name);
    }
    void update(const tmt::FrameData& time) override {}
    void end() override {}
};
TMT_OBJECT(EntityRef, (entity_ref));

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Mining Game",
        .command_args = args,
        .log_file = "mining_game_logs.txt"
    };
    // clang-format on

    /* Register Scenes */
    tmt::engine.scenes.register_scene<MainGameScene>();
    tmt::engine.scenes.register_scene<MainMenuScene>();

    /* Register Components */
    tmt::engine.component_registry.register_component<game::Player>();
    tmt::engine.component_registry.register_component<game::Weapon>();
    tmt::engine.component_registry.register_component<game::Spawner>();
    tmt::engine.component_registry.register_component<game::RifleProjectile>();
    tmt::engine.component_registry.register_component<game::ProjectileSpawner>();
    tmt::engine.component_registry.register_component<Wallet>();
    tmt::engine.component_registry.register_component<AnimationPlayer>();

    /* Register Game Components */
    tmt::engine.component_registry.register_component<EntityRef>();

    return std::make_unique<Game>(specs);
}
