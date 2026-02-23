#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/systems/camera/camera_system.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/input/input.hpp"

#include "engine/systems/ai/goap/goap_system.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"
#include "engine/systems/ai/goap/components/goap_agent_type_registry.hpp"
#include "engine/systems/ai/goap/components/goap_agent_factory.hpp"
#include "engine/systems/ai/goap/components/goap_action_registry.hpp"
#include "engine/systems/ai/goap/components/goap_goal_registry.hpp"
#include "goap_actions/chase_player.hpp"
#include "goap_actions/kill_player.hpp"
#include "goap_actions/patrol_area.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"

#include "engine/systems/ai/navigation/navigation_system.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    float time_passed = 0.0f;

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class AIScene : public tmt::Scene<AIScene> {
   public:
    static constexpr std::string_view scene_name() { return "AIScene"; }

    tmt::Entity voxel {};
    tmt::NavMesh* nav_mesh;
    tmt::ResourceRef<tmt::VoxelVolume> volumes[3];
    int index = 0;
    void on_start() override;
    void on_update(const tmt::FrameData&) override {};
    void on_end() override {};
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Example Game",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    tmt::engine.ecs.systems.add<tmt::CameraSystem>();
    tmt::engine.scenes.register_scene<AIScene>();

    tmt::engine.ecs.systems.add<tmt::Goap>();

    auto& ecs = tmt::engine.ecs;
    auto& goap = ecs.systems.get<tmt::Goap>();

    auto& action_reg = goap.actions();
    auto& goal_reg = goap.goals();
    auto& type_reg = goap.agent_types();

    // --- Register Actions ---
    action_reg.register_action(std::make_unique<tmt::PatrolArea>());
    action_reg.register_action(std::make_unique<tmt::ChasePlayer>());
    action_reg.register_action(std::make_unique<tmt::KillPlayer>());

    // --- Register Goals ---
    {
        tmt::GoapGoal patrol;
        patrol.name = "g_PatrolArea";
        patrol.desired_state = { { tmt::FactId("area_secure"), true } };
        patrol.priority = 1;
        patrol.valid = true;

        goal_reg.register_goal("g_PatrolArea", patrol);
    }

    {
        tmt::GoapGoal kill;
        kill.name = "g_KillPlayer";
        kill.desired_state = { { tmt::FactId("player_alive"), false } };
        kill.priority = 10;
        kill.valid = true;

        goal_reg.register_goal("g_KillPlayer", kill);
    }

    return std::make_unique<Game>(specs);
}

void AIScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -2.0f));
    }

#if 0
    { // --- Old example of angent type creation ---
        // --- Register Agent Types ---

        tmt::GoapAgentType enemy1;
        enemy1.id = "Enemy 1";

        enemy1.action_ids = {"a_PatrolArea", "a_ChasePlayer", "a_KillPlayer"};

        enemy1.goal_ids = {"g_PatrolArea", "g_KillPlayer"};

        // NOTE: You really shouldn't convert a 64bit hash to 32bits!!!!
        enemy1.default_world_state = { { (uint32_t)std::hash<std::string>()("player_visible"), true },
                                       { (uint32_t)std::hash<std::string>()("player_in_range"), false },
                                       { (uint32_t)std::hash<std::string>()("player_alive"), true },
                                       { (uint32_t)std::hash<std::string>()("area_secure"), false } };

        type_reg.register_type(enemy1);

        tmt::GoapAgentType enemy2;
        enemy2.id = "Enemy 2";

        enemy2.action_ids = {"a_PatrolArea"};

        enemy2.goal_ids = {"g_PatrolArea"};

        enemy2.default_world_state = { { (uint32_t)std::hash<std::string>()("area_secure"), false } };

        type_reg.register_type(enemy2);

        // Spawn agents from type registry via factory
        tmt::Entity ai1 = tmt::engine.ecs.create_entity("AI Agent 1");
        tmt::GoapAgentFactory::spawn_agent_from_type("Enemy 1", ai1);
        tmt::Entity ai2 = tmt::engine.ecs.create_entity("AI Agent 2");
        tmt::GoapAgentFactory::spawn_agent_from_type("Enemy 2", ai2);

        // Set unique positions
        ecs.get_component<tmt::Transform>(ai1).set_world_position({ 0.f, 0.f, 0.f });
        ecs.get_component<tmt::Transform>(ai2).set_world_position({ 5.f, 0.f, 0.f });
    }
#else
#endif

    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);

        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -5.0f));
    }

    {  // voxel entity with navmesh
        auto voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "test_asteroid_5.vengi" });
        auto voxel_volume = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);
        volumes[0] = voxel_volume;
        voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "test_asteroid_6.vengi" });
        voxel_volume = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);
        volumes[1] = voxel_volume;
        voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "test_asteroid_7.vengi" });
        voxel_volume = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);
        volumes[2] = voxel_volume;
        voxel = tmt::engine.ecs.create_entity("Moving Voxel");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel);
        renderer.resource = voxel_volume;
        // transform.set_world_rotation(glm::vec3(glm::radians(45.0f), glm::radians(45.0f), 0.0f));
        transform.set_world_scale(glm::vec3(1.0f, 1.0f, 1.0f));

        nav_mesh = &tmt::engine.ecs.add_component<tmt::NavMesh>(voxel);
        nav_mesh->voxel_volume = voxel_volume;
        nav_mesh->lod_level = 1;
        nav_mesh->generate_mesh_over_time();
    }
}
