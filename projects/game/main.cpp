#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"

#include "engine/core/scenes.hpp"
#include "engine/systems/gameplay/game_component.hpp"

// For custom ImGui logic
#include "editor/all.hpp"

// GOAP
#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"
#include "engine/systems/ai/goap/components/goap_goal_registry.hpp"
#include "engine/systems/ai/goap/components/goap_goal.hpp"

// AI actions
#include "ai_actions/chase_player.hpp"
#include "ai_actions/wander.hpp"
#include "ai_actions/fire_missiles.hpp"
#include "ai_actions/fire_laser.hpp"
#include "ai_actions/get_in_laser_range.hpp"
#include "ai_actions/stomp.hpp"
#include "ai_actions/flee.hpp"

// Enemies
#include "components/gameplay_functionality_components/enemy_components/medium_enemy.hpp"
#include "components/gameplay_functionality_components/enemy_components/missile.hpp"
#include "components/gameplay_functionality_components/enemy_components/small_enemy.hpp"

// Animation
#include "components/animation_player.hpp"
// Game Components
#include "components/gameplay_functionality_components/player.hpp"
#include "components/gameplay_functionality_components/mover_component.hpp"
#include "components/gameplay_functionality_components/ore_collector.hpp"
#include "components/gameplay_functionality_components/fuel.hpp"
#include "components/gameplay_functionality_components/upgrade.hpp"
#include "components/gameplay_functionality_components/destroy_timed.hpp"

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
#include "components/ui_components/self_destruct_component.hpp"
#include "components/ui_components/quit_game_component.hpp"
#include "components/ui_components/wallet_ui_link_component.hpp"
#include "components/ui_components/sell_ore_component.hpp"
#include "components/ui_components/button_resource_limit_checker.hpp"
#include "components/ui_components/compass_icon.hpp"
#include "components/ui_components/tutorial_button.hpp"
#include "components/ui_components/resource_bar_current.hpp"
#include "components/ui_components/opacity_fader.hpp"
#include "components/ui_components/depth_tracker.hpp"

// Managers
#include "components/managers/ore_manager.hpp"
#include "components/managers/menu_controller.hpp"
// Development tools
#include "components/development_tools/collision_trigger.hpp"
#include "components/development_tools/attach_component.hpp"
#include "components/development_tools/transform_tween.hpp"
// World generation
#include "components/world_gen/generation_component.hpp"
#include "components/world_gen/random_prefab_spawner.hpp"
// Data Headers
#include "data_headers/scene_list.hpp"
#include "data_headers/wallet.hpp"
#include "data_headers/ore_properties.hpp"
// Game Settings
#include "components/game_settings/audio_volume_change.hpp"
// Goap Components
#include "components/gameplay_functionality_components/weapon_and_tool_components/explosion.hpp"
#include "engine/systems/ai/goap/goap_system.hpp"
#include "components/goap_actions/steer_to_player.hpp"
#include "components/goap_actions/wander_steering.hpp"
#include "components/goap_actions/prepare_explode.hpp"
#include "components/goap_actions/explode.hpp"
#include "components/goap_actions/sensor_system.hpp"
#include "data_headers/save_entries.hpp"
#include "engine/tools/player_data.hpp"

#include <bitset>

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;

    void on_end() override {};
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
    tmt::engine.scenes.register_scene<MainMenuScene>();
    tmt::engine.scenes.register_scene<MainGameScene>();
    tmt::engine.scenes.register_scene<HubScene>();
    tmt::engine.scenes.register_scene<Zoo>();
    tmt::engine.scenes.register_scene<Gym>();
    tmt::engine.scenes.register_scene<DanielTestScene>();
    tmt::engine.scenes.register_scene<MikaTestScene>();
    tmt::engine.scenes.register_scene<LoekTestScene>();
    tmt::engine.scenes.register_scene<LoekTestScene2>();
    tmt::engine.scenes.register_scene<AnneTestScene>();
    tmt::engine.scenes.register_scene<WeaponMotionTest>();
    tmt::engine.scenes.register_scene<BogdanulTestScene>();
    tmt::engine.scenes.register_scene<JaedenTestScene>();

    /* Register Game Components */
    tmt::engine.component_registry.register_component<game::Player>();
    tmt::engine.component_registry.register_component<game::Weapon>();
    tmt::engine.component_registry.register_component<game::Spawner>();
    tmt::engine.component_registry.register_component<game::RifleProjectile>();
    tmt::engine.component_registry.register_component<game::ProjectileSpawner>();
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
    tmt::engine.component_registry.register_component<game::MediumEnemy>();
    tmt::engine.component_registry.register_component<game::Missile>();
    tmt::engine.component_registry.register_component<game::SmallEnemy>();
    tmt::engine.component_registry.register_component<game::OreProperties>();
    tmt::engine.component_registry.register_component<game::OreManager>();
    tmt::engine.component_registry.register_component<game::Explosion>();
    tmt::engine.component_registry.register_component<game::DestroyTimed>();
    tmt::engine.component_registry.register_component<game::TransformTween>();
    /* Register World Generation Components */
    tmt::engine.component_registry.register_component<game::GenerationComponent>();
    tmt::engine.component_registry.register_component<game::RandomPrefabSpawner>();
    /* Register Game UI Components */
    tmt::engine.component_registry.register_component<game::SceneSwitchComponent>();
    tmt::engine.component_registry.register_component<game::EntityControlComponent>();
    tmt::engine.component_registry.register_component<game::SelfDestructComponent>();
    tmt::engine.component_registry.register_component<game::QuitGameComponent>();
    tmt::engine.component_registry.register_component<game::WalletUiLink>();
    tmt::engine.component_registry.register_component<game::SellOreComponent>();
    tmt::engine.component_registry.register_component<game::ResourceLimitChecker>();
    tmt::engine.component_registry.register_component<game::CompassIcon>();
    tmt::engine.component_registry.register_component<game::TutorialButton>();
    tmt::engine.component_registry.register_component<game::ResourceBarCurrentComponent>();
    tmt::engine.component_registry.register_component<game::OpacityFader>();
    tmt::engine.component_registry.register_component<game::DepthTracker>();

    /* Register Game Settings */
    tmt::engine.component_registry.register_component<game::AudioVolumeChangeComponent>();

#if THERMITE_EDITOR
    tmt::editor.systems[tmt::Editor::Mode::SCENE].add<tmt::LevelEditor>();
#endif

    /* Register Goap Components */
    {
        if (!tmt::engine.ecs.systems.try_get<tmt::Goap>()) {
            tmt::engine.ecs.systems.add<tmt::Goap>();
        }

        /* Add sensor system specific for this game */
        if (!tmt::engine.ecs.systems.try_get<game::SensorsSystem>()) {
            tmt::engine.ecs.systems.add<game::SensorsSystem>();
        }

        auto& ecs = tmt::engine.ecs;
        auto& goap = ecs.systems.get<tmt::Goap>();

        auto& action_reg = goap.actions();
        auto& goal_reg = goap.goals();
        auto& type_reg = goap.agent_types();

        /* Register Actions */
        action_reg.register_action(std::make_unique<game::SteerToPlayer>());
        action_reg.register_action(std::make_unique<game::WanderSteering>());
        action_reg.register_action(std::make_unique<game::PrepareExplode>());
        action_reg.register_action(std::make_unique<game::Explode>());

        /* Register Goals */

        // Goal: Wander when player not in range
        {
            tmt::GoapGoal wander_steering;
            wander_steering.name = "g_WanderSteering";
            wander_steering.desired_state = { { tmt::FactId("s_wandering"), true } };
            wander_steering.priority = 1;
            wander_steering.valid = true;
            goal_reg.register_goal("g_WanderSteering", wander_steering);
        }
        // Goal: Steer to player, get within explosion range
        {
            tmt::GoapGoal steer_to_player;
            steer_to_player.name = "g_SteerToPlayer";
            steer_to_player.desired_state = { { tmt::FactId("s_player_in_explosion_zone"), true } };
            steer_to_player.priority = 5;
            steer_to_player.valid = true;
            goal_reg.register_goal("g_ReachTarget", steer_to_player);
        }
        // Goal: Explode when in explosion range
        {
            tmt::GoapGoal explode;
            explode.name = "g_Explode";
            explode.desired_state = { { tmt::FactId("s_exploded"), true } };
            explode.priority = 10;
            explode.valid = true;
            goal_reg.register_goal("g_Explode", explode);
        }

        // Medium enemy
        action_reg.register_action(std::make_unique<ChasePlayer>());
        action_reg.register_action(std::make_unique<Wander>());
        action_reg.register_action(std::make_unique<FireMissiles>());
        action_reg.register_action(std::make_unique<FireLaser>());
        action_reg.register_action(std::make_unique<GetInLaserRange>());
        action_reg.register_action(std::make_unique<Stomp>());
        action_reg.register_action(std::make_unique<Flee>());

        // Goap stuff
        {
            tmt::GoapGoal chase;
            chase.name = "g_ChasePlayer";
            chase.desired_state = { { tmt::FactId("m_get_close_to_player"), true } };
            chase.priority = 5;
            chase.valid = true;

            goal_reg.register_goal("g_ChasePlayer", chase);
        }

        {
            tmt::GoapGoal wander;
            wander.name = "g_Wander";
            wander.desired_state = { { tmt::FactId("m_wandering"), true } };
            wander.priority = 1;
            wander.valid = true;

            goal_reg.register_goal("g_Wander", wander);
        }
        {
            tmt::GoapGoal kill_player;
            kill_player.name = "g_KillPlayer";
            kill_player.desired_state = { { tmt::FactId("m_kill_player"), true } };
            kill_player.priority = 50;
            kill_player.valid = true;

            goal_reg.register_goal("g_KillPlayer", kill_player);
        }

        {
            tmt::GoapGoal get_in_laser_range;
            get_in_laser_range.name = "g_GetInLaserRange";
            get_in_laser_range.desired_state = { { tmt::FactId("m_in_laser_range"), true } };
            get_in_laser_range.priority = 10;
            get_in_laser_range.valid = true;

            goal_reg.register_goal("g_GetInLaserRange", get_in_laser_range);
        }

        {
            tmt::GoapGoal stay_safe;
            stay_safe.name = "g_StaySafe";
            stay_safe.desired_state = { { tmt::FactId("m_stay_safe"), true } };
            stay_safe.priority = 40;
            stay_safe.valid = true;

            goal_reg.register_goal("g_StaySafe", stay_safe);
        }
    }

    return std::make_unique<Game>(specs);
}

void Game::on_start() {
#ifdef THERMITE_EDITOR
#else
    tmt::engine.window.fullscreen_window(true);
#endif
}

void Game::on_update(const tmt::FrameData&) {
    const bool f11_down = tmt::engine.input.is_keyboard_button_just_pressed(tmt::Key::F11);
    if (f11_down) {
        tmt::engine.window.toggle_fullscreen();
    }
}