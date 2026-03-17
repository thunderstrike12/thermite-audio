#pragma once
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

struct RandomPrefabSpawner : public tmt::GameComponent<RandomPrefabSpawner> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Random Prefab Spawner"; }

    // Inherited via GameComponent
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    std::vector<tmt::ResourceRef<tmt::Json>> prefabs;

   private:
    void spawn_prefab(const tmt::ResourceRef<tmt::Json>& prefab) const;
};

}  // namespace game
TMT_OBJECT(game::RandomPrefabSpawner, (prefabs));
