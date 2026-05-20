#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "projects/game/data_headers/layer_mask.hpp"

#include "engine/tools/types/bezier_curve.hpp"
namespace game {

struct ExplosionParameters {
    float radius = 1.0f;
    float explosion_power = 1.0f;
    // we could use a curve for more control over how stuff explodes based on distance, linear will do for now
    tmt::BezierCurve distance_strength_curve {};
    LayerMask mask {};
};
class Explosion : public tmt::GameComponent<Explosion> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Explosion"; }

    void explode() const;
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override {};
    void draw_debug_lines() const override;

    // this would be private, but I still want it in the inspector
    ExplosionParameters param;

    tmt::Entity explosion = entt::null;

    float explosion_lifetime = 1.f;

   private:
    glm::vec3 get_position() const;

    float explode_time = 0.f;
};

}  // namespace game
TMT_OBJECT(game::ExplosionParameters, (radius, explosion_power, distance_strength_curve, mask));
TMT_GAME_COMPONENT(game::Explosion, (param, explosion, explosion_lifetime));
