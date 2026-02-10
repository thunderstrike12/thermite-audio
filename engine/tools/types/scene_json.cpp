#include "scene_json.hpp"
#include "engine/core/logger.hpp"

namespace tmt {

std::string SceneJson::dump() const { return json_value.dump(4); }

bool SceneJson::has_version() const { return json_value.contains("version"); }

uint64_t SceneJson::get_version() const {
    if (json_value.contains("version")) {
        return json_value.at("version").get<uint64_t>();
    }
    Log::warn(Log::Scope::ENGINE, "[Serialization] Json does not contain \"version\" number. Json:\n---\n%s\n---", json_value.dump());
    return 0;
}

void SceneJson::set_version(uint64_t version) { json_value["version"] = version; }

bool SceneJson::has_entities() const { return json_value.contains("entities"); }

json& SceneJson::entities() { return json_value["entities"]; }

const json& SceneJson::entities() const { return json_value.at("entities"); }

bool SceneJson::has_components(const std::string& component_name) const { return json_value.contains(component_name); }

json& SceneJson::components(const std::string& component_name) { return json_value[component_name]; }

const json& SceneJson::components(const std::string& component_name) const { return json_value.at(component_name); }

bool SceneJson::has_component_entry(const std::string& component_name, const Entity entity_id) const {
    if (has_components(component_name) == false) return false;

    const json& component_container = components(component_name);

    return has_entity_entry(component_container, entity_id);
}

std::optional<json> SceneJson::component_value(const std::string& component_name, const Entity entity_id) const {
    if (has_components(component_name) == false) return std::nullopt;

    const json& component_container = components(component_name);
    if (has_entity_entry(component_container, entity_id) == false) return std::nullopt;

    return std::optional<json> {get_entity_entry(component_container, entity_id)};
}

bool SceneJson::has_entity_entry(const json& container, const Entity entity_id) const {
    const std::string key = EntityHelper::to_string(entity_id);
    return container.contains(key);
}

void SceneJson::add_entity_entry(json& container, const Entity entity_id, const json& value) const {
    const std::string key = EntityHelper::to_string(entity_id);
    container[key] = value;
}

const json& SceneJson::get_entity_entry(const json& container, const Entity entity_id) const {
    const std::string key = EntityHelper::to_string(entity_id);
    return container.at(key);
}

}  // namespace tmt