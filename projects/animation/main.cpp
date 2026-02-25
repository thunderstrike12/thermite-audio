#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
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
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -16.0f));
    }

    //{ /* Animation entity */
    //    auto rig_ent = tmt::engine.ecs.create_entity();
    //    auto& rig_ent_rig_model_comp = tmt::engine.ecs.add_component<tmt::RigModel>(rig_ent);

    //    tmt::IO::FileLocation location { tmt::IO::Location::PROJECT, "Victory_animation.fbx" };
    //    rig_ent_rig_model_comp.init(location, rig_ent);
    //    //rig_ent_rig_model_comp.data->animation_files.push_back(location);
    //    //rig_ent_rig_model_comp.data->reload();

    //    rig_ent_rig_model_comp.set_current_animation("Victory_animation");
    //    rig_ent_rig_model_comp.state = tmt::RigModel::State::ANIMATE_LOOP;
    //    rig_ent_rig_model_comp.time = 0.0f;
    //}
}

void Game::on_update(const tmt::FrameData& time) {}

void Game::on_end() {}