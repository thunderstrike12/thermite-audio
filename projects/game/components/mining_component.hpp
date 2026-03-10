#pragma once
#include "debug_line_helper.hpp"
#include "events.hpp"
#include "ore_properties.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/shared/ray.hpp"

namespace game {

class MiningComponent : public tmt::GameComponent<MiningComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Mining Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void draw_debug_lines() const override;
    void on_weapon_fired(const WeaponFiredEvent& e);

    tmt::ResourceRef<tmt::Stencil> stencil;  // currently active stencil
    bool active = true;
    DebugLineConfig cfg;

   private:
    void on_stop_mining(const ReleaseShootEvent& e);

    struct PreviousAccumulatedHit {
        float mining_time = 0.0f;
        tmt::Material::Type type {};
        glm::uvec3 voxel_coord { std::numeric_limits<uint32_t>::max() };
        bool is_mining = false;
    };
    PreviousAccumulatedHit previous_hit;
    void handle_ore(const tmt::Hit& hit);

    void assign_database();
    std::unordered_map<tmt::Material::Type, OreProperties::MiningOre>* ore_database { nullptr };

    std::vector<tmt::ResourceRef<tmt::Stencil>> stencils;  // might be used in the future when we have different stencils to randomly select from, for now unused
    tmt::Ray last_ray;                                     // for debug lines
    tmt::Hit last_hit;                                     // for debug lines

    bool has_drawn_debug;                                  // for debug lines
    void mine(glm::vec3 origin, glm::vec3 dir);            // base mining function, do not overload if using events, create new function instead
};

}  // namespace game
TMT_OBJECT(game::MiningComponent, (stencil, active, cfg));
