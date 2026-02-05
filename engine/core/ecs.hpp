#pragma once
#include <memory>
#include <stdexcept>
#include <vector>

#include <entt/entt.hpp>

#include "engine/core/entity.hpp"

#include "engine/core/frame_data.hpp"
#include "engine/core/system.hpp"

#include "engine/core/components/name.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/collection.hpp"

#include "engine/events/game.hpp"
#include "engine/events/engine.hpp"

#include "engine/tools/fmt/helper.hpp"

#include "engine/tools/uuid.hpp"

namespace tmt {

using Registry = entt::registry;

struct Delete {};

template <typename T>
concept System = std::is_base_of_v<ISystem, T>;

template <typename T, typename = void>
struct EcsComponentTraits {
    static T& add(Registry& registry, Entity entity) { return registry.emplace<T>(entity); }
    static void remove(Registry& registry, Entity entity) { registry.remove<T>(entity); }
    static T& get(Registry& registry, Entity entity) { return registry.get<T>(entity); }
    static const T& get(const Registry& registry, Entity entity) { return registry.get<T>(entity); }
    static bool has(const Registry& registry, Entity entity) { return registry.all_of<T>(entity); }
    static T& add_or_get(Registry& registry, Entity entity) { return registry.get_or_emplace<T>(entity); }
    static T* try_get(Registry& registry, Entity entity) { return registry.try_get<T>(entity); }
    static const T* try_get(const Registry& registry, Entity entity) { return registry.try_get<T>(entity); }
};

class Ecs : public OnGameStart, public OnGameUpdate, public OnGameFixedUpdate, public OnGameEnd, public OnEndFrame {
   public:
    Ecs() = default;
    ~Ecs() = default;

    /* Entities / Components */
    Registry& get_registry() { return registry; }
    const Registry& get_registry() const { return registry; }

    /* Enforce Name component */
    Entity create_entity(const std::string& name = "", const Entity hint = entt::null) {
        Entity entity = registry.create(hint);
        /* Enforce Name component */
        registry.emplace<Name>(entity, name.empty() ? "Entity_" + EntityHelper::to_string(entity) : name);
        registry.emplace<Transform>(entity);
        auto& uuid = registry.emplace<UUID>(entity, UUIDGenerator::generate());
        uuid_entity_map[uuid] = entity;
        return entity;
    }

    /* No components enforced. Preferably use ``create_entity`` over this one */
    Entity create_empty_entity(const Entity hint = entt::null) { return registry.create(hint); }

    /* Enforce Name component */
    template <typename... Component>
    decltype(auto) create_entity(const std::string& name = "", const Entity hint = entt::null) {
        Entity entity = create_entity(name, hint);
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT > 0) {
            return add_or_get_component<Component...>(entity);
        } else {
            return entity;
        }
    }

    /* No components enforced. Preferably use ``create_entity`` over this one */
    template <typename... Component>
    Entity create_empty_entity(const Entity hint = entt::null) {
        Entity entity = create_empty_entity(hint);
        (add_component<Component>(entity), ...);
        return entity;
    }

    /* Single + Multiple add */
    template <typename... Component>
    decltype(auto) add_component(Entity entity) {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return EcsComponentTraits<Component...>::add(registry, entity);
        } else {
            return std::forward_as_tuple(add_component<Component>(entity)...);
        }
    }

    /* Single + Multiple remove */
    template <typename... Component>
    void remove_component(Entity entity) {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            EcsComponentTraits<Component...>::remove(registry, entity);
        } else {
            (remove_component<Component>(entity), ...);
        }
    }

    /* Single + Multiple get, mutable */
    template <typename... Component>
    decltype(auto) get_component(const Entity entity) {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return EcsComponentTraits<Component...>::get(registry, entity);
        } else {
            return std::forward_as_tuple(get_component<Component>(entity)...);
        }
    }

    /* Single + Multiple get, const */
    template <typename... Component>
    decltype(auto) get_component(const Entity entity) const {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return EcsComponentTraits<Component...>::get(registry, entity);
        } else {
            return std::forward_as_tuple(get_component<Component>(entity)...);
        }
    }

    template <typename... Component>
    bool has_component(const Entity entity) const {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return EcsComponentTraits<Component...>::has(registry, entity);
        } else {
            return (has_component<Component>(entity) && ...);
        }
    }

    template <typename... Component>
    decltype(auto) add_or_get_component(const Entity entity) {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return EcsComponentTraits<Component...>::add_or_get(registry, entity);
        } else {
            return std::make_tuple(add_or_get_component<Component>(entity)...);
        }
    }

    /* Single + Multiple try_get, mutable */
    template <typename... Component>
    decltype(auto) try_get_component(const Entity entity) {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return registry.try_get<Component...>(entity);
        } else {
            return std::make_tuple(try_get_component<Component>(entity)...);
        }
    }

    /* Single + Multiple try_get, const */
    template <typename... Component>
    decltype(auto) try_get_component(const Entity entity) const {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return registry.try_get<Component...>(entity);
        } else {
            return std::make_tuple(try_get_component<Component>(entity)...);
        }
    }

    template <typename Component>
    Entity get_entity(const Component& instance) {
        const auto& storage = registry.storage<Component>();
        return entt::to_entity(storage, instance);
    }

    std::optional<Entity> get_entity(const UUID& uuid) const {
        if (uuid_entity_map.contains(uuid)) {
            return uuid_entity_map.at(uuid);
        }
        return std::nullopt;
    }

    UUID get_uuid(const Entity entity) const {
        const auto& uuid = get_component<UUID>(entity);
        return uuid;
    }

    void destroy_entity(const Entity entity, bool force = false) {
        if (force) {
            registry.destroy(entity);
        } else {
            registry.emplace_or_replace<Delete>(entity);
        }

        auto children = get_component<Transform>(entity).get_children();
        for (auto child : children) {
            destroy_entity(child, force);
        }
    }

    void clear() { registry.clear(); }

    bool valid(const Entity entity) const { return registry.valid(entity); }

    Collection<ISystem> systems;

   private:
    Registry registry;
    std::unordered_map<UUID, Entity> uuid_entity_map;

    void on_game_start() override;

    void on_game_update(const FrameData& time) override;

    void on_game_fixed_update(const FrameData& time) override;

    void on_game_end() override;

    void on_end_frame() override;
};

}  // namespace tmt