#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"

#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"
#include "goap_actions/chase_player.hpp"
#include "goap_actions/kill_player.hpp"
#include "goap_actions/patrol_area.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {
        auto& registry = tmt::GoapActionRegistry::instance();

        // --- Register actions ---
        registry.register_action(std::make_unique<tmt::PatrolArea>());
        registry.register_action(std::make_unique<tmt::ChasePlayer>());
        registry.register_action(std::make_unique<tmt::KillPlayer>());
    }

    tmt::Entity voxel {};
    float time_passed = 0.0f;

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class AIScene : public tmt::Scene<AIScene> {
   public:
    static constexpr std::string_view scene_name() { return "AIScene"; }

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

    tmt::engine.scenes.register_scene<AIScene>();

    return std::make_unique<Game>(specs);
}

void AIScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -2.0f));
    }

    // Regiser agents
    {
        auto& ecs = tmt::engine.ecs;
        auto& registry = tmt::GoapActionRegistry::instance();

        // --- Agent 1 ---
        tmt::Entity ai = ecs.create_entity();
        auto& transform = ecs.get_component<tmt::Transform>(ai);
        auto& agent = ecs.add_component<tmt::GoapAgent>(ai);
        auto& ws = ecs.add_component<tmt::WorldState>(ai);
        transform.set_world_position({0.f, 0.f, 0.f});

        // Define which actions this agent can use
        std::vector<std::string> agent1_actions = {"KillPlayer", "PatrolArea", "ChasePlayer"};
        for (auto id : agent1_actions) {
            if (auto* action = registry.get(id)) {
                agent.available_actions.push_back(action);
            }
        }

        // Define goals
        tmt::GoapGoal patrol_goal;
        patrol_goal.name = "patrol area";
        patrol_goal.desired_state = {{tmt::FactId("area_secure"), tmt::FactValue(true)}};
        patrol_goal.priority = 1;
        patrol_goal.valid = true;

        tmt::GoapGoal kill_goal;
        kill_goal.name = "kill player";
        kill_goal.desired_state = {{tmt::FactId("player_alive"), tmt::FactValue(false)}};
        kill_goal.priority = 10;
        kill_goal.valid = true;

        agent.available_goals.push_back(patrol_goal);
        agent.available_goals.push_back(kill_goal);

        // Setup initial world state
        ws.facts[std::hash<std::string>()("player_visible")] = true;
        ws.facts[std::hash<std::string>()("player_in_range")] = false;
        ws.facts[std::hash<std::string>()("player_alive")] = true;
        ws.facts[std::hash<std::string>()("area_secure")] = false;

        // --- Agent 2 ---
        tmt::Entity ai2 = ecs.create_entity();
        auto& transform2 = ecs.get_component<tmt::Transform>(ai2);
        auto& agent2 = ecs.add_component<tmt::GoapAgent>(ai2);
        auto& ws2 = ecs.add_component<tmt::WorldState>(ai2);
        transform2.set_world_position({5.f, 0.f, 0.f});  // different position

        // Assign different actions
        std::vector<std::string> agent2_actions = {"KillPlayer", "PatrolArea", "ChasePlayer"};
        for (auto id : agent2_actions) {
            if (auto* action = registry.get(id)) {
                agent2.available_actions.push_back(action);
            }
        }

        agent2.available_goals.push_back(patrol_goal);
        agent2.available_goals.push_back(kill_goal);

        ws2.facts[std::hash<std::string>()("player_visible")] = true;
        ws2.facts[std::hash<std::string>()("player_in_range")] = false;
        ws2.facts[std::hash<std::string>()("player_alive")] = true;
        ws2.facts[std::hash<std::string>()("area_secure")] = false;
    }
}

void AIScene::on_update(const tmt::FrameData& time) {}

void AIScene::on_end() {}
