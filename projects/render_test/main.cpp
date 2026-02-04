#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/camera/camera_system.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class MainScene : public tmt::Scene<MainScene> {
   public:
    static constexpr std::string_view scene_name() { return "MainScene"; }

    tmt::ResourceRef<tmt::VoxelVolume> box_model;

    float time_passed = 0.0f;
    float cooldown = 0.0f;

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Render Test",
        .command_args = args,
        .log_file = "render_test_logs.txt"
    };
    // clang-format on

    /* Register Systems */
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    /* Register Scenes */
    tmt::engine.scenes.register_scene<MainScene>();

    return std::make_unique<Game>(specs);
}

void MainScene::on_start() {
    /* Load the throwable box model */
    const tmt::ResourceRef box_scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "box-10.vengi"});
    box_model = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(box_scene);
}

void MainScene::on_update(const tmt::FrameData& time) {
    /* Update the time elapsed and cooldown timer */
    time_passed += time.delta_time;
    cooldown -= time.delta_time;

    if (tmt::engine.input.is_mouse_button_pressed(tmt::MouseButton::LEFT) && cooldown <= 0.0f) {
        /* Get the camera position and forward direction */
        const tmt::Entity camera = tmt::Camera::get_active_camera();
        const tmt::Transform& cam_transform = tmt::engine.ecs.get_component<tmt::Transform>(camera);
        const glm::vec3 cam_pos = cam_transform.get_world_position();
        const glm::vec3 cam_forward = cam_transform.get_forward();

        /* Spawn box models with forward force */
        for (int x = -2; x <= 2; x++) {
            for (int y = -2; y <= 2; y++) {
                auto entity = tmt::engine.ecs.create_entity("Crate");
                auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
                auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
                renderer.resource = box_model;
                auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
                vb.resource = box_model;
                vb.gravity = 0.0f;
                vb.type = tmt::VoxelBody::DYNAMIC;
                tmt::Physics::initialize_voxel_body(vb);

                const glm::vec3 x_offset = cam_transform.get_right() * ((float)x * 1.5f);
                const glm::vec3 y_offset = cam_transform.get_up() * ((float)y * 1.5f);

                tmt::Physics::set_position(vb, cam_pos + cam_forward * 2.0f + x_offset + y_offset);

                float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f;
                float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f;
                float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f;

                tmt::Physics::set_rotation(vb, cam_transform.get_world_rotation() * glm::vec3(random_rot_x, random_rot_y, random_rot_z));
                tmt::Physics::add_force(vb, cam_forward * 20.0f);
            }
        }

        cooldown = 0.10f;
    }
}

void MainScene::on_end() {}
