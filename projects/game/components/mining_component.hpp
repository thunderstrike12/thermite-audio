#pragma once
#include "debug_line_helper.hpp"
#include "events.hpp"
#include "ore_properties.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/shared/ray.hpp"

namespace game {

struct VoxelID {
    tmt::Entity entity_id;
    uint32_t packed_coord;

    // DANGER
    VoxelID(tmt::Entity entityID, const glm::uvec3& coord) : entity_id { entityID }, packed_coord { (coord.x << 20) | (coord.y << 10) | coord.z } {
        assert(coord.x <= 1023 && coord.y <= 1023 && coord.z <= 1023);
    }
    glm::uvec3 unpack_coord() const { return { (packed_coord >> 20) & 0x3FF, (packed_coord >> 10) & 0x3FF, packed_coord & 0x3FF }; }
    bool operator==(const VoxelID&) const = default;
};

struct VoxelIDHash {
    std::size_t operator()(const VoxelID& v) const {
        uint64_t combined = (static_cast<uint64_t>(v.entity_id) << 32) | v.packed_coord;
        return std::hash<uint64_t> {}(combined);
    }
};
struct RayCylinder {
    // relative to the transform
    tmt::Entity base_transform_entity = entt::null;
    float base_radius = 0.20f;
    float ray_distance = 2.0f;
    uint32_t ray_amount = 20;
    float rotating_speed = 0.0f;
};
class MiningComponent : public tmt::GameComponent<MiningComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "Mining Component"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void draw_debug_lines() const override;
    void on_weapon_fired(const WeaponFiredEvent& e);

    bool active = true;
    DebugLineConfig cfg {};
    RayCylinder ray_cylinder {};
    // TODO replace with proper mask
    uint32_t ray_mask = 1 << 0;
    float time_draw_rays_in_debug = 2.0f;

   private:
    tmt::Transform* get_transform() const;
    void on_stop_mining(const ReleaseShootEvent& e);
    void handle_voxel(const VoxelID& voxel_id);
    std::vector<glm::vec3> compute_ray_origins(const tmt::Transform& transform) const;
    void assign_database();

    std::unordered_map<tmt::Material::Type, OreProperties::MiningOre>* ore_database { nullptr };

    std::vector<glm::vec3> previous_computed_origins;
    bool has_drawn_debug;             // for debug lines
    void mine(const glm::vec3& dir);  // base mining function, do not overload if using events, create new function instead

    using TimeStamp = float;
    using VoxelMap = std::unordered_map<VoxelID, TimeStamp, VoxelIDHash>;
    // does not require the < operator
    using UniqueVoxelsSet = std::unordered_set<VoxelID, VoxelIDHash>;
    VoxelMap mining_voxels {};
    bool stopped_mining = false;
};

}  // namespace game
TMT_OBJECT(game::RayCylinder, (base_transform_entity, base_radius, ray_distance, ray_amount, rotating_speed));
TMT_OBJECT(game::MiningComponent, (active, cfg, ray_cylinder, ray_mask, time_draw_rays_in_debug));
