#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input.hpp"
#include "engine/core/logger.hpp"

#include "engine/systems/ai/goap_system.hpp"
#include "engine/systems/ai/components/goap_agent.hpp"
#include "goap_actions/chase_player.hpp"
#include "goap_actions/kill_player.hpp"
#include "goap_actions/patrol_area.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    tmt::Entity voxel {};
    float time_passed = 0.0f;

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

    return std::make_unique<Game>(specs);
}

void Game::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -2.0f));
    }

    // { /* Voxel entity */
    //     voxel = tmt::engine.ecs.create_entity();
    //     auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(voxel);
    //     auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel);
    //     renderer.size = glm::uvec3(20u, 10u, 10u);
    //     transform.set_world_position(glm::vec3(2.0f, 2.0f, 0.0f));
    //     transform.set_world_rotation(glm::vec3(glm::radians(45.0f), glm::radians(45.0f), 0.0f));
    //     transform.set_world_scale(glm::vec3(1.0f, 2.0f, 1.0f));
    // }

    // constexpr float PRIM_RANGE = 128.0f;
    // for (int i = 0; i < 255; ++i) {
    //     auto entity = tmt::engine.ecs.create_entity();
    //     auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
    //     auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //     renderer.size = glm::uvec3(10u + (rand() % 90u), 10u + (rand() % 90u), 10u + (rand() % 90u));
    //     float rx = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
    //     float ry = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
    //     float rz = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * PRIM_RANGE - (PRIM_RANGE / 2.0f);
    //     float rrx = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 180.0f;
    //     float rry = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 180.0f;
    //     transform.set_world_position(glm::vec3(rx, ry, rz));
    //     transform.set_world_rotation(glm::vec3(glm::radians(rrx), glm::radians(rry), 0.0f));
    //     transform.set_world_scale(glm::vec3(1.0f, 1.0f, 1.0f));
    // }

    // Register agents
    {
        // Create agents
        tmt::Entity ai = tmt::engine.ecs.create_entity();

        // Give it whatever normal game components it needs
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(ai);

        // Add GOAP agent
        auto& agent = tmt::engine.ecs.add_component<tmt::GoapAgent>(ai);

        // Add world state (facts relevant to planning)
        auto& ws = tmt::engine.ecs.add_component<tmt::WorldState>(ai);

        transform.set_world_position({0.f, 0.f, 0.f});

        // --- Give the agent actions ---
        agent.actions.push_back(std::make_unique<tmt::PatrolArea>());
        agent.actions.push_back(std::make_unique<tmt::ChasePlayer>());
        agent.actions.push_back(std::make_unique<tmt::KillPlayer>());

        // --- Give the agent some goals ---
        tmt::GoapGoal patrol_goal;
        patrol_goal.name = "patrol area";
        patrol_goal.desired_state = {{{"area_secure"}, true}};
        patrol_goal.priority = 1;
        patrol_goal.valid = true;

        tmt::GoapGoal kill_goal;
        kill_goal.name = "kill player";
        kill_goal.desired_state = {{{"player_alive"}, false}};
        kill_goal.priority = 10;
        kill_goal.valid = true;

        agent.available_goals.push_back(patrol_goal);
        agent.available_goals.push_back(kill_goal);

        // --- Setup initial world state ---
        ws.facts[std::hash<std::string>()("player_visible")] = true;
        ws.facts[std::hash<std::string>()("player_in_range")] = false;
        ws.facts[std::hash<std::string>()("player_alive")] = true;
        ws.facts[std::hash<std::string>()("area_secure")] = false;

        tmt::Log::info("GOAP Agent created!");
    }
}

void Game::on_update(const tmt::FrameData& time) {
    // time_passed += time.delta_time;
    // auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
    // transform.set_world_position(glm::vec3(1.0f, sinf(time_passed), 1.0f));
    // transform.set_world_rotation(glm::vec3(0.0f, cosf(time_passed), 0.0f));
    // transform.set_world_scale(glm::vec3(1.0f, 1.5f + sinf(time_passed), 1.0f));
}

void Game::on_end() {}
