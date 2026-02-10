#pragma once
#include "component_collection.hpp"
#include "engine/systems/gameplay/game_component.hpp"

namespace tmt {

IGameComponent& ComponentCollection::add_component(const ComponentIndex& type_id, Entity entity) {
    GameComponentRegistry::is_registered_or_throw(type_id);

    if (has_component(type_id)) {
        Log::error(Log::Scope::ENGINE, "[ComponentCollection] add_component: Component already exists on entity.");
        throw std::runtime_error("[ComponentCollection] add_component: Component already exists on entity.");
    }

    auto info = engine.component_registry.get_component_info(type_id);
    components[type_id] = info.factory.create(entity);
    return *components[type_id];
}

IGameComponent& ComponentCollection::get_component(const ComponentIndex& type_id) {
    GameComponentRegistry::is_registered_or_throw(type_id);
    return *components.at(type_id);
}

const IGameComponent& ComponentCollection::get_component(const ComponentIndex& type_id) const {
    GameComponentRegistry::is_registered_or_throw(type_id);
    return *components.at(type_id);
}

IGameComponent& ComponentCollection::add_or_get_component(const ComponentIndex& type_id, Entity entity) {
    GameComponentRegistry::is_registered_or_throw(type_id);
    if (!has_component(type_id)) {
        return add_component(type_id, entity);
    }
    return get_component(type_id);
}

void ComponentCollection::remove_component(const ComponentIndex& type_id) {
    GameComponentRegistry::is_registered_or_throw(type_id);
    components.erase(type_id);
}

bool ComponentCollection::has_component(const ComponentIndex& type_id) const {
    GameComponentRegistry::is_registered_or_throw(type_id);
    return components.contains(type_id);
}

}  // namespace tmt