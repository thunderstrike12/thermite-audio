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

// For custom ImGui logic
#include "editor/all.hpp"

// Game Components
#include "components/player.hpp"
#include "components/wallet.hpp"
#include "components/asteroid_field_component.hpp"
#include "components/mining_component.hpp"
#include "components/animation_player.hpp"
#include "components/gravity_manipulation_component.hpp"
#include "components/collision_trigger.hpp"
#include "components/ore_collector.hpp"
#include "components/projectile_spawner.hpp"
#include "components/rifle_projectile.hpp"
#include "components/spawner.hpp"
#include "components/text_component.hpp"
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
class Zoo : public tmt::Scene<Zoo> {
   public:
    static constexpr std::string_view scene_name() { return "Zoo"; }
};
class Gym : public tmt::Scene<Gym> {
   public:
    static constexpr std::string_view scene_name() { return "Gym"; }
};

class DanielTestScene : public tmt::Scene<DanielTestScene> {
   public:
    static constexpr std::string_view scene_name() { return "DanielTestScene"; }
};

class MikaTestScene : public tmt::Scene<MikaTestScene> {
   public:
    static constexpr std::string_view scene_name() { return "MikaTestScene"; }
};

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
    tmt::engine.scenes.register_scene<DanielTestScene>();
    tmt::engine.scenes.register_scene<MikaTestScene>();
    tmt::engine.scenes.register_scene<DanielTestScene>();
    tmt::engine.scenes.register_scene<MainMenuScene>();
    tmt::engine.scenes.register_scene<Gym>();
    tmt::engine.scenes.register_scene<Zoo>();

    /* Register Game Components */
    tmt::engine.component_registry.register_component<game::Player>();
    tmt::engine.component_registry.register_component<game::Weapon>();
    tmt::engine.component_registry.register_component<game::Spawner>();
    tmt::engine.component_registry.register_component<game::RifleProjectile>();
    tmt::engine.component_registry.register_component<game::ProjectileSpawner>();
    tmt::engine.component_registry.register_component<game::AsteroidFieldComponent>();
    tmt::engine.component_registry.register_component<game::Wallet>();
    tmt::engine.component_registry.register_component<game::MiningComponent>();
    tmt::engine.component_registry.register_component<game::AnimationPlayer>();
    tmt::engine.component_registry.register_component<game::GravityManipulationComponent>();
    tmt::engine.component_registry.register_component<game::OreCollector>();
    tmt::engine.component_registry.register_component<game::CollisionTrigger>();
    tmt::engine.component_registry.register_component<game::WeaponManager>();
    tmt::engine.component_registry.register_component<game::TextComponent>();

    return std::make_unique<Game>(specs);
}
