#pragma once
#include "engine/systems/gameplay/game_component.hpp"

namespace game {
struct RandomPrefabSpawner : tmt::GameComponent<RandomPrefabSpawner> {
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Random Prefab Spawner"; }

    // Inherited via GameComponent
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    std::vector<tmt::ResourceRef<tmt::Json>> prefabs;

    int spawn_limit = 1;
    float respawn_time = 20.0f;
    bool re_randomize = true;

private:
    void spawn_random_prefab();
    void spawn_prefab(const tmt::ResourceRef<tmt::Json>& prefab);
    tmt::Entity spawned_entity = entt::null;
    tmt::ResourceRef<tmt::Json> last_spawned_prefab;
    std::mt19937 rng{ std::random_device{}() };
    int spawn_count = 0;
    float respawn_timer = 20.0f;
};
} // namespace game
TMT_OBJECT(game::RandomPrefabSpawner, (prefabs, spawn_limit, respawn_time, re_randomize));