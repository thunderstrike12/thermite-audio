#include "collision_trigger.hpp"

#include "events.hpp"
#include "engine/core/polyline.hpp"
#include "engine/systems/physics/physics_system.hpp"

void game::CollisionTrigger::fixed_update(const tmt::FrameData& time) {
    // TODO update aabb in case this moves
    const auto& bvh = tmt::engine.ecs.systems.get<tmt::Physics>().get_bvh();
    // TODO mask it

    auto world_aabb = get_aabb_world();

    auto overlap_hits = bvh.overlap({ world_aabb.min_bounds, world_aabb.max_bounds });
    if (!overlap_hits.empty()) {
        auto group = tmt::engine.ecs.get_registry().group<tmt::VoxelBody>(entt::get<tmt::Transform>);
        for (uint32_t hit : overlap_hits) {
            tmt::Entity other_entity = group[hit];
            // do stuff with the entity
            // TODO for now just kill it
            tmt::engine.ecs.get_dispatcher().trigger(TriggerCollisionEvent { .trigger = entity, .other_object = other_entity });
        }
    }
}
void game::CollisionTrigger::start() {}

void game::CollisionTrigger::end() {}
game::AABB game::CollisionTrigger::get_aabb_world() const {
    return collision_shape.world_aabb(tmt::engine.ecs.get_component<tmt::Transform>(entity).get_world_position());
}
void game::CollisionTrigger::draw_debug_lines() const {
    tmt::engine.polyline.use_color(color);
    tmt::engine.polyline.use_line_width(line_width);

    auto world_abb = get_aabb_world();
    auto min_aabb = world_abb.min_bounds;
    auto max_aabb = world_abb.max_bounds;
    tmt::engine.polyline.draw_aabb(min_aabb, max_aabb);
}
