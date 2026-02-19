#pragma once
#include <map>
#include <string>

#include "engine/core/entity.hpp"
#include "engine/systems/gameplay/types.hpp"

namespace tmt {

/* Forward declare */
class IGameComponent;

class GameComponentRegistry {
   public:
    GameComponentRegistry() = default;
    ~GameComponentRegistry() = default;

    template <typename T>
    requires std::is_base_of_v<IGameComponent, T>
    void register_component() {
        ComponentInfo info {
            .name = std::string(T::name()),
            .type_id = typeid(T),
        };
        info.factory.register_type<T, Entity>();
        name_to_index.emplace(info.name, info.type_id);
        registered_components.emplace(info.type_id, std::move(info));
    }

    ComponentInfo& get_component_info(const ComponentIndex& type_id) {
        if (registered_components.contains(type_id) == false) {
            throw std::runtime_error("[GameComponentRegistry] get_component_info: Component not registered");
        }
        return registered_components.at(type_id);
    }

    ComponentIndex get_component_index(const std::string& name) {
        if (name_to_index.contains(name) == false) {
            throw std::runtime_error("[GameComponentRegistry] get_component_index: Component name not registered");
        }
        return name_to_index.at(name);
    }

    bool is_component_registered(const std::string& name) const { return name_to_index.contains(name); }

    bool is_component_registered(const ComponentIndex& type_id) const { return registered_components.contains(type_id); }

    ComponentInfo& get_component_info(const std::string& name) {
        ComponentIndex type_id = get_component_index(name);
        return get_component_info(type_id);
    }

    const std::map<ComponentIndex, ComponentInfo>& get_registered_components() const { return registered_components; }

    static void is_registered_or_throw(const ComponentIndex& type_id);

   private:
    /* Needs to be ordered for serialization */
    std::map<ComponentIndex, ComponentInfo> registered_components;
    std::map<std::string, ComponentIndex> name_to_index;
};

}  // namespace tmt