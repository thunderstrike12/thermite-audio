#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/voxel_scene.hpp"

namespace game {

class SVHSpawner : public tmt::GameComponent<SVHSpawner> {
    using GameComponent::GameComponent;

   public:
    static std::string_view get_name() { return "SVH spawner"; }

    // Inherited via GameComponent
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    tmt::ResourceRef<tmt::VoxelScene> scene_to_spawn;
    bool needs_distance_culling = true;

   private:
    void init_children(const tmt::Entity parent);
    void add_required(const entt::entity child);
};

}  // namespace game

TMT_GAME_COMPONENT(game::SVHSpawner, (scene_to_spawn, needs_distance_culling));