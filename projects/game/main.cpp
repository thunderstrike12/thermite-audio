#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"

#include "engine/core/scenes.hpp"
#include "engine/systems/gameplay/game_component.hpp"

// For custom ImGui logic
#include "editor/all.hpp"
// Animation
#include "components/animation_player.hpp"
// Game Components
#include "components/gameplay_functionality_components/player.hpp"
#include "components/gameplay_functionality_components/mover_component.hpp"
#include "components/gameplay_functionality_components/ore_collector.hpp"
#include "components/gameplay_functionality_components/fuel.hpp"
#include "components/gameplay_functionality_components/upgrade.hpp"
// Weapons and tools
#include "components/gameplay_functionality_components/weapon_and_tool_components/projectile_spawner.hpp"
#include "components/gameplay_functionality_components/weapon_and_tool_components/rifle_projectile.hpp"
#include "components/gameplay_functionality_components/weapon_and_tool_components/spawner.hpp"
#include "components/gameplay_functionality_components/weapon_and_tool_components/weapon.hpp"
#include "components/gameplay_functionality_components/weapon_and_tool_components/mining_component.hpp"
#include "components/gameplay_functionality_components/weapon_and_tool_components/gravity_manipulation_component.hpp"
// Ui
#include "components/ui_components/text_component.hpp"
#include "components/ui_components/scene_switch_component.hpp"
#include "components/ui_components/entity_control_component.hpp"
// Managers
#include "components/managers/ore_manager.hpp"
#include "components/managers/menu_controller.hpp"
// Development tools
#include "components/development_tools/collision_trigger.hpp"
#include "components/development_tools/attach_component.hpp"
// World generation
#include "components/world_gen/asteroid_field_component.hpp"
// Data Headers
#include "data_headers/scene_list.hpp"
#include "data_headers/wallet.hpp"
#include "data_headers/ore_properties.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

// Moved scenes to data_headers/scene_list.hpp

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Mining Game",
        .command_args = args,
        .log_file = "mining_game_logs.txt"
    };
    // clang-format on

    /* Register Scenes */
    tmt::engine.scenes.register_scene<MainMenuScene>();
    tmt::engine.scenes.register_scene<MainGameScene>();
    tmt::engine.scenes.register_scene<Zoo>();
    tmt::engine.scenes.register_scene<Gym>();
    tmt::engine.scenes.register_scene<DanielTestScene>();
    tmt::engine.scenes.register_scene<MikaTestScene>();

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
    tmt::engine.component_registry.register_component<game::MoverComponent>();
    tmt::engine.component_registry.register_component<game::AttachComponent>();
    tmt::engine.component_registry.register_component<game::Upgrade>();
    tmt::engine.component_registry.register_component<game::MenuController>();
    tmt::engine.component_registry.register_component<game::FuelComponent>();
    tmt::engine.component_registry.register_component<game::OreProperties>();
    tmt::engine.component_registry.register_component<game::OreManager>();
    /* Register Game UI Components */
    tmt::engine.component_registry.register_component<game::SceneSwitchComponent>();
    tmt::engine.component_registry.register_component<game::EntityControlComponent>();

    return std::make_unique<Game>(specs);
}
