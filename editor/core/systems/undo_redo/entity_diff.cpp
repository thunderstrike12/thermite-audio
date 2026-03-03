#include "entity_diff.hpp"

#include <engine/engine.hpp>
#include <engine/tools/serializer/ecs.hpp>

namespace tmt {

EntityDiff::EntityDiff(const Entity entity, const bool is_creation) : is_creation { is_creation } {
    entity_ids.insert(entity);
    entity_json = Serializer::serialize(entity, engine.ecs);
}

EntityDiff::EntityDiff(const std::set<Entity>& entities, const bool is_creation) : is_creation { is_creation } {
    entity_ids = entities;
    entity_json = Serializer::serialize(entities, engine.ecs);
}

void EntityDiff::undo() {
    if (is_creation) {
        destroy_entities();
    } else {
        recreate_entities();
    }
}

void EntityDiff::redo() {
    if (is_creation) {
        recreate_entities();
    } else {
        destroy_entities();
    }
}

void EntityDiff::destroy_entities() {
    for (const Entity entity : entity_ids) {
        if (engine.ecs.valid(entity)) {
            engine.ecs.destroy_entity(entity);
        }
    }
}

void EntityDiff::recreate_entities() {
    std::set<Entity> new_entities;
    Serializer::deserialize(entity_json, new_entities, engine.ecs);
    entity_ids = new_entities;
}

}  // namespace tmt
