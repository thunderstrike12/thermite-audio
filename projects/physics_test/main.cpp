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
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/core/polyline.hpp"
#include <engine/systems/physics/destruction_system.hpp>

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    std::vector<tmt::Entity> voxels = {};
    tmt::Entity cam;
    int num = 0;
    float time_passed = 0.0f;
    float cooldown = 0.0f;

    tmt::ResourceRef<tmt::VoxelVolume> voxel_volume_cube;
    tmt::ResourceRef<tmt::VoxelVolume> volume_cube16;
    tmt::ResourceRef<tmt::VoxelVolume> volume_cube64;
    tmt::ResourceRef<tmt::Stencil> stencil;

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
};

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Physics Test",
        .command_args = args,
        .log_file = "physics_test_logs.txt"
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

    auto voxel_file_cube = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "box-10.vengi" });
    voxel_volume_cube = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_cube);

    auto voxel_file_piece = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "cube64.vengi" });
    auto voxel_volume_piece = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_piece);

    auto voxel_file_ass3 = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "test_asteroid_3.vengi" });
    auto voxel_volume_ass3 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_ass3);

    auto voxel_file_ass7 = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "test_asteroid_7.vengi" });
    auto voxel_volume_ass7 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file_ass7);

    {
        auto voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "cube4.vengi" });
        volume_cube16 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);
        stencil = tmt::engine.resources.copy_resource<tmt::Stencil>(voxel_file);
    }

    {
        auto voxel_file = tmt::engine.resources.load_resource<tmt::VoxelScene>({ tmt::IO::Location::PROJECT, "cube64.vengi" });
        volume_cube64 = tmt::engine.resources.copy_resource<tmt::VoxelVolume>(voxel_file);
    }

    {
        auto entity = tmt::engine.ecs.create_entity();
        auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        renderer.resource = voxel_volume_piece;
        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        vb.gravity = 0.0f;
        vb.type = tmt::VoxelBody::STATIC;
    }

    // for (size_t z = 0; z < 5; z++) {
    //     for (size_t i = 0; i < 0; i++) { /* Voxel Physics Entity */
    //         auto entity = tmt::engine.ecs.create_entity();
    //         auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);
    //         auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //         renderer.resource = voxel_volume_piece;

    //        auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //        vb.resource = voxel_volume_piece;

    //        vb.gravity = 0.0f;
    //        vb.type = tmt::VoxelBody::DYNAMIC;
    //        float random_x = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 150.0f;
    //        float random_y = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 150.0f;
    //        transform.set_world_position(glm::vec3(random_x, random_y, (5 - z) * 5.0f));

    //        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //        transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    //        voxels.push_back(entity);
    //    }
    //}

    //{ /* Voxel Physics Entity, Asteroid 3 */
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    renderer.resource = voxel_volume_ass3;
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    vb.resource = voxel_volume_ass3;
    //    vb.gravity = 0.0f;
    //    vb.type = tmt::VoxelBody::DYNAMIC;
    //    transform.set_world_position(glm::vec3(0.0f, 50.0f, 110.0f));

    //    float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    //}

    //{ /* Voxel Physics Entity, Asteroid 3 */
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    renderer.resource = voxel_volume_ass3;
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    vb.resource = voxel_volume_ass3;
    //    vb.gravity = 0.0f;
    //    vb.type = tmt::VoxelBody::DYNAMIC;
    //    transform.set_world_position(glm::vec3(40.0f, 20.0f, 100.0f));
    //    float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));

    //    //{
    //    //    auto e = tmt::engine.ecs.create_entity();
    //    //    auto& transform_e = tmt::engine.ecs.get_component<tmt::Transform>(e);
    //    //    auto& renderer_e = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(e);
    //    //    renderer_e.resource = voxel_volume_piece;
    //    //    auto& vb_e = tmt::engine.ecs.add_component<tmt::VoxelBody>(e);
    //    //    vb_e.resource = voxel_volume_piece;
    //    //    vb_e.gravity = 3.0f;
    //    //    vb_e.type = tmt::VoxelBody::DYNAMIC;
    //    //    transform_e.set_world_position(glm::vec3(45.0f, 50.0f, 100.0f));
    //    //    transform_e.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    //    //}
    //}

    //{ /* Voxel Physics Entity, Asteroid 3 */
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    renderer.resource = voxel_volume_ass3;
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    vb.resource = voxel_volume_ass3;
    //    vb.gravity = 0.0f;
    //    vb.type = tmt::VoxelBody::STATIC;
    //    transform.set_world_position(glm::vec3(-24.0f, -20.0f, 90.0f));
    //    float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    //}

    //{ /* Voxel Physics Entity, Asteroid 7 (close to test) */
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    renderer.resource = voxel_volume_ass7;
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    vb.resource = voxel_volume_ass7;
    //    vb.gravity = 0.0f;
    //    vb.type = tmt::VoxelBody::STATIC;
    //    transform.set_world_position(glm::vec3(50.0f, 31.6f, 100.0f));
    //    float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    //}

    //{ /* Voxel Physics Entity, Asteroid 7 */
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    renderer.resource = voxel_volume_ass7;
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    vb.resource = voxel_volume_ass7;
    //    vb.gravity = 0.0f;
    //    vb.type = tmt::VoxelBody::STATIC;
    //    transform.set_world_position(glm::vec3(20.0f, -35.0f, 120.0f));
    //    float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    //}

    //{ /* Voxel Physics Entity, Asteroid 7 */
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    renderer.resource = voxel_volume_ass7;
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    vb.resource = voxel_volume_ass7;
    //    vb.gravity = 0.0f;
    //    vb.type = tmt::VoxelBody::STATIC;
    //    transform.set_world_position(glm::vec3(-20.0f, 55.0f, 100.0f));
    //    float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    //}

    //{ /* Voxel Physics Entity, Asteroid 7 */
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    renderer.resource = voxel_volume_ass7;
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    vb.resource = voxel_volume_ass7;
    //    vb.gravity = 0.0f;
    //    vb.type = tmt::VoxelBody::STATIC;
    //    transform.set_world_position(glm::vec3(-14.0f, 17.0f, 70.0f));
    //    float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    //}

    //{ /* Voxel Physics Entity, Asteroid 7 */
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    renderer.resource = voxel_volume_ass7;
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    vb.resource = voxel_volume_ass7;
    //    vb.gravity = 0.0f;
    //    vb.type = tmt::VoxelBody::STATIC;
    //    transform.set_world_position(glm::vec3(19.0f, 0.0f, 90.0f));
    //    float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    //}

    //{ /* Voxel Physics Entity, Asteroid 7 */
    //    auto entity = tmt::engine.ecs.create_entity();
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    //    auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
    //    renderer.resource = voxel_volume_ass7;
    //    auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
    //    vb.resource = voxel_volume_ass7;
    //    vb.gravity = 0.0f;
    //    vb.type = tmt::VoxelBody::STATIC;
    //    transform.set_world_position(glm::vec3(54.0f, -48.0f, 75.0f));
    //    // float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    // float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    // float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f * 2.0f;
    //    // transform.set_world_rotation(glm::vec3(random_rot_x, random_rot_y, random_rot_z));
    //}
}

void Game::on_update(const tmt::FrameData& time) {
    // Debug Draw Stencil
    // tmt::engine.polyline.use_color(0.3f, 0.3f, 1.0f);
    // tmt::engine.polyline.use_line_width(0.25f);
    // auto* stencil_resource = stencil.resource.get();
    // glm::vec3 half_size = glm::vec3(stencil_resource->size) * UNITS_PER_VOXEL * 0.5f;
    // for (size_t x = 0; x < stencil_resource->size.x; x++) {
    //    for (size_t y = 0; y < stencil_resource->size.y; y++) {
    //        for (size_t z = 0; z < stencil_resource->size.z; z++) {
    //            if (stencil_resource->get_voxel(x, y, z) == 0) continue;

    //            glm::vec3 min = glm::vec3(x, y, z) * UNITS_PER_VOXEL;
    //            glm::vec3 max = min + UNITS_PER_VOXEL;
    //            tmt::engine.polyline.draw_aabb(min - half_size, max - half_size);
    //        }
    //    }
    //}

    time_passed += time.delta_time;

    cooldown -= time.delta_time;

    static float tool_radius = 2.0f;
    tool_radius = fmaxf(fminf(tool_radius + tmt::engine.input.get_mouse_wheel_y() * 0.1f, 6.0f), 1.0f);
    const int radius = (int)ceilf(tool_radius);

    /* Get the mouse position */
    const glm::ivec2 mouse_pos = glm::ivec2(tmt::engine.input.get_mouse_x(), tmt::engine.input.get_mouse_y());

    /* Create a ray from the mouse position for the current render view and trace it */
    const tmt::Ray mouse_ray = tmt::engine.renderer.render_view.pixel_ray(mouse_pos);
    const tmt::Hit hit = tmt::engine.renderer.trace_ray(mouse_ray);

    if (hit.miss() == false) {
        tmt::engine.polyline.use_color(0.95686274f, 0.60392156f, 0.21960784f);
        tmt::engine.polyline.use_line_width(2.0f);
        const float draw_radius = tool_radius * UNITS_PER_VOXEL;
        tmt::engine.polyline.draw_circle(mouse_ray.origin + mouse_ray.dir * (hit.distance - draw_radius), draw_radius);

        if (tmt::engine.input.is_mouse_button_pressed(tmt::MouseButton::LEFT)) {
            tmt::engine.ecs.systems.get<tmt::Destruction>().destroy_voxels(hit.entity, stencil.resource.get(), hit.coord);

            // const float r2 = tool_radius * tool_radius;
            // for (int z = -radius; z <= radius; ++z) {
            //     for (int y = -radius; y <= radius; ++y) {
            //         for (int x = -radius; x <= radius; ++x) {
            //             const float fx = (float)x + 0.5f, fy = (float)y + 0.5f, fz = (float)z + 0.5f;
            //             const float d2 = fx * fx + fy * fy + fz * fz;
            //             if (d2 > r2) continue;
            //             resource->blas->remove_voxel((uint32_t)((int)hit.coord.x + x), (uint32_t)((int)hit.coord.y + y), (uint32_t)((int)hit.coord.z + z));
            //         }
            //     }
            // }
            // resource->set_dirty();
        }
    }

    if (tmt::engine.input.is_mouse_button_pressed(tmt::MouseButton::MIDDLE) && cooldown <= 0.0f) {
        auto& cam_transform = tmt::engine.ecs.get_component<tmt::Transform>(cam);
        const glm::vec3 cam_pos = cam_transform.get_world_position();
        const glm::vec3 cam_forward = cam_transform.get_forward();

        // for (int x = -2; x <= 2; x++) {
        //     for (int y = -2; y <= 2; y++) {
        //         auto entity = tmt::engine.ecs.create_entity();
        //         auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        //         auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
        //         renderer.resource = voxel_volume_cube;
        //         auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
        //         vb.resource = voxel_volume_cube;
        //         vb.gravity = 0.0f;
        //         vb.type = tmt::VoxelBody::DYNAMIC;
        //         tmt::Physics::initialize_voxel_body(vb);

        //        const glm::vec3 x_offset = cam_transform.get_right() * ((float)x * 1.5f);
        //        const glm::vec3 y_offset = cam_transform.get_up() * ((float)y * 1.5f);

        //        tmt::Physics::set_position(vb, cam_pos + cam_forward * 2.0f + x_offset + y_offset);

        //        float random_rot_x = ((float)(rand() % 1000) / 1000.0f) * 3.1415f;
        //        float random_rot_y = ((float)(rand() % 1000) / 1000.0f) * 3.1415f;
        //        float random_rot_z = ((float)(rand() % 1000) / 1000.0f) * 3.1415f;

        //        tmt::Physics::set_rotation(vb, cam_transform.get_world_rotation() * glm::vec3(random_rot_x, random_rot_y, random_rot_z));
        //        tmt::Physics::add_force(vb, cam_forward * 20.0f);
        //    }
        //}

        {
            auto entity = tmt::engine.ecs.create_entity();
            auto& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(entity);
            renderer.resource = voxel_volume_cube;
            auto& vb = tmt::engine.ecs.add_component<tmt::VoxelBody>(entity);
            vb.gravity = 0.0f;
            vb.type = tmt::VoxelBody::DYNAMIC;
            tmt::Physics::initialize_voxel_body(vb, *voxel_volume_cube.resource);
            tmt::Physics::set_position(vb, cam_pos + cam_forward * 2.0f);
            tmt::Physics::set_rotation(vb, cam_transform.get_world_rotation());
            tmt::Physics::add_force(vb, cam_forward * 10.0f);
        }

        cooldown = 1.00f;
    }

    if (time_passed > 0.005f) {
        if (num >= voxels.size()) return;
        auto& vb = tmt::engine.ecs.get_component<tmt::VoxelBody>(voxels[num++]);
        float random_x = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 50.0f;
        float random_y = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 50.0f;
        tmt::Physics::add_force(vb, glm::vec3(random_x, random_y, 35.0f));
        time_passed = 0.0f;
    }

    // auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(voxel);
    // transform.set_world_position(glm::vec3(1.0f, sinf(time_passed), 1.0f));
    // transform.set_world_rotation(glm::vec3(sinf(time_passed), cosf(time_passed), 0.0f));
    // transform.set_world_scale(glm::vec3(1.0f, 1.5f + sinf(time_passed), 1.0f));
}

void Game::on_end() {}
