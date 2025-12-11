#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/animation/animation_system.hpp"

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
    using namespace tmt;
    { /* Camera entity */
        Entity entity = engine.ecs.create_entity("Camera");
        auto& transform = engine.ecs.add_component<Transform>(entity);
        auto& camera = engine.ecs.add_component<Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -16.0f));
    }

    { /* Animation entity */
        auto rig_ent = engine.ecs.create_entity();
        auto& rig_ent_rig_model_comp = engine.ecs.add_component<RigModel>(rig_ent);

        IO::FileLocation location {IO::Location::PROJECT, "Victory_animation.fbx"};
        rig_ent_rig_model_comp.init(location, rig_ent);
        rig_ent_rig_model_comp.data->animation_files.push_back(location);
        rig_ent_rig_model_comp.data->reload();

        rig_ent_rig_model_comp.set_current_animation("Victory_animation");
        rig_ent_rig_model_comp.state = RigModel::State::ANIMATE_LOOP;
        rig_ent_rig_model_comp.time = 0.0f;
    }

    { /* Voxel entity */
        voxel = tmt::engine.ecs.create_entity("Moving Voxel");
        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(voxel);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(voxel);
        // renderer.size = glm::uvec3(64u, 64u, 64u);
        transform.set_world_position(glm::vec3(2.0f, 2.0f, 0.0f));
        transform.set_world_rotation(glm::vec3(glm::radians(45.0f), glm::radians(45.0f), 0.0f));
        transform.set_world_scale(glm::vec3(1.0f, 1.0f, 1.0f));
    }
}

void Game::on_update(const tmt::FrameData& time) {}

void Game::on_end() {}
