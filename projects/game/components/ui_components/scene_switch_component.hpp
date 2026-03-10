#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/camera.hpp"
#include "../../data_headers/scene_list.hpp"

namespace game {

enum class TargetSceneEnum : uint8_t { MAIN_MENU = 0u, MAIN_GAME = 1u, ZOO = 2u, GYM = 3u, DANIEL_TEST = 4u, MIKA_TEST = 5u };

class SceneSwitchComponent : public tmt::GameComponent<SceneSwitchComponent> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Scene Switch Component (Button)"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    TargetSceneEnum target_scene = TargetSceneEnum::MAIN_MENU;

    void switch_scene();

   private:
};

}  // namespace game

TMT_OBJECT(game::SceneSwitchComponent, (target_scene));
