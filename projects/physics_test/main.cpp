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
#include "engine/core/resources/stencil.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include <engine/systems/physics/components/destructable.hpp>
#include <engine/systems/physics/destruction_system.hpp>

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
    tmt::ResourceRef<tmt::Stencil> stencil;

    float time_passed = 0.0f;
    float cooldown = 0.0f;

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

class EntityRef : public tmt::GameComponent<EntityRef> {
   public:
    using GameComponent::GameComponent;

    tmt::Entity entity_ref = entt::null;
    std::set<tmt::Entity> entity_set {};

    static constexpr std::string_view get_name() { return "EntityRef"; }

    // Inherited via GameComponent
    void start() override {
        const auto& name = tmt::engine.ecs.get_component<tmt::Name>(entity_ref);
        tmt::Log::info("EntityRef Component started! Referenced entity name: {}", name.name);
    }
    void update(const tmt::FrameData& time) override {}
    void end() override {}
};
TMT_OBJECT(EntityRef, (entity_ref, entity_set));

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Physics Test",
        .command_args = args,
        .log_file = "physics_test_logs.txt"
    };
    // clang-format on

    /* Register Systems */
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    /* Register Scenes */
    tmt::engine.scenes.register_scene<MainScene>();

    /* Register Components */
    tmt::engine.component_registry.register_component<EntityRef>();

    return std::make_unique<Game>(specs);
}

void MainScene::on_start() {
    /* Load the throwable box model */
    const tmt::ResourceRef box_scene = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "box-10.vengi" });
    box_model = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(box_scene);

    auto voxel_file_piece = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "test_asteroid_7.vengi" });
    auto voxel_volume_piece = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_piece);

    // auto voxel_file_ass3 = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "test_asteroid_3.vengi" });
    // auto voxel_volume_ass3 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_ass3);

    // auto voxel_file_ass7 = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "test_asteroid_7.vengi" });
    // auto voxel_volume_ass7 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_ass7);

    {
        auto voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "voronoi40.vengi" });
        // volume_cube16 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);
        stencil = tmt::engine.resources.copy_resource<tmt::Stencil>(voxel_file);
    }

    //{
    //    //auto voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "cube64.vengi" });
    //    //volume_cube64 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);
    //}

    {
        auto entity = tmt::engine.ecs.create_entity();
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_piece;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        // vb.resource = voxel_volume_piece;
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;

        // transform.rotate_world(glm::vec3(45.0f, 45.0f, 45.0f));

        tmt::engine.ecs.add_component<tmt::Destructible>(entity);
    }
}

void MainScene::on_update(const tmt::FrameData& time) {
    /* Update the time elapsed and cooldown timer */
    time_passed += time.delta_time;
    cooldown -= time.delta_time;

    /* Get the mouse position */
    const glm::ivec2 mouse_pos = glm::ivec2(tmt::engine.input.get_mouse_x(), tmt::engine.input.get_mouse_y());

    /* Create a ray from the mouse position for the current render view and trace it */
    const tmt::Ray mouse_ray = tmt::engine.renderer.render_view.pixel_ray(mouse_pos);
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(mouse_ray);

    if (hit.miss() == false) {
        tmt::engine.polyline.use_color(0.95686274f, 0.60392156f, 0.21960784f);
        tmt::engine.polyline.use_line_width(2.0f);
        const float draw_radius = 1.0f * UNITS_PER_VOXEL;
        tmt::engine.polyline.draw_circle(mouse_ray.origin + mouse_ray.dir * (hit.distance - draw_radius), draw_radius);

        // if (tmt::engine.input.is_mouse_button_pressed(tmt::MouseButton::LEFT)) {
        //     tmt::engine.ecs.systems.get<tmt::Destruction>().destroy_voxel(hit.entity, hit.coord);
        // }

        if (tmt::engine.input.is_mouse_button_just_pressed(tmt::MouseButton::LEFT)) {
            tmt::engine.ecs.systems.get<tmt::Destruction>().destroy_voxels(hit.entity, stencil.resource.get(), (glm::ivec3)hit.coord - glm::ivec3(20));
        }
    }

    // Use to test the logging please
    /*  static int i = 0;
      for (int j = 0; j < 2; j++) {
          tmt::Log::warn("{}", i);
          i += j;
      }*/
    if (tmt::engine.input.is_mouse_button_pressed(tmt::MouseButton::MIDDLE) && cooldown <= 0.0f) {
        /* Get the camera position and forward direction */
        const tmt::Entity camera = tmt::Camera::get_active_camera();
        const tmt::Transform& cam_transform = tmt::engine.ecs.get_component<tmt::Transform>(camera);
        const glm::vec3 cam_pos = cam_transform.get_world_position();
        const glm::vec3 cam_forward = cam_transform.get_forward();

        /* Spawn box models with forward force */
        for (int x = 0; x <= 0; x++) {
            for (int y = 0; y <= 0; y++) {
                auto entity = tmt::engine.ecs.create_entity("Crate");
                auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
                auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
                renderer.resource = box_model;
                auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
                // vb.resource = box_model;
                vb.density = 20.0f;
                vb.gravity = 0.0f;
                vb.type = tmt::VoxelBody::DYNAMIC;
                // tmt::Physics::initialize_voxel_body(vb);

                const glm::vec3 x_offset = cam_transform.get_right() * ((float)x * 1.5f);
                const glm::vec3 y_offset = cam_transform.get_up() * ((float)y * 1.5f);

                transform.set_world_position(cam_pos + cam_forward * 2.0f + x_offset + y_offset);

                // tmt::Physics::set_position(vb, cam_pos + cam_forward * 2.0f + x_offset + y_offset);

                float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f;
                float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f;
                float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f;

                transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));

                // tmt::Physics::set_rotation(vb, cam_transform.get_world_rotation() * glm::vec3(random_rot_x, random_rot_y, random_rot_z));
                tmt::Physics::add_force(vb, cam_forward * 7.0f);
            }
        }

        cooldown = 0.10f;
    }
}

void MainScene::on_end() {}
