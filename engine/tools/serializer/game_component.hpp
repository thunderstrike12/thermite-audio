#pragma once
#include <JsonReflect.hpp>
#include <nlohmann/json.hpp>
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/component_collection.hpp"
#include "engine/systems/gameplay/game_component_registry.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/core/logger.hpp"

template <typename... Args>
JsonReflect::json tag_invoke(JsonReflect::serialize_lib_t, const tmt::IGameComponent& value, Args&&... args);

template <typename... Args>
void tag_invoke(JsonReflect::deserialize_lib_t, const JsonReflect::json& j, tmt::IGameComponent& value, Args&&... args);

template <typename... Args>
inline JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::ComponentCollection& collection, Args&&... args) {
    JsonReflect::json result;
    for (const auto& [type_id, component] : collection.get_all_components()) {
        const auto& component_info = tmt::engine.component_registry.get_component_info(type_id);
        result[component_info.name] = tmt::Serializer::serialize(*component, std::forward<Args>(args)...);
    }
    return result;
}

template <typename... Args>
inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::ComponentCollection& collection, Args&&... args) {
    const tmt::Entity owner = tmt::engine.ecs.get_entity(collection);
    for (const auto& [key, value] : j.items()) {
        const bool is_registered = tmt::engine.component_registry.is_component_registered(key);
        if (is_registered == false) {
            tmt::Log::warn(tmt::Log::Scope::ENGINE, "[ComponentCollection] deserialize: Component with name '{}' is not registered in the GameComponentRegistry. Skipping deserialization of this component.", key);
            continue;
        }

        const auto type_id = tmt::engine.component_registry.get_component_index(key);

        if (collection.has_component(type_id) == false) {
            collection.add_component(type_id, owner);
        }

        tmt::IGameComponent& component = collection.get_component(type_id);
        tmt::Serializer::deserialize(value, component, std::forward<Args>(args)...);
    }
}