#pragma once
#include "projects/game/data_headers/collision_shapes.hpp"
#include "debug_line_helper.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/shared/aabb.hpp"
#include "projects/game/data_headers/layer_mask.hpp"

#include "engine/tools/tweening.hpp"
#include "engine/tools/tweening/transform.hpp"

namespace game {

class CollisionTrigger : public tmt::GameComponent<CollisionTrigger> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "CollisionTrigger"; }

    void start() override;
    void update(const tmt::FrameData& time) override {}
    void fixed_update(const tmt::FrameData& time) override;
    void end() override;
    game::AABB get_aabb_world() const;
    void draw_debug_lines() const override;

    AABBShape collision_shape;
    DebugLineConfig line_config;
    LayerMask layer_mask;

   private:
};

}  // namespace game
TMT_OBJECT(game::CollisionTrigger, (collision_shape, line_config, layer_mask));
