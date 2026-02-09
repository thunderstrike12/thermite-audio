#pragma once
#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"

#include "engine/engine.hpp"
#include "engine/core/entity.hpp"
#include "engine/core/ecs.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/core/reflection.hpp"
#include "engine/systems/gameplay/types.hpp"

namespace tmt {

template <typename... T>
    requires(sizeof...(T) > 0)
class ComponentDiff : public IUndoRedo {
   public:
    ComponentDiff(const Entity entity) : entity(entity) {}

    void before() { ((serialize_component<T>(before_json)), ...); }

    void after() { ((serialize_component<T>(after_json)), ...); }

    void commit() {
        auto diff = nlohmann::json::diff(before_json, after_json);
        if (diff.empty()) return;

        const std::string names = ((std::string(Component<T>::get_name()) + ", "), ...);
        send_to_manager(std::move(*this), "Modified: " + names);
    }

    void undo() override { ((deserialize_component<T>(before_json)), ...); }

    void redo() override { ((deserialize_component<T>(after_json)), ...); }

   private:
    const Entity entity = entt::null;

    tmt::json before_json;
    tmt::json after_json;

    template <typename U>
    void serialize_component(tmt::json& json_data) const {
        constexpr auto NAME = Component<U>::get_name();
        const bool has_component = engine.ecs.has_component<U>(entity);
        if (has_component) {
            const U& component = engine.ecs.get_component<U>(entity);
            json_data[Component<U>::get_name()] = Serializer::serialize(component);
        } else {
            json_data[NAME] = nullptr;
        }
    }

    template <typename U>
    void deserialize_component(const tmt::json& json_data) const {
        constexpr auto NAME = Component<U>::get_name();
        if (json_data.contains(NAME)) {
            if (json_data[NAME].is_null()) {
                if (engine.ecs.has_component<U>(entity)) {
                    engine.ecs.remove_component<U>(entity);
                }
            } else {
                if (engine.ecs.has_component<U>(entity) == false) {
                    engine.ecs.add_component<U>(entity);
                }
                U& component = engine.ecs.get_component<U>(entity);
                Serializer::deserialize(json_data[NAME], component);
            }
        }
    }

   protected:
    void inspect() override {}
};

class RuntimeComponentDiff : public IUndoRedo {
   public:
    RuntimeComponentDiff(const Entity entity, const ComponentIndex component_index) : entity(entity), component_index(component_index) {}

    void before();

    void after();

    void commit(const std::string& component_name);

    // Inherited via IUndoRedo
    void undo() override;
    void redo() override;
    void inspect() override;

   private:
    const Entity entity = entt::null;
    const ComponentIndex component_index = NULL_COMPONENT;

    tmt::json before_json;
    tmt::json after_json;

    void serialize_component(tmt::json& json_data) const;

    void deserialize_component(const tmt::json& json_data) const;
};

}  // namespace tmt