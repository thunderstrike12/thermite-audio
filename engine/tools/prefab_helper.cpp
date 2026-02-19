#include "prefab_helper.hpp"
#include "engine/core/components/prefab.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/json.hpp"

namespace tmt {

void PrefabHelper::create_prefab(const IO::FileLocation& location, const Entity& root) {
    /* unparent before */
    auto& root_transform = engine.ecs.get_component<Transform>(root);
    Entity parent = root_transform.get_parent();
    if (root_transform.has_parent()) {
        root_transform.set_parent(entt::null);
    }

    auto& root_name = engine.ecs.get_component<Name>(root);
    root_name.name = location.relative_path.stem().generic_string();

    /* Serialize to disk */
    const auto json = Serializer::serialize(root, engine.ecs);
    std::string dump = json.dump(4);

    /* Set parent back again */
    root_transform.set_parent(parent);

    IO::write_text_file(location, dump);

    /* Add Prefab components */
    std::set<Entity> entities = { root };
    entities.merge(root_transform.get_all_children());

    const PrefabInstanceID instance_id = UUIDGenerator::generate();

    for (const auto& entity : entities) {
        const bool has_prefab = engine.ecs.has_component<Prefab>(entity);
        if (has_prefab) {
            auto& prefab_comp = engine.ecs.get_component<Prefab>(entity);
            prefab_comp.prefab_chain.push_back(
                {
                    location,
                    entity,
                    instance_id,
                }
            );
        } else {
            auto& prefab_comp = engine.ecs.add_or_get_component<Prefab>(entity);
            prefab_comp.source_location = location;
            prefab_comp.instance_id = instance_id;
            prefab_comp.root_entity = root;
            prefab_comp.source_entity = entity;
        }
    }
}

Entity PrefabHelper::instantiate_prefab(const IO::FileLocation& location, const Entity& parent) {
    /* Load prefab json from disk */
    const auto prefab_json_resource = engine.resources.load_resource<Json>(location);
    if (prefab_json_resource == nullptr) {
        Log::error(Log::Scope::ENGINE, "[PrefabHelper] instantiate_prefab: Failed to load prefab json at location '{}'", location.get_absolute_path().string());
        return entt::null;
    }

    return instantiate_prefab(prefab_json_resource->get_parsed_json(), location, parent);
}

Entity PrefabHelper::instantiate_prefab(const ResourceRef<Json>& prefab_json, const Entity& parent) {
    if (prefab_json == nullptr) {
        Log::error(Log::Scope::ENGINE, "[PrefabHelper] instantiate_prefab: Prefab json resource is nullptr");
        return entt::null;
    }

    return instantiate_prefab(prefab_json->get_parsed_json(), prefab_json->file_location, parent);
}

Entity PrefabHelper::instantiate_prefab(const tmt::json& parsed_json, const IO::FileLocation& location, const Entity& parent) {
    const bool has_prefab = engine.ecs.has_component<Prefab>(parent);
    /* Check if we create a loop */
    if (has_prefab) {
        const auto& parent_prefab_comp = engine.ecs.get_component<Prefab>(parent);
        for (const auto chain : parent_prefab_comp.prefab_chain) {
            if (chain.source_location == location) {
                Log::error(
                    Log::Scope::ENGINE, "[PrefabHelper] instantiate_prefab: Cannot instantiate prefab at location '{}' under parent entity {} as it would create a loop",
                    location.get_absolute_path().string(), EntityHelper::to_string(parent)
                );
                return entt::null;
            }
        }
    }

    /* New prefab */
    Prefab prefab_data {};
    prefab_data.source_location = location;
    prefab_data.instance_id = UUIDGenerator::generate();

    /* Deserialize entities */
    std::set<Entity> new_entities;
    Serializer::deserialize(parsed_json, new_entities, engine.ecs, prefab_data);
    if (new_entities.empty()) {
        Log::error(Log::Scope::ENGINE, "[PrefabHelper] instantiate_prefab: No entities deserialized from prefab at location '{}'", location.get_absolute_path().string());
        return entt::null;
    }

    /* Set parent */
    const Entity root_entity = *EntityHelper::upper_parents(new_entities).begin();
    auto& root_transform = engine.ecs.get_component<Transform>(root_entity);
    root_transform.set_parent(parent);
    return parent == entt::null ? root_entity : parent;
}

}  // namespace tmt