#pragma once
#include <memory>
#include <type_traits>
#include <typeindex>

#include "engine/core/ecs.hpp"
#include "engine/systems/gameplay/types.hpp"
#include "engine/engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/systems/gameplay/game_component_registry.hpp"
#include "engine/systems/gameplay/gameplay.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

/* Forward declare */
class IGameComponent;

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
        get_gameplay_system()->register_component_instance(type_id, components[type_id]);
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
    IGameComponent& add_component(const ComponentIndex& type_id, Entity entity);

    IGameComponent& get_component(const ComponentIndex& type_id);

    const IGameComponent& get_component(const ComponentIndex& type_id) const;

    IGameComponent& add_or_get_component(const ComponentIndex& type_id, Entity entity);

    void remove_component(const ComponentIndex& type_id);

    bool has_component(const ComponentIndex& type_id) const;

    const std::unordered_map<ComponentIndex, std::shared_ptr<IGameComponent>>& get_all_components() const { return components; }

   private:
    BEFRIEND_VISITABLE();
    std::unordered_map<ComponentIndex, std::shared_ptr<IGameComponent>> components;

    static Gameplay* get_gameplay_system() {
        auto* game_play_system = engine.ecs.systems.try_get<Gameplay>();
        if (game_play_system == nullptr) {
            Log::error(Log::Scope::ENGINE, "[EcsComponentTraits] view: Gameplay system not found.");
            throw std::runtime_error("[EcsComponentTraits] view: Gameplay system not found.");
        }
        return game_play_system;
    }
};

}  // namespace tmt

TMT_COMPONENT(tmt::ComponentCollection, "ComponentCollection", (components));

/* Specialization for IGameComponent derived types */
namespace tmt {

template <typename... Types>
struct ViewElement {
    const Entity entity;
    std::tuple<Types&...> components;

    ViewElement(Entity entity, Types&... components) : entity(std::move(entity)), components(components...) {}

    template <typename T>
    requires(std::is_same_v<T, Types> || ...)
    T& get() {
        return std::get<T&>(components);
    }

    template <typename T>
    requires(std::is_same_v<T, Types> || ...)
    const T& get() const {
        return std::get<T&>(components);
    }

    template <typename T>
    requires(std::is_same_v<T, Types> || ...)
    operator T&() {
        return get<T>();
    }

    template <typename T>
    requires(std::is_same_v<T, Types> || ...)
    operator const T&() const {
        return get<T>();
    }

    template <std::size_t N>
    decltype(auto) get() {
        if constexpr (N == 0)
            return entity;
        else
            return std::get<N - 1>(components);
    }

    template <std::size_t N>
    decltype(auto) get() const {
        if constexpr (N == 0)
            return entity;
        else
            return std::get<N - 1>(components);
    }
};

template <typename T>
struct EcsComponentTraits<T, std::enable_if_t<std::is_base_of_v<IGameComponent, T>>> {
    static constexpr bool IS_GAME_COMPONENT = true;
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

    template <typename... Rest, typename... Excludes, typename Reg>
    static auto view_with_impl(Reg& registry, entt::exclude_t<Excludes...>) {
        const auto& instances = get_gameplay_system()->get_component_instances(typeid(T));

        std::vector<ViewElement<T, Rest...>> result;
        for (const auto& weak : instances) {
            auto component = weak.lock();
            if (!component) continue;

            Entity e = static_cast<T*>(component.get())->entity;
            if (!registry.all_of<ComponentCollection>(e)) continue;
            auto& collection = registry.get<ComponentCollection>(e);

            // Check includes
            if (!(collection.has_component<Rest>() && ...)) continue;

            // Check excludes - game components via collection, native via registry
            bool excluded = false;
            auto check_exclude = [&]<typename E>() {
                if constexpr (EcsComponentTraits<E>::IS_GAME_COMPONENT) {
                    if (collection.has_component<E>()) excluded = true;
                } else {
                    if (registry.all_of<E>(e)) excluded = true;
                }
            };
            (check_exclude.template operator()<Excludes>(), ...);
            if (excluded) continue;

            result.emplace_back(e, static_cast<T&>(*component), collection.get_component<Rest>()...);
        }
        return result;
    }

    /* non-const */
    template <typename... Rest, typename... Excludes>
    static auto view_with(Registry& registry, entt::exclude_t<Excludes...> = entt::exclude_t {}) {
        return view_with_impl<Rest...>(registry, entt::exclude_t<Excludes...> {});
    }

    /* const */
    template <typename... Rest, typename... Excludes>
    static auto view_with(const Registry& registry, entt::exclude_t<Excludes...> = entt::exclude_t {}) {
        return view_with_impl<Rest...>(registry, entt::exclude_t<Excludes...> {});
    }

   private:
    static Gameplay* get_gameplay_system() {
        auto* game_play_system = engine.ecs.systems.try_get<Gameplay>();
        if (game_play_system == nullptr) {
            Log::error(Log::Scope::ENGINE, "[EcsComponentTraits] view: Gameplay system not found.");
            throw std::runtime_error("[EcsComponentTraits] view: Gameplay system not found.");
        }
        return game_play_system;
    }
};

}  // namespace tmt

template <typename... Types>
struct std::tuple_size<tmt::ViewElement<Types...>> : std::integral_constant<std::size_t, sizeof...(Types) + 1> {};

// Index 0 = Entity
template <typename... Types>
struct std::tuple_element<0, tmt::ViewElement<Types...>> {
    using type = tmt::Entity;
};

// Index N = Nth component type
template <std::size_t N, typename... Types>
struct std::tuple_element<N, tmt::ViewElement<Types...>> {
    using type = std::tuple_element_t<N - 1, std::tuple<Types&...>>;
};

// template <typename T>
// requires(std::is_base_of_v<tmt::IGameComponent, T>)
// struct JsonReflect::Detail::delta_serialize<T> : std::true_type {};
//
// template <typename T>
// requires(std::is_base_of_v<tmt::IGameComponent, T>)
// struct JsonReflect::Detail::delta_default<T> {
//     static T make() { return T(static_cast<tmt::Entity>(entt::null)); }
// };