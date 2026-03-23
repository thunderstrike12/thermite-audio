#include "scene_switch_component.hpp"
#include "engine/core/components/button.hpp"
#include "engine/core/ecs.hpp"

namespace game {

void game::SceneSwitchComponent::start() {
    if (auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        tmt::Log::info("Found button component, adding switch scene to on click.");
        button_component->on_click.add(this, &SceneSwitchComponent::switch_scene);
    } else {
        tmt::Log::warn("No button found for scene switch component!");
    }
}

void game::SceneSwitchComponent::update(const tmt::FrameData& time) {}

void game::SceneSwitchComponent::end() {
    if (auto button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) {
        button_component->on_click.clear();
    }
}

// Cannot be made const because its being used in the button callback and on_click.add doesnt take const functions
void SceneSwitchComponent::switch_scene() {
    switch (target_scene) {
        case TargetSceneEnum::MAIN_MENU:
            tmt::engine.scenes.enqueue_scene<MainMenuScene>();
            break;
        case TargetSceneEnum::MAIN_GAME:
            tmt::engine.scenes.enqueue_scene<MainGameScene>();
            break;
        case TargetSceneEnum::GYM:
            tmt::engine.scenes.enqueue_scene<Gym>();
            break;
        case TargetSceneEnum::ZOO:
            tmt::engine.scenes.enqueue_scene<Zoo>();
            break;
        case TargetSceneEnum::DANIEL_TEST:
            tmt::engine.scenes.enqueue_scene<DanielTestScene>();
            break;
        case TargetSceneEnum::MIKA_TEST:
            tmt::engine.scenes.enqueue_scene<MikaTestScene>();
            break;
        case TargetSceneEnum::HUB:
            tmt::engine.scenes.enqueue_scene<HubScene>();
            break;
        case TargetSceneEnum::WEAPON_MOTION_TEST:
            tmt::engine.scenes.enqueue_scene<WeaponMotionTest>();
        default:
            tmt::engine.scenes.enqueue_scene<MainMenuScene>();
            break;
    }
}

}  // namespace game