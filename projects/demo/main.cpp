#include "engine/entry_point.hpp"
#include <glm/glm.hpp>
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/components/camera.hpp"

#include "engine/core/resources/voxel_volume.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/systems/camera/camera_system.hpp"

#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"
#include "engine/systems/ai/goap/components/goap_agent_type_registry.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"
#include "engine/systems/ai/goap/components/goap_goal_registry.hpp"
#include "engine/systems/ai/goap/components/goap_goal.hpp"

/* Components */
#include "components/player.hpp"
#include "components/asteroid_spawner.hpp"
#include "components/animation_player.hpp"
#include "components/walking.hpp"

#include "ai_actions/chase_player.hpp"
#include "ai_actions/wander.hpp"

class Demo : public tmt::Application {
   public:
    Demo(const tmt::ApplicationSpecs& specs) : Application(specs) {}
    void on_start() override {
        /* Register actions */
        auto& ecs = tmt::engine.ecs;
        auto& goap = ecs.systems.get<tmt::Goap>();

        auto& action_reg = goap.actions();
        auto& goal_reg = goap.goals();
        auto& type_reg = goap.agent_types();

        action_reg.register_action(std::make_unique<ChasePlayer>());
        action_reg.register_action(std::make_unique<Wander>());

        tmt::GoapAgentType dragon;
        dragon.id = "dragon";
        dragon.action_ids = { "ChasePlayer", "Wander" };

        dragon.default_world_state = {
            { "player_in_range", false },
            { "in_attack_range", false },
        };

        {
            tmt::GoapGoal chase;
            chase.name = "ChasePlayer";
            chase.desired_state = { { tmt::FactId("in_attack_range"), true } };
            chase.priority = 10;
            chase.valid = true;

            goal_reg.register_goal("ChasePlayer", chase);
        }

        {
            tmt::GoapGoal wander;
            wander.name = "Wander";
            wander.desired_state = { { tmt::FactId("player_in_range"), true } };
            wander.priority = 1;
            wander.valid = true;

            goal_reg.register_goal("Wander", wander);
        }

        dragon.goal_ids = { "ChasePlayer", "Wander" };

        type_reg.register_type(dragon);
    }
};

class MainScene : public tmt::Scene<MainScene> {
   public:
    static constexpr std::string_view scene_name() { return "MainScene"; }
};

class

    std::unique_ptr<tmt::Application>
    create_application(const tmt::CommandLineArgs& args) {
    tmt::ApplicationSpecs specs {
        .name = "Demo",
        .command_args = args,
        .log_file = "demo_logs.txt",
    };

    /* Register Scenes */
    tmt::engine.scenes.register_scene<MainScene>();

    /*Regsiter Components */
    tmt::engine.component_registry.register_component<Player>();
    tmt::engine.component_registry.register_component<AsteroidSpawner>();
    tmt::engine.component_registry.register_component<AnimationPlayer>();
    tmt::engine.component_registry.register_component<Walking>();

    return std::make_unique<Demo>(specs);
}
