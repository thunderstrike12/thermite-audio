#pragma once
#include "collision_shapes.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/shared/aabb.hpp"
namespace game {

class CollisionTrigger : public tmt::GameComponent<CollisionTrigger> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "CollisionTrigger"; }

    void start() override;
    void update(const tmt::FrameData& time) override {};
    void fixed_update(const tmt::FrameData& time) override;
    void end() override;
    game::AABB get_aabb_world() const;
    void draw_debug_lines() const override;

    // TODO callable from debug lines
    AABBShape collision_shape;
    glm::vec4 color { 1.0f };
    float line_width = 1.0f;

   private:
    void on_transform_updated(entt::registry& reg, entt::entity e);
};

}  // namespace game
TMT_OBJECT(game::CollisionTrigger, (collision_shape, color, line_width));
