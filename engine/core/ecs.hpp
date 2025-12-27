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

namespace tmt {

using Registry = entt::registry;

struct Delete {};

template <typename T>
concept System = std::is_base_of_v<ISystem, T>;

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
        return entity;
    }

    /* No components enforced. Preferably use ``create_entity`` over this one */
    Entity create_empty_entity(const Entity hint = entt::null) { return registry.create(hint); }

    /* Enforce Name component */
    template <typename... Component>
    Entity create_entity(const std::string& name = "", const Entity hint = entt::null) {
        Entity entity = create_entity(name, hint);
        (registry.emplace<Component>(entity), ...);
        return entity;
    }

    /* No components enforced. Preferably use ``create_entity`` over this one */
    template <typename... Component>
    Entity create_empty_entity(const Entity hint = entt::null) {
        Entity entity = create_empty_entity(hint);
        (registry.emplace<Component>(entity), ...);
        return entity;
    }

    /* Single + Multiple add */
    template <typename... Component>
    decltype(auto) add_component(Entity entity) {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return registry.emplace<Component...>(entity);
        } else {
            return std::make_tuple(registry.emplace<Component>(entity)...);
        }
    }

    /* Single + Multiple remove */
    template <typename... Component>
    void remove_component(Entity entity) {
        (registry.remove<Component>(entity), ...);
    }

    /* Single + Multiple get, mutable */
    template <typename... Component>
    decltype(auto) get_component(const Entity entity) {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return registry.get<Component...>(entity);
        } else {
            return std::make_tuple(registry.get<Component>(entity)...);
        }
    }

    /* Single + Multiple get, const */
    template <typename... Component>
    decltype(auto) get_component(const Entity entity) const {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return registry.get<Component...>(entity);
        } else {
            return std::make_tuple(registry.get<Component>(entity)...);
        }
    }

    template <typename... Component>
    bool has_component(const Entity entity) const {
        return registry.all_of<Component...>(entity);
    }

    template <typename... Component>
    decltype(auto) add_or_get_component(const Entity entity) {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return registry.emplace_or_replace<Component...>(entity);
        } else {
            return std::make_tuple(registry.emplace_or_replace<Component>(entity)...);
        }
    }

    /* Single + Multiple try_get, mutable */
    template <typename... Component>
    decltype(auto) try_get_component(const Entity entity) {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return registry.try_get<Component...>(entity);
        } else {
            return std::make_tuple(registry.try_get<Component>(entity)...);
        }
    }

    /* Single + Multiple try_get, const */
    template <typename... Component>
    decltype(auto) try_get_component(const Entity entity) const {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return registry.try_get<Component...>(entity);
        } else {
            return std::make_tuple(registry.try_get<Component>(entity)...);
        }
    }

    template <typename Component>
    Entity get_entity(const Component& instance) {
        const auto& storage = registry.storage<Component>();
        return entt::to_entity(storage, instance);
    }

    void destroy_entity(const Entity entity, bool force = false) {
        auto children = get_component<Transform>(entity).get_all_children();
        if (force) {
            registry.destroy(entity);
        } else {
            registry.emplace<Delete>(entity);
        }

        for (const auto child : children) {
            destroy_entity(child, force);
        }
    }

    void clear() { registry.clear(); }

    Collection<ISystem> systems;

   private:
    Registry registry;

    void on_game_start() override;

    void on_game_update(const FrameData& time) override;

    void on_game_fixed_update(const FrameData& time) override;

    void on_game_end() override;

    void on_end_frame() override;
};

}  // namespace tmt