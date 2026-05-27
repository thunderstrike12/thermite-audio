#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/emitter.hpp"
#include "projects/game/data_headers/events.hpp"

namespace game {

class OreCollector : public tmt::GameComponent<OreCollector> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "OreCollector"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void draw_debug_lines() const override;
    void end() override;
    // Shoots rays
    float radius = 1.0f;
    // debugging settings
    tmt::Entity wallet_entity { entt::null };
    tmt::ResourceRef<tmt::Json> ore_collection_vfx_prefab;

    // Attraction variables
    float attraction_range = 20.f;
    float attraction_strength = 100.f;

   private:
    std::unordered_map<tmt::Entity, float> emitter_lifetime_table;
    void spawn_emitter(glm::vec3 spawn_pos);
    uint64_t ore_count = 0u;
    void on_collision_trigger(const TriggerCollisionEvent& trigger);
    float emitter_lifetime = 2.0f;
};

}  // namespace game
TMT_GAME_COMPONENT(game::OreCollector, (radius, wallet_entity, attraction_range, attraction_strength, ore_collection_vfx_prefab));
