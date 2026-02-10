#include "component_diff.hpp"
#include "engine/core/components/component_collection.hpp"
#include "engine/tools/serializer/all.hpp"

namespace tmt {

void RuntimeComponentDiff::before() { serialize_component(before_json); }

void RuntimeComponentDiff::after() { serialize_component(after_json); }

void RuntimeComponentDiff::commit(const std::string& component_name) {
    auto diff = nlohmann::json::diff(before_json, after_json);
    if (diff.empty()) return;
    send_to_manager(std::move(*this), "Modified: " + component_name);
}

void RuntimeComponentDiff::undo() { deserialize_component(before_json); }

void RuntimeComponentDiff::redo() { deserialize_component(after_json); }

void RuntimeComponentDiff::inspect() {}

void RuntimeComponentDiff::serialize_component(tmt::json& json_data) const {
    const bool has_collection = engine.ecs.has_component<ComponentCollection>(entity);
    const auto& name = engine.component_registry.get_component_info(component_index).name;
    if (!has_collection) {
        json_data[name] = nullptr;
        return;
    }

    const auto& component_collection = engine.ecs.get_component<ComponentCollection>(entity);
    const bool has_component = component_collection.has_component(component_index);
    if (has_component) {
        const IGameComponent& component = component_collection.get_component(component_index);
        json_data[name] = Serializer::serialize(component);
    } else {
        json_data[name] = nullptr;
    }
}

void RuntimeComponentDiff::deserialize_component(const tmt::json& json_data) const {
    const bool has_collection = engine.ecs.has_component<ComponentCollection>(entity);
    const auto& name = engine.component_registry.get_component_info(component_index).name;
    if (!has_collection) {
        return;
    }

    auto& component_collection = engine.ecs.get_component<ComponentCollection>(entity);
    if (!json_data.contains(name)) return;

    if (json_data[name].is_null()) {
        if (component_collection.has_component(component_index)) {
            component_collection.remove_component(component_index);
        }
    } else {
        IGameComponent& component = component_collection.add_or_get_component(component_index, entity);
        Serializer::deserialize(json_data[name], component);
    }
}

}  // namespace tmt