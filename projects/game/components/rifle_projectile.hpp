#pragma once
#include "events.hpp"
#include "engine/shared/ray.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"

namespace game {

class RifleProjectile : public tmt::GameComponent<RifleProjectile> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "RifleProjectile"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void draw_debug_lines() const override;
    /// <summary>
    /// This needs to be set when the component is created.
    /// </summary>
    /// <param name="dir">Should be normalized</param>
    void set_direction(const glm::vec3& dir) { direction = dir; }

    tmt::ResourceRef<tmt::Stencil> stencil;
    float movement_speed = 1.0f;

   private:
    void collide(const tmt::Hit& hit) const;
    float last_step_length = 0.0f;
    glm::vec3 previous_position { 0.0f };
    // normalized
    glm::vec3 direction { 0.0f };
};

}  // namespace game
TMT_OBJECT(game::RifleProjectile, (stencil, movement_speed));
