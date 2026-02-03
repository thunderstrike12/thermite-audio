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
#include "engine/systems/physics/physics_system.hpp"
#include "engine/core/polyline.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    std::vector<tmt::Entity> voxels = {};
    tmt::Entity cam;
    float time_passed = 0.0f;
    float cooldown = 0.0f;

    tmt::ResourceRef<tmt::VoxelVolume> voxel_volume_cube;

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

    return std::make_unique<Game>(specs);
}

void Game::on_start() {
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    { /* Camera entity */
        cam = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(cam);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(cam);
        transform.set_world_position(glm::vec3(0.0f, 0.25f, -5.0f));
    }

    auto voxel_file_cube = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "box-10.vengi"});
    voxel_volume_cube = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_cube);

    auto voxel_file_ass3 = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "test_asteroid_3.vengi"});
    auto voxel_volume_ass3 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_ass3);

    auto voxel_file_ass7 = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "test_asteroid_7.vengi"});
    auto voxel_volume_ass7 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_ass7);
    
    auto voxel_file_1x1x1 = tmt::engine.resources.load_resource<tmt::VoxelScene>({tmt::IO::Location::PROJECT, "1x1x1.vengi"});
    auto voxel_volume_1x1x1 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_1x1x1);

    { /* 1x1x1 voxel entity */
        auto entity = tmt::engine.ecs.create_entity("1x1x1");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_1x1x1;
    }

    { /* Voxel Physics Entity, Asteroid 3 */
        auto entity = tmt::engine.ecs.create_entity("Asteroid");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass3;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass3;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(0.0f, 50.0f, 110.0f));

        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 3 */
        auto entity = tmt::engine.ecs.create_entity("Asteroid");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass3;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass3;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(40.0f, 20.0f, 100.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 3 */
        auto entity = tmt::engine.ecs.create_entity("Asteroid");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass3;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass3;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(-24.0f, -20.0f, 90.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 (close to test) */
        auto entity = tmt::engine.ecs.create_entity("Asteroid");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(50.0f, 31.6f, 100.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity("Asteroid");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(20.0f, -35.0f, 120.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity("Asteroid");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(-20.0f, 55.0f, 100.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity("Asteroid");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(-14.0f, 17.0f, 70.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity("Asteroid");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(19.0f, 0.0f, 90.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }

    { /* Voxel Physics Entity, Asteroid 7 */
        auto entity = tmt::engine.ecs.create_entity("Asteroid");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_ass7;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.resource = voxel_volume_ass7;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
        transform.set_world_position(glm::vec3(54.0f, -48.0f, 75.0f));
        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    }
}

void Game::on_update(const tmt::FrameData& time) {
    time_passed += time.delta_time;

    cooldown -= time.delta_time;

    if (tmt::engine.input.is_mouse_button_pressed(tmt::MouseButton::LEFT) && cooldown <= 0.0f) {
        auto& cam_transform = tmt::engine.ecs.get_component<tmt::Transform>(cam);
        const glm::vec3 cam_pos = cam_transform.get_world_position();
        const glm::vec3 cam_forward = cam_transform.get_forward();

        for (int x = -2; x <= 2; x++) {
            for (int y = -2; y <= 2; y++) {
                auto entity = tmt::engine.ecs.create_entity("Crate");
                auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
                auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
                renderer.resource = voxel_volume_cube;
                auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
                vb.resource = voxel_volume_cube;
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

void Game::on_end() {
    /* Gets rid of the resource */
    voxel_volume_cube.resource.reset();
}
