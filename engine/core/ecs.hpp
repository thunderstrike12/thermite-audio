#pragma once
#include <memory>
#include <stdexcept>
#include <vector>

#include <entt/entt.hpp>

#include "frame_data.hpp"
#include "system.hpp"

#include "engine/events/game.hpp"
#include "engine/events/engine.hpp"

namespace tmt {

using Registry = entt::registry;
using Entity = entt::entity;

struct Delete {};

template <typename T>
concept System = std::is_base_of_v<ISystem, T>;

class Ecs : public OnStart, public OnUpdate, public OnFixedUpdate, public OnEnd, public OnEndFrame {
   public:
    Ecs() = default;
    ~Ecs() = default;

    /* Systems */
    template <System T, typename... Args>
    T& register_system(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;

        systems.push_back(std::move(system));
        return ref;
    }

    template <System T>
    T& get_system() {
        for (auto& system : systems) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return *casted;
            }
        }
        throw std::runtime_error("System not found");
    }

    template <System T>
    T* try_get_system() {
        for (auto& system : systems) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    /* Entities / Components */
    Registry& get_registry() { return registry; }

    Entity create_entity(const Entity hint = entt::null) { return registry.create(hint); }

    template <typename... Component>
    Entity create_entity(const Entity hint = entt::null) {
        Entity entity = registry.create(hint);
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

    /* Single + Multiple get */
    template <typename... Component>
    decltype(auto) get_component(const Entity entity) {
        constexpr size_t COUNT = sizeof...(Component);
        if constexpr (COUNT == 1) {
            return registry.get<Component...>(entity);
        } else {
            return std::make_tuple(registry.get<Component>(entity)...);
        }
    }

    /* Single + Multiple try_get */
    template <typename... Component>
    decltype(auto) try_get_component(const Entity entity) {
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
        if (force) {
            registry.destroy(entity);
        } else {
            registry.emplace<Delete>(entity);
        }
    }

    void clear() { registry.clear(); }

   private:
    std::vector<std::unique_ptr<ISystem>> systems;

    Registry registry;

    void on_start() override;

    void on_update(const FrameData& time) override;

    void on_fixed_update(const FrameData& time) override;

    void on_end() override;

    void on_end_frame() override;
};

}  // namespace tmt