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
#include "engine/tools/types/tree_map.hpp"
#include "engine/tools/types/scene_json.hpp"

namespace tmt {

struct SerializeState {
    SerializeState(const tmt::Ecs& ecs_ref, const std::set<tmt::Entity>& original_entities_ref, const std::set<tmt::Entity>& all_entities_ref) :
        ecs(ecs_ref), original_entities(original_entities_ref), all_entities(all_entities_ref) {}

    const tmt::Ecs& ecs;
    /* Original provided entities */
    const std::set<tmt::Entity>& original_entities;
    /* Originals + children */
    const std::set<tmt::Entity>& all_entities;

    /* per prefab */
    std::set<tmt::Entity> referenced_entities_scope;
    /* Global */
    std::unordered_map<tmt::Entity, std::set<tmt::Entity>> reference_to_referencers;

    bool is_being_referenced(const tmt::Entity& entity) const {
        if (reference_to_referencers.contains(entity) == false) {
            return false;
        }

        // const bool is_prefab = ecs.has_component<tmt::Prefab>(entity);
        // if (is_prefab) {
        //     return false;
        // }

        const auto& referencers = reference_to_referencers.at(entity);
        for (const auto& referencer : referencers) {
            const bool has_referencer_prefab = ecs.has_component<tmt::Prefab>(referencer);
            if (has_referencer_prefab == false) {
                return true;
            }

            const auto& referencer_prefab_comp = ecs.get_component<tmt::Prefab>(referencer);
            if (current_prefab_instance_id.has_value()) {
                if (referencer_prefab_comp.instance_id != current_prefab_instance_id) {
                    return true;
                }
            }

            const bool has_self_prefab = ecs.has_component<tmt::Prefab>(entity);
            if (has_self_prefab) {
                const auto& self_prefab_comp = ecs.get_component<tmt::Prefab>(entity);
                if (self_prefab_comp.instance_id != referencer_prefab_comp.instance_id) {
                    return true;
                }
            }
        }
        return false;
    }

    /* All entities coming from prefabs that have been changed */
    /* Needed for tracking changes in prefab instances */
    std::set<tmt::Entity> changed_prefab_entities;

    std::optional<tmt::PrefabInstanceID> current_prefab_instance_id;

    tmt::SceneJson json;
};

struct DeserializeState {
    tmt::Ecs& ecs;
    const tmt::SceneJson& json;

    tmt::PrefabInstanceID current_prefab_instance_id { tmt::NULL_UUID };
    tmt::Entity current_root_entity = entt::null;
    tmt::IO::FileLocation current_prefab_location;

    std::unordered_map<tmt::PrefabInstanceID, tmt::PrefabInstanceID> instance_id_mapping { { tmt::NULL_UUID, tmt::NULL_UUID } };

    std::unordered_map<tmt::PrefabInstanceID, std::unordered_map<tmt::Entity, tmt::Entity>> entity_mappings { { tmt::NULL_UUID, { { entt::null, entt::null } } } };

    // std::set<tmt::Entity> deleted_entities;
    std::unordered_map<tmt::PrefabInstanceID, std::set<tmt::Entity>> deleted_entities;

    std::optional<Entity> get_mapping(const tmt::PrefabInstanceID instance, const tmt::Entity entity) const {
        if (entity_mappings.contains(instance) == false) {
            return std::nullopt;
        }
        const auto& mapping = entity_mappings.at(instance);
        if (mapping.contains(entity) == false) {
            return std::nullopt;
        }
        return mapping.at(entity);
    }

    std::optional<Entity> get_root_mapping(const tmt::Entity entity) const { return get_mapping(tmt::NULL_UUID, entity); }

    std::optional<Entity> get_current_mapping(const tmt::Entity entity) const { return get_mapping(current_prefab_instance_id, entity); }

    void add_mapping(const tmt::PrefabInstanceID instance, const tmt::Entity from, const tmt::Entity to) {
        if (entity_mappings.contains(instance) == false) {
            entity_mappings[instance] = { { entt::null, entt::null } };
        }
        const bool already_contains = entity_mappings[instance].contains(from);
        if (already_contains) {
            const auto entry = entity_mappings[instance][from];
            if (entry != to) {
                tmt::Log::debug(
                    tmt::Log::Scope::ENGINE, "[Serialization] Overriding existing mapping for instance \"{}\" from entity {} to entity {} (was mapped to {})", instance, from, to,
                    entity_mappings[instance][from]
                );
            }
        }
        entity_mappings[instance][from] = to;
    }

    void remove_mapping(const tmt::PrefabInstanceID instance, const tmt::Entity from) {
        if (entity_mappings.contains(instance)) {
            entity_mappings[instance].erase(from);
        }
    }

    bool is_deleted(const tmt::PrefabInstanceID instance, const tmt::Entity entity) const {
        //
        return deleted_entities.contains(instance) && deleted_entities.at(instance).contains(entity);
    }
};

}  // namespace tmt
/* Entity with state */
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, tmt::SerializeState& state) {
    if (state.current_prefab_instance_id.has_value()) {
        const bool is_prefab = state.ecs.has_component<tmt::Prefab>(entity);
        if (is_prefab) {
            const auto& prefab_comp = state.ecs.get_component<tmt::Prefab>(entity);
            if (prefab_comp.instance_id == state.current_prefab_instance_id.value()) {
                return static_cast<std::uint32_t>(prefab_comp.source_entity);
            }
        }
    }

    state.referenced_entities_scope.insert(entity);
    return static_cast<std::uint32_t>(entity);
}

/* Prefab with state */
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::Prefab& prefab, tmt::SerializeState& state) {
    tmt::json j;
    j["source_location"] = tmt::Serializer::serialize(prefab.source_location, state);
    j["source_entity"] = tmt::Serializer::serialize(prefab.source_entity); /* No state */
    j["root_entity"] = tmt::Serializer::serialize(prefab.root_entity, state);
    j["instance_id"] = tmt::Serializer::serialize(prefab.instance_id, state);
    return j;
}

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Entity& entity, const tmt::DeserializeState& state) {
    tmt::Entity deserialized_entity = entt::null;
    tmt::Serializer::deserialize(j, deserialized_entity);

    auto mapping = state.get_current_mapping(deserialized_entity);
    if (mapping.has_value() == false) {
        entity = deserialized_entity;
        const bool is_valid_entity = state.ecs.valid(entity);
        const auto& prefab_location = state.current_prefab_location;
        tmt::Log::debug(
            tmt::Log::Scope::ENGINE, "[Serialization] No existing mapping found for entity {} in prefab {} instance {}, using {} deserialized entity directly.", deserialized_entity,
            prefab_location, state.current_prefab_instance_id, is_valid_entity ? "(valid)" : "(invalid)"
        );
        return;
    }

    const tmt::Entity mapped_entity = mapping.value();

    if (state.is_deleted(state.current_prefab_instance_id, mapped_entity)) {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serialization] Entity {} is referenced but has been deleted, setting value to entt::null.", deserialized_entity);
        entity = entt::null;
        return;
    }

    entity = mapped_entity;
}

/* ECS */
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::Ecs& ecs) {
    std::set<tmt::Entity> entities;
    for (const auto entity : ecs.view<entt::entity>()) {
        entities.insert(entity);
    }
    tmt::json result = tmt::Serializer::serialize(entities, ecs);
    tmt::json& systems = result["systems"];
    // const auto size = ecs.systems.size();
    for (const auto& system : ecs.systems) {
        const auto serialized = system->serialize();
        if (serialized.empty() == false) {
            const auto name = system->get_name();
            systems[name] = serialized;
        }
    }

    return result;
}

void tag_invoke(JsonReflect::deserialize_t, const tmt::json& j, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data) {
    std::set<tmt::Entity> new_entities;
    tmt::Serializer::deserialize(j, new_entities, ecs, prefab_data);

    if (j.contains("systems")) {
        const tmt::json& systems = j["systems"];
        for (auto& system : ecs.systems) {
            const auto name = system->get_name();
            if (systems.contains(name)) {
                const tmt::json& system_json = systems[name];
                system->deserialize(system_json);
            }
        }
    }
}

/* Single entity */
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::Entity& entity, const tmt::Ecs& ecs) {
    const std::set<tmt::Entity> entities { entity };
    return tmt::Serializer::serialize(entities, ecs);
}

void tag_invoke(JsonReflect::deserialize_t, const tmt::json& j, tmt::Entity& entity, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data) {
    std::set<tmt::Entity> new_entities;
    tmt::Serializer::deserialize(j, new_entities, ecs, prefab_data);
    if (new_entities.empty()) {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serialization] Deserialized entity JSON resulted in no entities.");
        entity = entt::null;
    } else {
        entity = *tmt::EntityHelper::upper_parents(new_entities).begin();
    }
}

std::optional<tmt::json> recursive_find_component_json(const tmt::SceneJson& prefab_json, const std::string& component_name, const tmt::Entity& entity_id) {
    return prefab_json.component_value(component_name, entity_id);
}

static void apply_patch_override(tmt::json& base, const tmt::json& override) {
    for (const auto& operation : override) {
        try {
            nlohmann::json singlePatch = nlohmann::json::array({ operation });
            base = base.patch(singlePatch);
        } catch (const nlohmann::json::out_of_range& e) {
            tmt::Log::warn(
                tmt::Log::Scope::ENGINE, "[Serialization] Failed to apply patch override due to out of range error. This might be caused by a component structure change. Error: {}", e.what()
            );
            continue;
        } catch (const std::exception& e) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] Failed to apply patch override. Error: %s", e.what());
        }
    }
}

template <typename ComponentType>
std::optional<tmt::json> get_source_component_json(const tmt::Entity entity, const tmt::SerializeState& state) {
    TMT_ZONE_SCOPED_N("get_source_component_json")
    /* Go fromsource prefab all the way up */
    const bool has_prefab = state.ecs.has_component<tmt::Prefab>(entity);
    if (has_prefab == false) return std::nullopt;

    constexpr auto COMPONENT_NAME = tmt::Component<ComponentType>::get_name();

    const tmt::Prefab& prefab = state.ecs.get_component<tmt::Prefab>(entity);
    const tmt::Entity source_entity = prefab.source_entity;
    const tmt::ResourceRef<tmt::Json> source_parsed_json = tmt::engine.resources.load_resource<tmt::Json>(prefab.source_location);
    const tmt::SceneJson source_prefab_json(source_parsed_json->get_parsed_json());

    std::optional<tmt::json> result = source_prefab_json.component_value(COMPONENT_NAME, source_entity);

    return result;
}

template <typename ComponentType>
void serialize_component(tmt::SerializeState& state) {
    /* If no component instance exists, continue */
    if (state.ecs.view<ComponentType>().size() <= 0) return;

    constexpr auto COMPONENT_NAME = tmt::Component<ComponentType>::get_name();
    TMT_ZONE_SCOPED_N(COMPONENT_NAME);

    for (const auto entity : state.all_entities) {
        if (state.ecs.has_component<ComponentType>(entity) == false) continue;
        const auto& component = state.ecs.get_component<ComponentType>(entity);

        /*TMT_ZONE_SCOPED_STRING(tmt::EntityHelper::to_string(entity));*/

        if constexpr (std::is_same_v<ComponentType, tmt::Prefab>) {
            const bool changed = state.changed_prefab_entities.contains(entity);
            const bool referenced = state.is_being_referenced(entity);
            const bool is_root = component.root_entity == entity;
            if (!changed && (!is_root) && !referenced) {
                continue;
            }
        } else {
            state.referenced_entities_scope.clear();
        }

        if constexpr (std::is_same_v<ComponentType, tmt::Prefab> == false) {
            const std::optional<tmt::json> source_value = get_source_component_json<ComponentType>(entity, state);

            if (source_value.has_value()) {
                TMT_ZONE_SCOPED_N("diff_component");
                /* Guarenteed to have a prefab, diff it */
                const tmt::Prefab& prefab_comp = state.ecs.get_component<tmt::Prefab>(entity);

                state.current_prefab_instance_id = prefab_comp.instance_id;
                const tmt::json component_value_json = tmt::Serializer::serialize(component, state);
                state.current_prefab_instance_id = std::nullopt;

                const tmt::json diff = nlohmann::json::diff(source_value.value(), component_value_json);
                if (diff.empty()) {
                    continue;
                }

                tmt::json value = tmt::json::object();
                value["diff"] = diff;

                tmt::json& components = state.json.components(COMPONENT_NAME);
                state.json.add_entity_entry(components, entity, value);
                state.changed_prefab_entities.insert(entity);
                continue;
            }
        }
        TMT_ZONE_SCOPED_N("serialize_component");
        tmt::json& components = state.json.components(COMPONENT_NAME);
        const auto value = tmt::Serializer::serialize(component, state);
        state.json.add_entity_entry(components, entity, value);

        for (const auto& referenced_entity : state.referenced_entities_scope) {
            state.reference_to_referencers[referenced_entity].insert(entity);
        }
    }
}

/* [Serialize] Set of entities */
tmt::json tag_invoke(JsonReflect::serialize_t, const std::set<tmt::Entity>& entities_original, const tmt::Ecs& ecs) {
    TMT_ZONE_SCOPED_NS("Ecs::serialize");

    /* Add children */
    auto entities = entities_original;
    for (const auto entity : entities_original) {
        entities.merge(ecs.get_component<tmt::Transform>(entity).get_all_children());
    }

    tmt::SerializeState state { ecs, entities_original, entities };

    state.json.set_version(tmt::Serializer::Config::VERSION);

    /* Add entities object at the top*/
    state.json.entities();

    /* Serialize each component */
    tmt::SerializeComponents::for_each([&state](auto type_tag) {
        using ComponentType = typename decltype(type_tag)::type;
        if constexpr (!std::is_same_v<ComponentType, tmt::Prefab>) {
            serialize_component<ComponentType>(state);
        }
    });
    /* Serialize Prefab component last */
    serialize_component<tmt::Prefab>(state);

    /* Add entities */
    for (const auto entity : state.all_entities) {
        const bool is_prefab = ecs.has_component<tmt::Prefab>(entity);

        bool should_add = !is_prefab;
        if (is_prefab) {
            const tmt::Prefab& prefab = ecs.get_component<tmt::Prefab>(entity);
            const bool is_root = prefab.root_entity == entity;
            should_add |= is_root;
            should_add |= state.changed_prefab_entities.contains(entity);
            should_add |= state.is_being_referenced(entity);
        }

        if (should_add) {
            state.json.entities().push_back(entity);
        }
    }

    return state.json.get_json();
}

template <typename ComponentType>
void deserialize_component(tmt::DeserializeState& state) {
    constexpr auto COMPONENT_NAME = tmt::Component<ComponentType>::get_name();
    TMT_ZONE_SCOPED_N(COMPONENT_NAME);

    if (state.json.has_components(COMPONENT_NAME) == false) return;

    std::set<tmt::Entity> skipped_entities {};
    const tmt::json& entries = state.json.components(COMPONENT_NAME);
    /* Add components */
    {
        TMT_ZONE_SCOPED_N("Ecs::deserialize_component::add_components");
        for (const auto& [json_key, json_value] : entries.items()) {
            tmt::Entity entity_key = tmt::EntityHelper::from_string(json_key);

            if (entity_key == entt::null) {
                tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] %s objects contain an entt::null entry.", COMPONENT_NAME);
                continue;
            }

            // todo: check if component removed in diff

            auto mapping = state.get_current_mapping(entity_key);
            if (mapping.has_value() == false) {
                tmt::Log::debug(
                    tmt::Log::Scope::ENGINE, "[Serialization] Entity: {} with instance id: {} has no entry in mapping: {}", entity_key, state.current_prefab_instance_id, state.entity_mappings
                );
                continue;
            }

            if (state.is_deleted(state.current_prefab_instance_id, entity_key)) {
                continue;
            }

            const tmt::Entity entity = mapping.value();
            const bool is_diffed = json_value.is_object() && json_value.contains("diff");

            if (is_diffed) {
                const bool has_prefab = state.ecs.has_component<tmt::Prefab>(entity);
                if (has_prefab) {
                    /* check if component is still in prefab */
                    const tmt::Prefab& prefab_comp = state.ecs.get_component<tmt::Prefab>(entity);
                    const tmt::Entity source_entity = prefab_comp.source_entity;

                    const tmt::ResourceRef<tmt::Json> source_parsed_json = tmt::engine.resources.load_resource<tmt::Json>(prefab_comp.source_location);
                    const tmt::SceneJson source_prefab_json(source_parsed_json->get_parsed_json());
                    const std::optional<tmt::json> source_component_json = recursive_find_component_json(source_prefab_json, COMPONENT_NAME, source_entity);

                    if (source_component_json.has_value() == false) {
                        /* Component removed in prefab, skip */
                        skipped_entities.insert(entity_key);
                        tmt::Log::debug(
                            tmt::Log::Scope::ENGINE,
                            "[Serialization] Entity {} component {} is diffed but has no source component in prefab, skipping deserialization of this component. This likely means the "
                            "component was removed in the prefab.",
                            entity_key, COMPONENT_NAME
                        );
                        continue;
                    }
                }
            }

            state.ecs.add_or_get_component<ComponentType>(entity);
        }
    }

    /* Deserialize values */
    {
        for (const auto& [json_key, json_value] : entries.items()) {
            tmt::Entity entity_key = tmt::EntityHelper::from_string(json_key);

            if (entity_key == entt::null) {
                tmt::Log::debug(tmt::Log::Scope::ENGINE, "[Serialization] {} objects contain an entt::null entry in file: {}", COMPONENT_NAME, state.current_prefab_location);
                continue;
            }

            if (skipped_entities.contains(entity_key)) {
                continue;
            }

            auto mapping = state.get_current_mapping(entity_key);
            if (mapping.has_value() == false) {
                tmt::Log::debug(
                    tmt::Log::Scope::ENGINE, "[Serialization] Entity: {} with instance id: {} has no entry in mapping: {}", entity_key, state.current_prefab_instance_id, state.entity_mappings
                );
                continue;
            }

            const tmt::Entity entity = mapping.value();
            if (state.is_deleted(state.current_prefab_instance_id, entity)) {
                continue;
            }
            ComponentType& component_value = state.ecs.get_component<ComponentType>(entity);

            /* check if diff */
            if (json_value.is_object() && json_value.contains("diff")) {
                TMT_ZONE_SCOPED_N("Ecs::deserialize_component::apply_diff");
                const tmt::json diff = json_value.at("diff");
                tmt::SerializeState temp_state { state.ecs, {}, {} };
                tmt::json base = tmt::Serializer::serialize(component_value, temp_state);
                // tmt::json patched = base.patch(diff);
                apply_patch_override(base, diff);
                tmt::Serializer::deserialize<ComponentType>(base, component_value, state);
            } else {
                TMT_ZONE_SCOPED_N("Ecs::deserialize_component::deserialize_component");
                if constexpr (std::is_same_v<ComponentType, tmt::Prefab>) {
                    /* Special handling for Prefab component */
                    tmt::Prefab prefab_original;
                    tmt::Serializer::deserialize<tmt::Prefab>(json_value, prefab_original);
                    tmt::Serializer::deserialize<ComponentType>(json_value, component_value, state);
                    component_value.source_entity = prefab_original.source_entity;

                    if (state.instance_id_mapping.contains(prefab_original.instance_id)) {
                        component_value.instance_id = state.instance_id_mapping.at(prefab_original.instance_id);
                    }

                } else {
                    tmt::Serializer::deserialize<ComponentType>(json_value, component_value, state);
                }
            }
        }
    }
}

/* [Deserialize] */
static void deserialize_scene(std::set<tmt::Entity>& new_entities, tmt::DeserializeState& state) {
    TMT_ZONE_SCOPED_N("Ecs::deserialize_scene")
    if (state.json.entities() == false) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] Json does not contain \"entities\" object. Json:\n---\n%s\n---", state.json.dump());
        return;
    }
    if (state.json.has_version() == false) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] Json does not contain \"version\" number. Json:\n---\n%s\n---", state.json.dump());
        return;
    }

    const uint64_t version = state.json.get_version();
    if (version != tmt::Serializer::Config::VERSION) {
        tmt::Log::warn(
            tmt::Log::Scope::ENGINE, "[Serialization] Using outdated version, something might break. current version: %ui, provided version: %ui", version, tmt::Serializer::Config::VERSION
        );
    }

    const auto& json_entities = state.json.entities();
    std::unordered_map<tmt::Entity, tmt::Entity> new_to_old;

    /* Create entities */
    {
        TMT_ZONE_SCOPED_N("Ecs::deserialize_scene::create_entities")
        for (const auto& json_entity : json_entities) {
            tmt::Entity deserialized_entity_id = entt::null;
            tmt::Serializer::deserialize(json_entity, deserialized_entity_id);

            if (deserialized_entity_id == entt::null) {
                tmt::Log::error(tmt::Log::Scope::ENGINE, "[Serialization] \"Entities\" object contain an entt::null entity.");
                continue;
            }

            auto mapping = state.get_current_mapping(deserialized_entity_id);
            if (mapping) {
                /* Entity already exists in mapping, skip */
                new_to_old.insert({ mapping.value(), deserialized_entity_id });
                continue;
            } else {
                /* Create entity and add to mapping */
                const tmt::Entity created_entity = state.ecs.create_empty_entity(deserialized_entity_id);
                /* Register scene entities */
                // state.entity_mappings[state.current_prefab_instance_id][deserialized_entity_id] = created_entity;
                state.add_mapping(state.current_prefab_instance_id, deserialized_entity_id, created_entity);
                new_to_old.insert({ created_entity, deserialized_entity_id });
                new_entities.insert(created_entity);
            }
        }
    }

    /* Gather all already existing prefabs in scene */
    std::unordered_set<tmt::Prefab> existing_prefab_instances;
    std::unordered_map<tmt::PrefabInstanceID, tmt::Prefab> prefab_instance_ids;

    {
        TMT_ZONE_SCOPED_N("Ecs::deserialize_scene::prepare_prefabs")
        auto view = state.ecs.view<tmt::Prefab>();
        for (const auto&& [entity, prefab_comp] : view.each()) {
            existing_prefab_instances.insert(prefab_comp);
        }

        for (const auto& [new_entity, old_entity] : new_to_old) {
            constexpr auto PREFAB_COMPONENT_NAME = tmt::Component<tmt::Prefab>::get_name();
            auto prefab_entry = state.json.component_value(PREFAB_COMPONENT_NAME, old_entity);
            if (prefab_entry.has_value() == false) {
                continue;
            }

            tmt::Prefab prefab_comp {};
            tmt::Serializer::deserialize<tmt::Prefab>(prefab_entry.value(), prefab_comp);

            /* Check if the original entity is still in the prefab since it could be deleted */
            const auto json = tmt::engine.resources.load_resource<tmt::Json>(prefab_comp.source_location);
            const auto prefab_json = tmt::SceneJson(json->get_parsed_json());
            const auto& entities_in_prefab_json = prefab_json.entities();

            std::set<tmt::Entity> entities_in_prefab {};
            tmt::Serializer::deserialize(entities_in_prefab_json, entities_in_prefab);

            const bool entity_still_exists = entities_in_prefab.contains(prefab_comp.source_entity);
            if (entity_still_exists == false) {
                tmt::Log::debug(
                    tmt::Log::Scope::ENGINE,
                    "[Serialization] Prefab source entity {} not found in prefab JSON. This might be caused by a prefab structure change, marking it for delete. PrefabInstanceID: {}, "
                    "SourceLocation: {}",
                    prefab_comp.source_entity, prefab_comp.instance_id, prefab_comp.source_location
                );

                /* Mark it for delete */
                /* Delete it next frame */
                state.ecs.destroy_entity(new_entity, false);
                state.deleted_entities[state.current_prefab_instance_id].insert(new_entity);
                // state.remove_mapping(state.current_prefab_instance_id, old_entity);
            }

            /* Check if prefab instance already exists in scene, if yes, generate new instance id */
            if (existing_prefab_instances.contains(prefab_comp)) {
                // generate new ID
                tmt::PrefabInstanceID new_instance_id = tmt::UUIDGenerator::generate();

                state.instance_id_mapping[prefab_comp.instance_id] = new_instance_id;
                prefab_comp.instance_id = new_instance_id;
                tmt::Log::debug(
                    tmt::Log::Scope::ENGINE,
                    "[Serialization] Detected prefab instance with duplicate ID during deserialization. "
                    "Generated new instance ID. Old Instance ID: {}, New Instance ID: {}, Source Location: {}",
                    prefab_comp.instance_id,                             // old ID
                    state.instance_id_mapping[prefab_comp.instance_id],  // new ID (the remapped value)
                    prefab_comp.source_location                          // source location
                );
            }

            const tmt::PrefabInstanceID& instance_id = prefab_comp.instance_id;

            prefab_instance_ids[instance_id] = prefab_comp;

            /* add to new mapping absed on instance id */
            state.add_mapping(instance_id, old_entity, new_entity);
            state.add_mapping(instance_id, prefab_comp.source_entity, new_entity);
        }
    }
    /* Deserialize Prefabs */
    deserialize_component<tmt::Prefab>(state);
    /* for each prefab instance */
    {
        TMT_ZONE_SCOPED_N("Ecs::deserialize_scene::deserialize_prefab_instances")
        for (const auto& [prefab_instance_id, prefab_info] : prefab_instance_ids) {
            auto original_prefab_json = tmt::engine.resources.load_resource<tmt::Json>(prefab_info.source_location);
            if (original_prefab_json == nullptr) {
                tmt::Log::error(
                    tmt::Log::Scope::ENGINE, "[Serialization] Failed to load prefab JSON for deserialization. PrefabInstanceID: {}, SourceLocation: {}", prefab_instance_id,
                    prefab_info.source_location
                );
                continue;
            }
            tmt::SceneJson prefab_scene_json(original_prefab_json->get_parsed_json());

            tmt::DeserializeState prefab_state { state.ecs, prefab_scene_json };

            {
                TMT_ZONE_SCOPED_N("Ecs::deserialize_scene::deserialize_prefab_instances::setup_state")
                prefab_state.current_prefab_instance_id = prefab_instance_id;
                prefab_state.current_root_entity = prefab_info.root_entity;
                prefab_state.current_prefab_location = prefab_info.source_location;
                prefab_state.entity_mappings = std::move(state.entity_mappings);   /* Inherit mappings from parent prefab */
                prefab_state.deleted_entities = std::move(state.deleted_entities); /* Inherit deleted entities from parent prefab */
            }

            std::set<tmt::Entity> prefab_entities;
            deserialize_scene(prefab_entities, prefab_state);
            state.entity_mappings = std::move(prefab_state.entity_mappings);   /* Update mappings after deserializing prefab instance */
            state.deleted_entities = std::move(prefab_state.deleted_entities); /* Update deleted entities after deserializing prefab instance */
            new_entities.insert(prefab_entities.begin(), prefab_entities.end());
        }
    }

    /* Deserialize each component */
    tmt::SerializeComponents::for_each([&state](auto type_tag) {
        using ComponentType = typename decltype(type_tag)::type;
        if constexpr (!std::is_same_v<ComponentType, tmt::Prefab>) {
            deserialize_component<ComponentType>(state);
        }
    });

    if (state.current_prefab_instance_id != tmt::NULL_UUID) {
        TMT_ZONE_SCOPED_N("Ecs::deserialize_scene::finalize")
        tmt::Entity new_root = state.current_root_entity;

        if (new_root == entt::null) {
            const auto roots = tmt::EntityHelper::upper_parents(new_entities);
            if (roots.size() != 1) {
                tmt::Log::warn(
                    tmt::Log::Scope::ENGINE, "[Serialization] Prefab instance has multiple root entities after deserialization. PrefabInstanceID: {}, Roots count: {}",
                    state.current_prefab_instance_id, roots.size()
                );
            }
            new_root = *roots.begin();
        }

        for (const auto new_entity : new_entities) {
            if (state.is_deleted(state.current_prefab_instance_id, new_entity)) {
                continue;
            }
            const bool alread_has_prefab = state.ecs.has_component<tmt::Prefab>(new_entity);
            if (alread_has_prefab) {
                tmt::Log::debug(
                    tmt::Log::Scope::ENGINE,
                    "[Serialization] Entity already has a Prefab component during finalization. This might be caused by nested prefabs or multiple references. Entity: {}, PrefabInstanceID: "
                    "{}",
                    new_entity, state.current_prefab_instance_id
                );
            } else {
                /* Set prefab */
                tmt::Prefab& prefab = state.ecs.add_component<tmt::Prefab>(new_entity);
                prefab.instance_id = state.current_prefab_instance_id;
                prefab.source_location = state.current_prefab_location;
                prefab.root_entity = new_root;
                prefab.source_entity = new_to_old.at(new_entity);
            }
        }
    }
}

void tag_invoke(JsonReflect::deserialize_t, const tmt::json& j, std::set<tmt::Entity>& new_entities, tmt::Ecs& ecs, const std::optional<tmt::Prefab> prefab_data) {
    tmt::DeserializeState state { ecs, tmt::SceneJson(j) };

    if (prefab_data.has_value()) {
        state.current_prefab_instance_id = prefab_data->instance_id;
        state.current_root_entity = prefab_data->root_entity;
        state.current_prefab_location = prefab_data->source_location;
    }
    deserialize_scene(new_entities, state);
    // state.entity_mapping.print();

    for (const auto new_entity : new_entities) {
        const bool has_disabled = ecs.has_component<tmt::DisableFlag>(new_entity);
        if (has_disabled == false) continue;

        /* If entity is disabled, disable it */
        ecs.disable(new_entity, true);
    }
}
