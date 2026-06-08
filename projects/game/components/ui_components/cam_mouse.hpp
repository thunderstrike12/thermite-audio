#pragma once
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class CameraMouse : public tmt::GameComponent<CameraMouse> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "CameraMouse"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    float distance = 10.0f;
    float sway_radius_x = 0.3f;
    float sway_radius_y = 0.165f;
    float smoothing = 6.0f;

    bool invert_x = true;
    bool invert_y = false;

   private:
    glm::vec3 base_position = glm::vec3(0.0f);
    glm::vec3 initial_look_at_position = glm::vec3(0.0f);
    glm::vec3 base_right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 base_up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec2 current_offset = glm::vec2(0.0f);
};

}  // namespace game

TMT_GAME_COMPONENT(game::CameraMouse, (distance, sway_radius_x, sway_radius_y, smoothing, invert_x, invert_y));