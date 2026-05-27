#include "ore_collector.hpp"

#include "projects/game/components/development_tools/collision_trigger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"
#include "engine/tools/player_data.hpp"
#include "engine/tools/prefab_helper.hpp"
#include "projects/game/data_headers/save_entries.hpp"
#include "projects/game/data_headers/wallet.hpp"

namespace game {

void game::OreCollector::start() {
    tmt::engine.ecs.get_dispatcher().sink<TriggerCollisionEvent>().connect<&OreCollector::on_collision_trigger>(this);
    if (wallet_entity == entt::null) {
        wallet_entity = entity;
    }
}
void game::OreCollector::update(const tmt::FrameData& time) {
    // explosion vfx handling
    std::unordered_set<tmt::Entity> destroyed_emitters;
    for (std::pair<const tmt::Entity, float>& emitter_entity : emitter_lifetime_table) {
        if (emitter_entity.second < 0.0f) {
            tmt::engine.ecs.destroy_entity(emitter_entity.first);
            destroyed_emitters.insert(emitter_entity.first);
        } else {
            emitter_entity.second -= time.delta_time;
        }
    }
    for (auto emitter_entity : destroyed_emitters) {
        emitter_lifetime_table.erase(emitter_entity);
    }

    auto& collector_transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    auto barge_entity = collector_transform.get_parent();
    if (!tmt::engine.ecs.valid(barge_entity)) return;

    auto& barge_transform = tmt::engine.ecs.get_component<tmt::Transform>(barge_entity);

    glm::vec3 barge_pos = barge_transform.get_world_position();

    float dt = time.delta_time;

    for (auto&& [physics_entity, body, transform] : tmt::engine.ecs.view<tmt::VoxelBody, tmt::Transform>().each()) {
        if (physics_entity == entity) continue;
        if (body.type != tmt::VoxelBody::DYNAMIC) continue;

        glm::vec3 pos = transform.get_world_position();
        glm::vec3 to_barge = barge_pos - pos;

        float dist = glm::length(to_barge);
        if (dist < 0.001f || dist > attraction_range) continue;

        glm::vec3 dir = to_barge / dist;

        // stop the bodies
        const float capture_radius = 1.5f;
        if (dist < capture_radius) {
            body.velocity = glm::vec3(0.0f);
            transform.set_world_position(barge_pos);
            continue;
        }

        // falloff
        float t = 1.0f - (dist / attraction_range);
        t = t * t;

        // addative attraction only
        float accel = attraction_strength * t;

        body.velocity += dir * accel * dt;

        // damping
        body.velocity *= 0.985f;
    }
}

void game::OreCollector::draw_debug_lines() const {
    auto& collector_transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    auto barge = collector_transform.get_parent();
    auto barge_transform = tmt::engine.ecs.get_component<tmt::Transform>(barge);
    glm::vec3 barge_pos = barge_transform.get_world_position();

    tmt::engine.polyline.use_color(1.0f, 0.0f, 0.0f);
    tmt::engine.polyline.use_line_width(0.5f);
    tmt::engine.polyline.draw_sphere(barge_pos, attraction_range);
}

void game::OreCollector::end() {
    tmt::engine.ecs.get_dispatcher().sink<TriggerCollisionEvent>().disconnect<&OreCollector::on_collision_trigger>(this);
}

void game::OreCollector::on_collision_trigger(const TriggerCollisionEvent& trigger) {
    // TODO do something smarter maybe, check type of ore etc.
    if (trigger.trigger != entity) {
        return;
    }
    if (!tmt::engine.ecs.valid(trigger.other_object)) {
        return;
    }

    auto* vr = tmt::engine.ecs.try_get_component<tmt::VoxelRenderer>(trigger.other_object);
    if (vr == nullptr) {
        tmt::Log::error("Entity has no voxel renderer and is collected!", trigger.other_object);
        tmt::engine.ecs.destroy_entity(trigger.other_object);

        return;
    }

    auto* voxel_tree = vr->resource.resource.get();
    if (voxel_tree == nullptr) {
        tmt::Log::error("Entity has no voxel body and is collected!", trigger.other_object);
        tmt::engine.ecs.destroy_entity(trigger.other_object);

        return;
    }
    auto& blas = voxel_tree->blas;
    // deframent, danger it might be very slow
    voxel_tree->blas->defrag();
    std::array<uint64_t, magic_enum::enum_count<tmt::Material::Type>()> type_counts {};
    type_counts.fill(0);

    for (uint32_t i = 0; i < blas->node_count; ++i) {
        // search for actual voxels only
        if (!blas->nodes[i].is_leaf()) continue;

        // get the actual voxel material ptr
        const uint32_t ptr = blas->nodes[i].abs_ptr();
        // just how many materials there are in that memory
        const uint32_t count = __popcnt64(blas->nodes[i].child_mask);

        for (uint32_t j = 0; j < count; ++j) {
            auto type = blas->palette.entries[blas->materials[ptr + j]].type;
            type_counts[static_cast<uint8_t>(type)]++;
        }
    }
    for (auto type : magic_enum::enum_values<tmt::Material::Type>()) {
        auto count = type_counts[static_cast<uint8_t>(type)];
        tmt::Log::info("Ore count is {}, for {}", count, magic_enum::enum_name(type));
    }

    // vfx trigger before deletion
    spawn_emitter(tmt::engine.ecs.try_get_component<tmt::Transform>(trigger.other_object)->get_world_position());

    tmt::engine.ecs.destroy_entity(trigger.other_object);

    // update the wallet
    auto* wallet = tmt::engine.ecs.try_get_component<Wallet>(wallet_entity);
    if (wallet == nullptr) {
        tmt::Log::error("No Wallet component found");

        return;
    }
    auto ore_view = tmt::engine.ecs.view<OreProperties>();
    if (ore_view.begin() == ore_view.end()) {
        tmt::Log::error("No OreProperties found");
        return;
    }

    auto& ore_properties = ore_view.front().get<OreProperties>();
    for (auto& [mat_type, ore] : ore_properties.ores) {
        auto count = type_counts[static_cast<uint8_t>(mat_type)];
        if (count == 0) continue;

        wallet->currencies.resource_counts[ore.ore_resource] += count * ore.resource_per_voxel;
    }
}

void OreCollector::spawn_emitter(glm::vec3 spawn_pos) {
    if (emitter_lifetime <= 0.0f) {
        return;
    }

    tmt::Entity instantiated_entity = tmt::PrefabHelper::instantiate_prefab(ore_collection_vfx_prefab.file_location);
    if (instantiated_entity == entt::null) return;
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(instantiated_entity);

    transform.set_world_position(spawn_pos);

    tmt::ParticleEmitter* emitter_component = tmt::engine.ecs.try_get_component<tmt::ParticleEmitter>(instantiated_entity);
    if (!emitter_component) {
        tmt::Log::error("Cant spawn particle on emitter, check prefab on ore collector component, on entity: {}", entity);
        return;
    }

    emitter_component->active = false;
    emitter_component->should_burst = true;

    emitter_lifetime_table.emplace(instantiated_entity, emitter_lifetime);
}

}  // namespace game
