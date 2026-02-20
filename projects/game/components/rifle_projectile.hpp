#pragma once
#include "events.hpp"
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class RifleProjectile : public tmt::GameComponent<RifleProjectile> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "RifleProjectile"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    /// <summary>
    /// This needs to be set when the component is created.
    /// </summary>
    /// <param name="dir">Should be normalized</param>
    void set_direction(const glm::vec3& dir) { direction = dir; }

    float movement_speed = 1.0f;

   private:
    void collide();
    glm::vec3 previous_position { 0.0f };
    // normalized
    glm::vec3 direction { 0.0f };
};

}  // namespace game
TMT_OBJECT(game::RifleProjectile, (movement_speed));
