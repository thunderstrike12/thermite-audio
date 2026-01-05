#pragma once
#include <nlohmann/json.hpp>
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/systems/gameplay/game_component_registry.hpp"
#include "engine/core/components/component_collection.hpp"

static inline JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::IGameComponent& value) {
    //
    return value.serialize();
}
static inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::IGameComponent& value) {
    //
    value.deserialize(j);
}

static inline JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::ComponentCollection& collection) {
    JsonReflect::json result;
    for (const auto& [type_id, component] : collection.get_all_components()) {
        const auto& component_info = tmt::engine.component_registry.get_component_info(type_id);
        result[component_info.name] = JsonReflect::to_json(component);
    }
    return result;
}

static inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::ComponentCollection& collection) {
    const tmt::Entity owner = tmt::engine.ecs.get_entity(collection);
    for (const auto& [key, value] : j.items()) {
        const auto type_id = tmt::engine.component_registry.get_component_index(key);

        if (collection.has_component(type_id) == false) {
            collection.add_component(type_id, owner);
        }

        tmt::IGameComponent& component = collection.get_component(type_id);
        JsonReflect::from_json(value, component);
    }
}