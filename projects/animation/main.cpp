#include "engine/entry_point.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/animation/animation_system.hpp"
#include "engine/systems/camera/camera_system.hpp"
#include "engine/core/scene.hpp"
#include "engine/core/scenes.hpp"
#include "engine/systems/animation/rig_renderer.hpp"
#include "engine/systems/animation/components/constraints.hpp"
#include "engine/tools/test_motion.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/animation/components/bone_hierarchy_renderer.hpp"
#include "engine/systems/animation/constraints/two_bone_ik.hpp"



class Game : public tmt::Application {
   public:
    Game(const tmt::ApplicationSpecs& specs) : Application(specs) {}

    tmt::Entity voxel {};
    float time_passed = 0.0f;

    void on_start() override {};
    void on_update(const tmt::FrameData& time) override {};
    void on_end() override {};
};

class AnimationScene : public tmt::Scene<AnimationScene> {
   public:
    static constexpr std::string_view scene_name() { return "AnimationScene"; }

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;

    tmt::Entity parent_entity;
    tmt::Entity snake_entity;
    tmt::Entity walker_entity;

    tmt::Entity arm_entities[3];
    tmt::AnimConstraints::TwoBoneIK::EffectorWalkCycle walk_cycles[3];
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
    tmt::engine.scenes.register_scene<AnimationScene>();

    return std::make_unique<Game>(specs);
}

void AnimationScene::on_start() {
    using namespace tmt;
    { /* Camera entity */
        tmt::Entity entity = tmt::engine.ecs.create_entity("Camera");
        auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        auto& camera = tmt::engine.ecs.add_component<tmt::Camera>(entity);
        transform.set_world_position(glm::vec3(0.0f, 0.0f, -16.0f));
    }

    //{ /* Animation entity */
    //    auto rig_ent = tmt::engine.ecs.create_entity();
    //    auto& rig_ent_rig_model_comp = tmt::engine.ecs.add_component<tmt::RigModel>(rig_ent);

        //IO::FileLocation location { IO::Location::PROJECT, "Victory_animation.fbx" };
        //rig_ent_rig_model_comp.init(location, rig_ent);
        //rig_ent_rig_model_comp.data->animation_files.push_back(location);
      //  rig_ent_rig_model_comp.data->reload();

       // rig_ent_rig_model_comp.set_current_animation("Victory_animation");
        //rig_ent_rig_model_comp.state = RigModel::State::ANIMATE_LOOP;
       // rig_ent_rig_model_comp.time = 0.0f;
    


    { /* Test rig */
        parent_entity = engine.ecs.create_entity();
        engine.ecs.add_component<AnimConstraints::DampedTransformConstraint>(parent_entity).damp = 0.95f;
        auto& bone_renderer = engine.ecs.add_component<BoneHierarchyRenderer>(parent_entity);
        bone_renderer.color = glm::vec4(1.f, 0.f, 1.f, 1.f);
        bone_renderer.line_width = 1.f;
        auto& parent_transform = engine.ecs.get_component<Transform>(parent_entity);
        parent_transform.set_local_position(glm::vec3(5.f, 0.f, 0.f));

        auto child_0 = engine.ecs.create_entity();
        auto& child_0_transform = engine.ecs.get_component<Transform>(child_0);
        child_0_transform.set_local_position(glm::vec3(5.f, 2.f, -1.f));

        auto child_1 = engine.ecs.create_entity();
        auto& child_1_transform = engine.ecs.get_component<Transform>(child_1);
        child_1_transform.set_local_position(glm::vec3(5.f, 4.f, -4.f));

         auto child_2 = engine.ecs.create_entity();
        auto& child_2_transform = engine.ecs.get_component<Transform>(child_2);
        child_2_transform.set_local_position(glm::vec3(5.f, 5.f, -7.f));

         auto child_3 = engine.ecs.create_entity();
        auto& child_3_transform = engine.ecs.get_component<Transform>(child_3);
        child_3_transform.set_local_position(glm::vec3(5.f, 6.f, -10.0f));

         auto child_4 = engine.ecs.create_entity();
        auto& child_4_transform = engine.ecs.get_component<Transform>(child_4);
        child_4_transform.set_local_position(glm::vec3(5.f, 6.5f, -12.f));

        parent_transform.add_child(child_0);
        child_0_transform.add_child(child_1);
        child_1_transform.add_child(child_2);
        child_2_transform.add_child(child_3);
        child_3_transform.add_child(child_4);
    }
    { /* snake rig */
        snake_entity = engine.ecs.create_entity();
        engine.ecs.add_component<AnimConstraints::DampedTransformConstraint>(snake_entity).damp = 0.95f;
        auto& bone_renderer = engine.ecs.add_component<BoneHierarchyRenderer>(snake_entity);
        bone_renderer.color = glm::vec4(1.f, 1.f, 0.f, 1.f);
        bone_renderer.line_width = 1.f;
        auto& parent_transform = engine.ecs.get_component<Transform>(snake_entity);
        parent_transform.set_local_position(glm::vec3(0.f, -3.f, -0.f));

        auto child_0 = engine.ecs.create_entity();
        auto& child_0_transform = engine.ecs.get_component<Transform>(child_0);
        child_0_transform.set_local_position(glm::vec3(-2.f, -3.f, -0.f));

        auto child_1 = engine.ecs.create_entity();
        auto& child_1_transform = engine.ecs.get_component<Transform>(child_1);
        child_1_transform.set_local_position(glm::vec3(-4.f, -3.f, -0.f));

        auto child_2 = engine.ecs.create_entity();
        auto& child_2_transform = engine.ecs.get_component<Transform>(child_2);
        child_2_transform.set_local_position(glm::vec3(-6.f, -3.f, -0.f));

        auto child_3 = engine.ecs.create_entity();
        auto& child_3_transform = engine.ecs.get_component<Transform>(child_3);
        child_3_transform.set_local_position(glm::vec3(-8.f, -3.f, -0.0f));

        auto child_4 = engine.ecs.create_entity();
        auto& child_4_transform = engine.ecs.get_component<Transform>(child_4);
        child_4_transform.set_local_position(glm::vec3(-10.f, -3.f, -0.f));

        parent_transform.add_child(child_0);
        child_0_transform.add_child(child_1);
        child_1_transform.add_child(child_2);
        child_2_transform.add_child(child_3);
        child_3_transform.add_child(child_4);
    }
    {
        //walker_entity = engine.ecs.create_entity("walker");
        //auto& bone_renderer = engine.ecs.add_component<BoneHierarchyRenderer>(walker_entity);
        //for(int i = 0; i < 3; i++)
        //{
        //    walk_cycles[i].step_time = 0.8f;
        //    walk_cycles[i].step_height = 0.5f;
        //    walk_cycles[i].step_duration = 0.5f;
        //    walk_cycles[i].step_prediction_strength = 5.f;
        //    walk_cycles[i].set_offset(i * (0.8f/3.f));

        //    float rad_ang = (i+1) * 2.f/3.f * glm::pi<float>(); 

        //    glm::quat rot = glm::angleAxis(rad_ang, glm::vec3(0.f, 1.f, 0.f));
        //    glm::vec3 p = glm::normalize(rot * glm::vec3(0.f, 0.f, 1.f));



        //    arm_entities[i] = engine.ecs.create_entity();
        //    auto& two_bone_ik_constraint = engine.ecs.add_component<AnimConstraints::TwoBoneIKConstraint>(arm_entities[i]);
        //    auto& walk_cycle = engine.ecs.add_component<AnimConstraints::EffectorWalkCycle>(arm_entities[i]);
        //    bone_renderer.color = glm::vec4(0.f, 1.f, 1.f, 1.f);
        //    bone_renderer.line_width = 0.4f;
        //    auto& transform = engine.ecs.get_component<Transform>(arm_entities[i]);
        //    transform.set_local_position(p);

        //    auto child_0 = engine.ecs.create_entity();
        //    auto& child_0_transform = engine.ecs.get_component<Transform>(child_0);
        //    child_0_transform.set_local_position(p * 2.f);

        //    auto child_1 = engine.ecs.create_entity();
        //    auto& child_1_transform = engine.ecs.get_component<Transform>(child_1);
        //    child_1_transform.set_local_position(p * 3.f);

        //    auto bend_entity = engine.ecs.create_entity();
        //    engine.ecs.add_component<NoBone>(bend_entity);
        //    auto& bend_transform = engine.ecs.get_component<Transform>(bend_entity);
        //    bend_transform.set_parent(walker_entity);
        //    bend_transform.set_world_position(p * 2.f + glm::vec3(0.f, 100.f, 0.f));

        //    /*two_bone_ik_constraint.parent = walker_entity;
        //    two_bone_ik_constraint.root = arm_entities[i];*/
        //    two_bone_ik_constraint.mid = child_0;
        //    two_bone_ik_constraint.tip = child_1;
        //    two_bone_ik_constraint.bend_position_entity = bend_entity;
        //    
        //    transform.set_parent(walker_entity);
        //    transform.add_child(child_0);
        //    child_0_transform.add_child(child_1);      
        //}
        
    }
}

void AnimationScene::on_update(const tmt::FrameData& time) 
{
    using namespace tmt;
    static float x = 0.f;
    x-=time.delta_time*2.f;

    auto& parent_transform = engine.ecs.get_component<Transform>(walker_entity);
    //for (size_t i = 0; i < 3; i++) {

    //    float rad_ang = (i+1) * 2.f/3.f * glm::pi<float>(); 

    //    glm::quat rot = glm::angleAxis(rad_ang, glm::vec3(0.f, 1.f, 0.f));
    //    glm::vec3 p = glm::normalize(rot * glm::vec3(0.f, 0.f, 1.f));

    //    auto& two_bone_ik = engine.ecs.get_component<tmt::AnimConstraints::TwoBoneIKConstraint>(arm_entities[i]);

    //    tmt::AnimConstraints::TwoBoneIK::EffectorWalkCycle::WalkCycleUpdateVariables anim_vars;
    //    glm::vec3 target = parent_transform.get_world_matrix() * glm::vec4(p*3.f, 1.f);
    //    anim_vars.continuous_available_pos = glm::vec3(target.x, 0.f, target.z);
    //    anim_vars.grounded = true;
    //    anim_vars.up = glm::vec3(0.f, 1.f, 0.f);

    //    two_bone_ik.target_position_entity = snake_entity;//walk_cycles[i].do_walk_cycle(time.delta_time, anim_vars);
    //    
    //}

    //auto& snake_transform = engine.ecs.get_component<Transform>(snake_entity);
    //snake_transform.set_local_position(TestMotion::sample_position(time.elapsed_time));
    //snake_transform.set_local_rotation(TestMotion::sample_rotation(time.elapsed_time, time.delta_time));

    //auto& two_bone_ik = engine.ecs.get_component<AnimConstraints::TwoBoneIKConstraint>(arm_entity);
    //two_bone_ik.target_position = TestMotion::sample_position(time.elapsed_time + 100.f);
}

void AnimationScene::on_end() {}
