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
using Dispatcher = entt::dispatcher;

struct Delete {};

template <typename T>
concept System = std::is_base_of_v<ISystem, T>;

template <typename T, typename = void>
struct EcsComponentTraits {
    static constexpr bool IS_GAME_COMPONENT = false;
    static T& add(Registry& registry, Entity entity) { return registry.emplace<T>(entity); }
    static void remove(Registry& registry, Entity entity) { registry.remove<T>(entity); }
    static T& get(Registry& registry, Entity entity) { return registry.get<T>(entity); }
    static const T& get(const Registry& registry, Entity entity) { return registry.get<T>(entity); }
    static bool has(const Registry& registry, Entity entity) { return registry.all_of<T>(entity); }
    static T& add_or_get(Registry& registry, Entity entity) { return registry.get_or_emplace<T>(entity); }
    static T* try_get(Registry& registry, Entity entity) { return registry.try_get<T>(entity); }
    static const T* try_get(const Registry& registry, Entity entity) { return registry.try_get<T>(entity); }

    /* view - non const */
    template <typename... Rest, typename... Excludes>
    static auto view_with(Registry& registry, entt::exclude_t<Excludes...> = entt::exclude_t {}) {
        return registry.view<T, Rest...>(entt::exclude_t<Excludes...> {});
    }

    /* view - const */
    template <typename... Rest, typename... Excludes>
    static auto view_with(const Registry& registry, entt::exclude_t<Excludes...> = entt::exclude_t {}) {
        return registry.view<T, Rest...>(entt::exclude_t<Excludes...> {});
    }
};

/* Ecs traits for empty components like Disable and DisableFlag */
template <typename T>
struct EcsComponentTraits<T, std::enable_if_t<std::is_empty_v<T>>> {
    static constexpr bool IS_GAME_COMPONENT = false;

    static T& add(Registry& registry, Entity entity) {
        registry.emplace<T>(entity);
        static T instance {};
        return instance;
    }

    static void remove(Registry& registry, Entity entity) { registry.remove<T>(entity); }

    static T& get(Registry&, Entity) {
        static T instance {};
        return instance;
    }

    static const T& get(const Registry&, Entity) {
        static const T INSTANCE {};
        return INSTANCE;
    }

    static bool has(const Registry& registry, Entity entity) { return registry.all_of<T>(entity); }

    static T& add_or_get(Registry& registry, Entity entity) {
        if (!registry.all_of<T>(entity)) registry.emplace<T>(entity);
        static T instance {};
        return instance;
    }

    static T* try_get(Registry& registry, Entity entity) {
        static T instance {};
        return registry.all_of<T>(entity) ? &instance : nullptr;
    }

    static const T* try_get(const Registry& registry, Entity entity) {
        static const T INSTANCE {};
        return registry.all_of<T>(entity) ? &INSTANCE : nullptr;
    }

    template <typename... Rest, typename... Excludes>
    static auto view_with(Registry& registry, entt::exclude_t<Excludes...> = entt::exclude_t {}) {
        return registry.view<T, Rest...>(entt::exclude_t<Excludes...> {});
    }

    template <typename... Rest, typename... Excludes>
    static auto view_with(const Registry& registry, entt::exclude_t<Excludes...> = entt::exclude_t {}) {
        return registry.view<T, Rest...>(entt::exclude_t<Excludes...> {});
    }
};

class Ecs : public OnGameStart, public OnGameUpdate, public OnGameFixedUpdate, public OnGameEnd, public OnEndFrame {
   public:
    Ecs() = default;
    ~Ecs() = default;

    /* Entities / Components */
    Registry& get_registry() { return registry; }
    const Registry& get_registry() const { return registry; }

    // TODO this does not handle queued events for now
    Dispatcher& get_dispatcher() { return dispatcher; }
    const Dispatcher& get_dispatcher() const { return dispatcher; }

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
            return EcsComponentTraits<Component...>::try_get(registry, entity);
        } else {
            return std::make_tuple(try_get_component<Component>(entity)...);
        }
    }

    /* Single + Multiple try_get, const */
    template <typename... Component>
    decltype(auto) try_get_component(const Entity entity) const {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return EcsComponentTraits<Component...>::try_get(registry, entity);
        } else {
            return std::make_tuple(try_get_component<Component>(entity)...);
        }
    }

    template <typename Component>
    Entity get_entity(const Component& instance) {
        const auto& storage = registry.storage<Component>();
        return entt::to_entity(storage, instance);
    }

    void destroy_entity(const Entity entity, bool force = false) {
        if (force) {
            registry.destroy(entity);
        } else {
            registry.emplace_or_replace<Delete>(entity);
        }

        /* Need to make sure it has transform since we can destroy empty entities */
        const bool has_transform = registry.all_of<Transform>(entity);
        if (has_transform == false) return;

        auto children = get_component<Transform>(entity).get_children();
        for (auto child : children) {
            destroy_entity(child, force);
        }
    }

    /* Returns a view of given elements */
    template <typename First, typename... Rest, typename... Excludes>
    auto view(entt::exclude_t<Excludes...> = entt::exclude_t {}) {
        constexpr bool ALL_NATIVE = (!EcsComponentTraits<First>::IS_GAME_COMPONENT && (!EcsComponentTraits<Rest>::IS_GAME_COMPONENT && ...));
        constexpr bool ALL_GAME = (EcsComponentTraits<First>::IS_GAME_COMPONENT && (EcsComponentTraits<Rest>::IS_GAME_COMPONENT && ...));

        static_assert(ALL_NATIVE || ALL_GAME, "Cannot mix native ECS components and IGameComponent-derived types in a single view.");
        static_assert(ALL_GAME || (!EcsComponentTraits<Excludes>::IS_GAME_COMPONENT && ...), "Cannot exclude game components from a native component view.");

        return EcsComponentTraits<First>::template view_with<Rest...>(registry, entt::exclude_t<Disable, Excludes...> {});
    }

    /* Returns a view of given elements (const) */
    template <typename First, typename... Rest, typename... Excludes>
    auto view(entt::exclude_t<Excludes...> = entt::exclude_t {}) const {
        constexpr bool ALL_NATIVE = (!EcsComponentTraits<First>::IS_GAME_COMPONENT && (!EcsComponentTraits<Rest>::IS_GAME_COMPONENT && ...));
        constexpr bool ALL_GAME = (EcsComponentTraits<First>::IS_GAME_COMPONENT && (EcsComponentTraits<Rest>::IS_GAME_COMPONENT && ...));

        static_assert(ALL_NATIVE || ALL_GAME, "Cannot mix native ECS components and IGameComponent-derived types in a single view.");
        static_assert(ALL_GAME || (!EcsComponentTraits<Excludes>::IS_GAME_COMPONENT && ...), "Cannot exclude game components from a native component view.");
        return EcsComponentTraits<First>::template view_with<Rest...>(registry, entt::exclude_t<Excludes...> {});
    }

    /* Returns a group of given elements, default ignores disabled entities */
    /* Only supports native ecs components */
    template <typename... Owned, typename... Get, typename... Exclude>
    auto group(entt::get_t<Get...> = entt::get_t {}, entt::exclude_t<Exclude...> = entt::exclude_t {}) {
        constexpr bool OWNED_ALL_NATIVE = (!EcsComponentTraits<Owned>::IS_GAME_COMPONENT && ...);
        static_assert(OWNED_ALL_NATIVE, "Only native ECS components can be owned by a group.");

        constexpr bool GET_ALL_NATIVE = (!EcsComponentTraits<Get>::IS_GAME_COMPONENT && ...);
        static_assert(GET_ALL_NATIVE, "Only native ECS components can be observed by a group.");

        constexpr bool EXCLUDE_ALL_NATIVE = (!EcsComponentTraits<Exclude>::IS_GAME_COMPONENT && ...);
        static_assert(EXCLUDE_ALL_NATIVE, "Only native ECS components can be excluded from a group.");

        /* Add Disable to ignore by default */
        return registry.group<Owned...>(entt::get_t<Get...>(), entt::exclude_t<Disable, Exclude...>());
    }

    /* Returns a group of given elements, default ignores disabled entities (const) */
    /* Only supports native ecs components */
    template <typename... Owned, typename... Get, typename... Exclude>
    auto group(entt::get_t<Get...> = entt::get_t {}, entt::exclude_t<Exclude...> = entt::exclude_t {}) const {
        constexpr bool OWNED_ALL_NATIVE = (!EcsComponentTraits<Owned>::IS_GAME_COMPONENT && ...);
        static_assert(OWNED_ALL_NATIVE, "Only native ECS components can be owned by a group.");

        constexpr bool GET_ALL_NATIVE = (!EcsComponentTraits<Get>::IS_GAME_COMPONENT && ...);
        static_assert(GET_ALL_NATIVE, "Only native ECS components can be observed by a group.");

        constexpr bool EXCLUDE_ALL_NATIVE = (!EcsComponentTraits<Exclude>::IS_GAME_COMPONENT && ...);
        static_assert(EXCLUDE_ALL_NATIVE, "Only native ECS components can be excluded from a group.");

        /* Add Disable to ignore by default */
        return registry.group<Owned...>(entt::get_t<Get...>(), entt::exclude_t<Disable, Exclude...>());
    }

    void clear() { registry.clear(); }

    bool valid(const Entity entity) const { return registry.valid(entity); }

    bool is_enabled(const Entity entity) const;

    bool is_disabled(const Entity entity) const;

    void enable(const Entity entity, const bool mark = true);

    void disable(const Entity entity, const bool mark = true);

    Collection<ISystem> systems;

    void on_game_start() override;

    void on_game_end() override;

   private:
    Registry registry;
    Dispatcher dispatcher;

    void on_game_update(const FrameData& time) override;

    void on_game_fixed_update(const FrameData& time) override;

    void on_end_frame() override;

    void propagate_enable(const Entity entity);
};

}  // namespace tmt
