#pragma once
#include <optional>
#include <string>
#include "engine/tools/json.hpp"
#include "engine/core/entity.hpp"

namespace tmt {

class SceneJson {
   private:
    json json_value;

   public:
    SceneJson() = default;
    SceneJson(json j) : json_value(std::move(j)) {}

    const json& get_json() const { return json_value; }
    std::string dump() const;

    /* version */
    bool has_version() const;
    uint64_t get_version() const;
    void set_version(uint64_t version);

    /* entities */
    bool has_entities() const;
    json& entities();
    const json& entities() const;

    /* components */
    bool has_components(const std::string& component_name) const;
    json& components(const std::string& component_name);
    const json& components(const std::string& component_name) const;

    bool has_component_entry(const std::string& component_name, const tmt::Entity entity_id) const;
    /* Copy */
    std::optional<json> component_value(const std::string& component_name, const tmt::Entity entity_id) const;
    /* entity entry */
    bool has_entity_entry(const json& container, const tmt::Entity entity_id) const;
    void add_entity_entry(json& container, const tmt::Entity entity_id, const json& value) const;
    const json& get_entity_entry(const json& container, const tmt::Entity entity_id) const;
};

}  // namespace tmt