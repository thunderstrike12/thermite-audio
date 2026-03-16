#include "ore_collector.hpp"

#include "projects/game/components/development_tools/collision_trigger.hpp"
#include "engine/core/polyline.hpp"
void game::OreCollector::start() {
    tmt::engine.ecs.get_dispatcher().sink<TriggerCollisionEvent>().connect<&OreCollector::on_collision_trigger>(this);
}
void game::OreCollector::update(const tmt::FrameData& time) {}

void game::OreCollector::end() {
    tmt::engine.ecs.get_dispatcher().sink<TriggerCollisionEvent>().disconnect<&OreCollector::on_collision_trigger>(this);
}

void game::OreCollector::on_collision_trigger(const TriggerCollisionEvent& trigger) {
    // TODO do something smarter maybe, check type of ore etc.
    if (trigger.trigger != entity) {
        return;
    }
    if (!tmt::engine.ecs.valid(trigger.other_object)) {
        return;
    }
    tmt::engine.ecs.destroy_entity(trigger.other_object);

    tmt::Log::info("Ore count is {}", ++ore_count);
}
