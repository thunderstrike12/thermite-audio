#include "ecs.hpp"
#include "engine/tools/serializer.hpp"

#include "engine/core/entity.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/reflection.hpp"

#include "engine/core/components/all.hpp"
#include "engine/core/components/transform.hpp"

#include "engine/tools/serializer/all.hpp"
#include "engine/tools/fmt/nlohmann.hpp"
#include "engine/tools/profiler.hpp"

using Json = JsonReflect::json;

struct SerializeState {
    const tmt::Ecs& ecs;
    /* Original provided entities */
    const std::set<tmt::Entity>& original_entities;
    /* Originals + children */
    const std::set<tmt::Entity>& all_entities;
};

struct DeserializeState {
    tmt::Ecs& ecs;
    const Json& original_json;

    std::unordered_map<tmt::Entity, tmt::Entity> entity_mapping {{entt::null, entt::null}}; /* Default null to null value */
};

/* Entt entity with state */
static inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, const DeserializeState& state) {
    tmt::Entity json_entity = entt::null;
    tmt::Serializer::deserialize(j, json_entity);

    if (state.entity_mapping.contains(json_entity)) {
        entity = state.entity_mapping.at(json_entity);
    } else {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serialization] Entity mapping does not contain entity {}", json_entity);
        entity = json_entity;
    }
}

/* ECS */
Json tag_invoke(JsonReflect::serialize_t, const tmt::Ecs& ecs) {
    std::set<tmt::Entity> entities;
    for (const auto entity : ecs.get_registry().view<entt::entity>()) {
        entities.insert(entity);
    }
    tmt::json result = tmt::Serializer::serialize(entities, ecs);
    tmt::json& systems = result["systems"];
    const auto size = ecs.systems.size();
    for (const auto& system : ecs.systems) {
        const auto serialized = system->serialize();
        if (serialized.empty() == false) {
            const auto name = system->get_name();
            systems[name] = serialized;
        }
    }

    return result;
}

void tag_invoke(JsonReflect::deserialize_t, const Json& j, tmt::Ecs& ecs) {
    std::set<tmt::Entity> new_entities;
    tmt::Serializer::deserialize(j, new_entities, ecs);

    if (j.contains("systems")) {
        const Json& systems = j["systems"];
        for (auto& system : ecs.systems) {
            const auto name = system->get_name();
            if (systems.contains(name)) {
                const Json& system_json = systems[name];
                system->deserialize(system_json);
            }
        }
    }
}

/* Single entity */
Json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, const tmt::Ecs& ecs) {
    const std::set<tmt::Entity> entities {entity};
    return tmt::Serializer::serialize(entities, ecs);
}

void tag_invoke(JsonReflect::deserialize_t, const Json&, tmt::Entity&, tmt::Ecs&) {}

/* [Serialize] Set of entities */
Json tag_invoke(JsonReflect::serialize_t, const std::set<tmt::Entity>& entities_original, const tmt::Ecs& ecs) {
    TMT_ZONE_SCOPED_NS("Ecs::serialize");
    auto entities = entities_original;

    /* Add children */
    for (const auto entity : entities_original) {
        entities.merge(ecs.get_component<tmt::Transform>(entity).get_all_children());
    }

    SerializeState state {ecs, entities_original, entities};

    Json result;
    result["version"] = tmt::Serializer::Config::VERSION;

    Json& json_entities = result["entities"];
    for (const auto entity : entities) {
        json_entities.push_back(entity);
    }

    tmt::SerializeComponents::for_each([&state, &result](auto type_tag) {
        using ComponentType = typename decltype(type_tag)::type;

        /* If no component instance exists, continue */
        if (state.ecs.get_registry().view<ComponentType>().size() <= 0) return;

        constexpr auto COMPONENT_NAME = tmt::Component<ComponentType>::get_name();
        TMT_ZONE_SCOPED_N(COMPONENT_NAME);

        Json& components = result[COMPONENT_NAME];

        for (const auto entity : state.all_entities) {
            if (state.ecs.has_component<ComponentType>(entity) == false) continue;

            const auto key = tmt::EntityHelper::to_string(entity);
            const auto value = tmt::Serializer::serialize(state.ecs.get_component<ComponentType>(entity));
            components[key] = value;
        }
    });

    return result;
}

/* [Deserialize] */
void tag_invoke(JsonReflect::deserialize_t, const Json& j, std::set<tmt::Entity>& new_entities, tmt::Ecs& ecs) {
    TMT_ZONE_SCOPED_NS("Ecs::deserialize");

    if (j.contains("entities") == false) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] Json does not contain \"entities\" object. Json:\n---\n%s\n---", j.dump());
        return;
    }
    if (j.contains("version") == false) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] Json does not contain \"version\" number. Json:\n---\n%s\n---", j.dump());
        return;
    }

    DeserializeState state {ecs, j};

    if (state.original_json["version"] != tmt::Serializer::Config::VERSION) {
        const Json& version = state.original_json["version"];
        tmt::Log::warn(
            tmt::Log::Scope::ENGINE, "[Serialization] Using outdated version, something might break. current version: %ui, provided version: %ui", version, tmt::Serializer::Config::VERSION
        );
    }

    const auto& json_entities = state.original_json["entities"];

    /* Create entities */
    for (const auto& json_entity : json_entities) {
        tmt::Entity deserialized_entity = entt::null;
        tmt::Serializer::deserialize(json_entity, deserialized_entity);

        if (deserialized_entity == entt::null) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] \"Entities\" object contain an entt::null entity.");
            continue;
        }

        const tmt::Entity created_entity = state.ecs.create_entity("", deserialized_entity);
        state.entity_mapping[deserialized_entity] = created_entity;
    }

    /* Deserialize each component */
    tmt::SerializeComponents::for_each([&state](auto type_tag) {
        using ComponentType = typename decltype(type_tag)::type;

        constexpr auto COMPONENT_NAME = tmt::Component<ComponentType>::get_name();
        TMT_ZONE_SCOPED_N(COMPONENT_NAME);

        if (state.original_json.contains(COMPONENT_NAME) == false) return;

        const Json& entries = state.original_json[COMPONENT_NAME];
        for (const auto& [json_key, json_value] : entries.items()) {
            const tmt::Entity json_entity = tmt::EntityHelper::from_string(json_key);

            if (json_entity == entt::null) {
                tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] %s objects contain an entt::null entry.", COMPONENT_NAME);
                continue;
            }

            const tmt::Entity entity = state.entity_mapping.at(json_entity);
            ComponentType& component_value = state.ecs.add_or_get_component<ComponentType>(entity);
            tmt::Serializer::deserialize(json_value, component_value, state);
        }
    });

    /* Return newly created entities */
    for (const auto& [_, mapped_entity] : state.entity_mapping) {
        if (mapped_entity != entt::null) {
            new_entities.insert(mapped_entity);
        }
    }
}
