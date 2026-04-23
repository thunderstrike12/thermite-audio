#pragma once
#include "engine/shared/ray.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"
#include "projects/game/data_headers/layer_mask.hpp"
#include "projects/game/data_headers/ore_properties.hpp"

// custom hasher
namespace std {

template <>
struct hash<glm::uvec3> {
    size_t operator()(const glm::uvec3& v) const {
        size_t h1 = std::hash<uint32_t> {}(v.x);
        size_t h2 = std::hash<uint32_t> {}(v.y);
        size_t h3 = std::hash<uint32_t> {}(v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

template <>
struct hash<std::pair<tmt::Entity, glm::uvec3>> {
    size_t operator()(const std::pair<tmt::Entity, glm::uvec3>& p) const {
        size_t h1 = std::hash<tmt::Entity> {}(p.first);
        size_t h2 = std::hash<glm::uvec3> {}(p.second);
        return h1 ^ (h2 << 1);
    }
};

}  // namespace std

namespace game {

struct ThermiteOreSettings {
    float radius_explosion = 1.0f;
    float damage_explosion = 10.0f;
    float cooldown_explosion = 0.5f;
    // Determines the max toughness of ores that get destroyed by the explosion
    float explosion_strength = 20.0f;
    LayerMask layer_mask;
};

class OreManager : public tmt::GameComponent<OreManager> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Ore Manager"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void initiate_thermite_explosion(tmt::Entity voxel_entity, glm::uvec3 voxel_position);

    ThermiteOreSettings thermite_ore_settings;

   private:
    std::unordered_set<std::pair<tmt::Entity, glm::uvec3>> thermite_to_explode;
    std::unordered_set<std::pair<tmt::Entity, glm::uvec3>> new_thermite_to_explode;
    tmt::Entity ore_properties_entity = entt::null;
    std::unordered_map<tmt::Material::Type, OreProperties::MiningOre> ore_database;
    float thermite_ore_explosion_cooldown_timer = 0.0f;
    void process_thermite_ore_explosion(tmt::Entity voxel_entity, glm::uvec3 explosion_center);
    tmt::Entity player_entity = entt::null;
};

}  // namespace game
TMT_OBJECT(game::ThermiteOreSettings, (radius_explosion, damage_explosion, cooldown_explosion, explosion_strength, layer_mask));
TMT_OBJECT(game::OreManager, (thermite_ore_settings));
