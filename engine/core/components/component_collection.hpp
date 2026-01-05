#pragma once
#include <memory>
#include <type_traits>
#include <typeindex>

#include "engine/core/ecs.hpp"
#include "engine/systems/gameplay/types.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/gameplay/game_component_registry.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {
/* Attached to an entity */
class ComponentCollection {
   public:
    ComponentCollection() = default;
    ~ComponentCollection() = default;

    ComponentCollection(const ComponentCollection&) = delete;
    ComponentCollection& operator=(const ComponentCollection&) = delete;

    ComponentCollection(ComponentCollection&&) = default;
    ComponentCollection& operator=(ComponentCollection&&) = default;

    /* Compile time */
    template <typename T>
        requires std::is_base_of_v<IGameComponent, T>
    T& add_component(Entity entity) {
        const ComponentIndex type_id = typeid(T);
        GameComponentRegistry::is_registered_or_throw(type_id);

        components[type_id] = std::make_unique<T>(entity);
        return static_cast<T&>(*components[type_id]);
    }

    template <typename T>
        requires std::is_base_of_v<IGameComponent, T>
    T& get_component() {
        const ComponentIndex type_id = typeid(T);
        GameComponentRegistry::is_registered_or_throw(type_id);

        return static_cast<T&>(*components.at(type_id));
    }

    template <typename T>
        requires std::is_base_of_v<IGameComponent, T>
    const T& get_component() const {
        const ComponentIndex type_id = typeid(T);
        GameComponentRegistry::is_registered_or_throw(type_id);

        return static_cast<const T&>(*components.at(type_id));
    }

    template <typename T>
        requires std::is_base_of_v<IGameComponent, T>
    void remove_component() {
        const ComponentIndex type_id = typeid(T);
        GameComponentRegistry::is_registered_or_throw(type_id);

        components.erase(type_id);
    }

    template <typename T>
        requires std::is_base_of_v<IGameComponent, T>
    bool has_component() const {
        const ComponentIndex type_id = typeid(T);
        GameComponentRegistry::is_registered_or_throw(type_id);

        return components.contains(type_id);
    }

    /* Runtime */
    IGameComponent& add_component(const ComponentIndex& type_id, Entity entity) {
        GameComponentRegistry::is_registered_or_throw(type_id);

        if (has_component(type_id)) {
            Log::error(Log::Scope::ENGINE, "[ComponentCollection] add_component: Component already exists on entity.");
            throw std::runtime_error("[ComponentCollection] add_component: Component already exists on entity.");
        }

        auto info = engine.component_registry.get_component_info(type_id);
        components[type_id] = info.factory.create(entity);
        return *components[type_id];
    }

    IGameComponent& get_component(const ComponentIndex& type_id) {
        GameComponentRegistry::is_registered_or_throw(type_id);
        return *components.at(type_id);
    }

    const IGameComponent& get_component(const ComponentIndex& type_id) const {
        GameComponentRegistry::is_registered_or_throw(type_id);
        return *components.at(type_id);
    }

    IGameComponent& add_or_get_component(const ComponentIndex& type_id, Entity entity) {
        GameComponentRegistry::is_registered_or_throw(type_id);
        if (!has_component(type_id)) {
            return add_component(type_id, entity);
        }
        return get_component(type_id);
    }

    void remove_component(const ComponentIndex& type_id) {
        GameComponentRegistry::is_registered_or_throw(type_id);
        components.erase(type_id);
    }

    bool has_component(const ComponentIndex& type_id) const {
        GameComponentRegistry::is_registered_or_throw(type_id);
        return components.contains(type_id);
    }

    const std::unordered_map<ComponentIndex, std::unique_ptr<IGameComponent>>& get_all_components() const { return components; }

   private:
    BEFRIEND_VISITABLE();
    std::unordered_map<ComponentIndex, std::unique_ptr<IGameComponent>> components;
};
}  // namespace tmt

TMT_COMPONENT(tmt::ComponentCollection, "ComponentCollection", (components));

/* Specialization for IGameComponent derived types */
namespace tmt {
template <typename T>
struct EcsComponentTraits<T, std::enable_if_t<std::is_base_of_v<IGameComponent, T>>> {
    static T& add(Registry& registry, Entity entity) {
        auto& collection = registry.get_or_emplace<ComponentCollection>(entity);
        return collection.add_component<T>(entity);
    }
    static void remove(Registry& registry, Entity entity) {
        auto& collection = registry.get_or_emplace<ComponentCollection>(entity);
        collection.remove_component<T>();
    }
    static T& get(Registry& registry, Entity entity) {
        auto& collection = registry.get_or_emplace<ComponentCollection>(entity);
        return collection.get_component<T>();
    }
    static const T& get(const Registry& registry, Entity entity) {
        if (registry.all_of<ComponentCollection>(entity) == false) {
            Log::error(Log::Scope::ENGINE, "[EcsComponentTraits] get: Entity does not have ComponentCollection.");
            throw std::runtime_error("[EcsComponentTraits] get: Entity does not have ComponentCollection.");
        }

        const auto& collection = registry.get<ComponentCollection>(entity);
        return collection.get_component<T>();
    }
    static bool has(const Registry& registry, Entity entity) {
        if (registry.all_of<ComponentCollection>(entity) == false) {
            return false;
        }

        const auto& collection = registry.get<ComponentCollection>(entity);
        return collection.has_component<T>();
    }
    static T& add_or_get(Registry& registry, Entity entity) {
        auto& collection = registry.get_or_emplace<ComponentCollection>(entity);
        if (!collection.has_component<T>()) {
            return collection.add_component<T>(entity);
        }

        return collection.get_component<T>();
    }
    static T* try_get(Registry& registry, Entity entity) {
        if (registry.all_of<ComponentCollection>(entity) == false) {
            return nullptr;
        }

        auto& collection = registry.get<ComponentCollection>(entity);
        if (collection.has_component<T>()) {
            return &collection.get_component<T>();
        }
        return nullptr;
    }
    static const T* try_get(const Registry& registry, Entity entity) {
        if (!registry.all_of<ComponentCollection>(entity)) {
            return nullptr;
        }

        const auto& collection = registry.get<ComponentCollection>(entity);
        if (collection.has_component<T>()) {
            return &collection.get_component<T>();
        }
        return nullptr;
    }
};
}  // namespace tmt