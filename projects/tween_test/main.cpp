#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/emitter.hpp"
#include "engine/core/components/ui_component.hpp"
#include "engine/core/components/image_renderer.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/systems/camera/camera_system.hpp"

#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/renderer/voxel_object.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"
#include "engine/tools/player_data.hpp"

#include "engine/tools/tweening.hpp"
#include "engine/tools/tweening/transform.hpp"
#include "engine/tools/prefab_helper.hpp"

class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class DragonScene : public tmt::Scene<DragonScene> {
   public:
    static constexpr std::string_view scene_name() { return "DragonScene"; }

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;

    tmt::Entity voxel {};
    tmt::Entity emitter1 {};
    tmt::Entity emitter2 {};
    tmt::Entity ui_entity {};
    float elapsed_time = 0.0f;
};

class Tweener : public tmt::GameComponent<Tweener> {
    // Inherited via GameComponent
   public:
    using GameComponent::GameComponent;

    static std::string_view name() { return "Tweener"; }

    void start() override;
    void update(const tmt::FrameData& time) override {}
    void end() override {}

    glm::vec3 move_offset { 0.0f, 0.0f, 10.0f };
    Tweening::TypedTween<tmt::Entity, tmt::Transform> my_move_transform;
    glm::vec3 rotate_offset { 0.0f, glm::radians(180.0f), 0.0f };
    Tweening::TypedTween<tmt::Entity, tmt::Transform> my_rotate_transform;
    glm::vec3 scale_offset { 2.0f };
    Tweening::TypedTween<tmt::Entity, tmt::Transform> my_scale_transform;
};
TMT_OBJECT(Tweener, (move_offset, my_move_transform, rotate_offset, my_rotate_transform, scale_offset, my_scale_transform));

std::unique_ptr<tmt::Application> create_application(const tmt::CommandLineArgs& args) {
    // clang-format off
    tmt::ApplicationSpecs specs {
        .name = "Example Game",
        .organization = "Thermite",
        .command_args = args,
        .log_file = "example_game_logs.txt"
    };
    // clang-format on

    /* Register Systems */
    tmt::engine.ecs.systems.add<tmt::CameraSystem>();

    /* Register Scenes */
    tmt::engine.scenes.register_scene<DragonScene>();

    /* Register Game Components */
    tmt::engine.component_registry.register_component<Tweener>();

    return std::make_unique<Game>(specs);
}

/* Dragon Scene */
void DragonScene::on_start() {
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);

        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        transform.set_world_position(glm::vec3(0.0f, 5.0f, -20.0f));
    }
}

void DragonScene::on_update(const tmt::FrameData& /*time*/) {}

void DragonScene::on_end() {}

void Tweener::start() {
    static int move_ease_offset = 0;
    Tweening::tween(my_move_transform)  //
        .owner(entity)
        .ease(static_cast<Tweening::Ease>(move_ease_offset % static_cast<int>(Tweening::Ease::IN_OUT_BOUNCE)))
        .move()
        .offset(move_offset);
    move_ease_offset++;
    static int rotate_ease_offset = 0;
    Tweening::tween(my_rotate_transform)  //
        .owner(entity)
        .ease(static_cast<Tweening::Ease>(rotate_ease_offset % static_cast<int>(Tweening::Ease::IN_OUT_BOUNCE)))
        .rotate()
        .offset(rotate_offset);
    rotate_ease_offset++;
    static int scale_ease_offset = 0;
    Tweening::tween(my_scale_transform)  //
        .owner(entity)
        .ease(static_cast<Tweening::Ease>(scale_ease_offset % static_cast<int>(Tweening::Ease::IN_OUT_BOUNCE)))
        .scale()
        .offset(scale_offset);
    scale_ease_offset++;

    // my_tween = Tweening::tween(my_float)  //
    //                .from(0.0f)
    //                .to(1.0f)
    //                .duration(2.0f)
    //                .ease(Tweening::Ease::IN_OUT_SINE)
    //                .repeat(-1)
    //                .ping_pong()
    //                .on_update([](float alpha, float* value) {
    //                    //
    //                    tmt::Log::info("Tween alpha: {}, value: {}", alpha, *value);
    //                });

    // my_tween->duration(5.0f);
    // my_tween->start();
    // my_tween->stop();

    //{
    //    /* Move */
    //    tmt::Entity new_entity = tmt::PrefabHelper::instantiate_prefab(prefab);
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(new_entity);
    //    transform.set_world_position(-depth_offset);
    //    Tweening::tween<tmt::Transform>(new_entity)  //
    //        .start_delay(1.5f)
    //        .duration(duration)
    //        .ease(ease)
    //        .repeat(repeat_count)
    //        .ping_pong()
    //        .move()
    //        .offset(glm::vec3(0.0f, 0.0f, 10.0f));
    //}
    //{
    //    /* Scale */
    //    tmt::Entity new_entity = tmt::PrefabHelper::instantiate_prefab(prefab);
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(new_entity);
    //    transform.set_world_position({ 0.0f, 0.0f, 0.0f });
    //    Tweening::tween<tmt::Transform>(new_entity)  //
    //        .start_delay(1.5f)
    //        .duration(duration)
    //        .ease(ease)
    //        .repeat(repeat_count)
    //        .ping_pong()
    //        .scale()
    //        .from(glm::vec3(1.0f, 1.0f, 1.0f))
    //        .to(glm::vec3(2.0f, 2.0f, 2.0f));
    //}
    //{
    //    /* Rotate */
    //    tmt::Entity new_entity = tmt::PrefabHelper::instantiate_prefab(prefab);
    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(new_entity);
    //    transform.set_world_position(depth_offset);
    //    Tweening::tween<tmt::Transform>(new_entity)  //
    //        .start_delay(1.5f)
    //        .duration(duration)
    //        .ease(ease)
    //        .repeat(repeat_count)
    //        .ping_pong()
    //        .rotate()
    //        .offset(glm::quat(glm::vec3(0.0f, glm::radians(180.0f), 0.0f)));
    //}

    // Tweening::tween<tmt::Transform>(entity)  //
    //     .duration(duration)
    //     .ease(ease)
    //     .repeat(repeat_count)
    //     .ping_pong()
    //     .start_delay(1.0f)
    //     .reverse_delay(1.0f)
    //     .repeat_delay(1.0f)
    //     .end_delay(1.0f)
    //     .on_pre_start([]() { tmt::Log::info("Tween pre-start"); })
    //     .on_start([]() { tmt::Log::info("Tween started"); })
    //     .on_cycle_begin([]() { tmt::Log::info("Tween cycle begun"); })
    //     .on_forward_end([]() { tmt::Log::info("Tween forward ended"); })
    //     .on_reverse_begin([]() { tmt::Log::info("Tween reverse begun"); })
    //     .on_reverse_end([]() { tmt::Log::info("Tween reverse ended"); })
    //     .on_cycle_end([]() { tmt::Log::info("Tween cycle ended"); })
    //     .on_repeat([]() { tmt::Log::info("Tween repeated"); })
    //     .on_complete([]() { tmt::Log::info("Tween completed"); })
    //     .scale()
    //     .offset(offset)
    //     .on_update([](float alpha, const tmt::Transform& transform) { /* Do something */ });

    /* Loop through Tweening::ease */
    // for (int i = 0; i <= static_cast<int>(Tweening::Ease::IN_OUT_BOUNCE); ++i) {
    //     const auto ease = static_cast<Tweening::Ease>(i);

    //    tmt::Entity new_entity = tmt::PrefabHelper::instantiate_prefab(prefab);
    //    if (new_entity == entt::null) {
    //        tmt::Log::error("Failed to instantiate prefab for Tweener component on entity {}. Skipping tween creation.", entity);
    //        continue;
    //    }

    //    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(new_entity);
    //    transform.set_world_position(glm::vec3(0.0f, 0.0f, depth_offset * i));

    //    Tweening::tween<tmt::Transform>(new_entity)  //
    //        .duration(duration)
    //        .ease(ease)
    //        .repeat(repeat_count)
    //        .ping_pong()
    //        .move()
    //        .offset(offset);
    //}
}

template <typename A, typename B>
struct MyObject {
    A a;
    B b;
};

VISITABLE_TEMPLATE_STRUCT((typename A, typename B), MyObject, (A, B), a, b);