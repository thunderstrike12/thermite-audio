#pragma once
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class Spawner : public tmt::GameComponent<Spawner> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name();

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    tmt::Entity spawn() const;
    tmt::Entity spawn(const tmt::ResourceRef<tmt::Json>& location, const tmt::Entity& parent = entt::null);
    // core state
    tmt::Entity spawn_parent { entt::null };
    tmt::Entity entity_trigger { entt::null };
    tmt::ResourceRef<tmt::Json> prefab_location;
    // configurable
    bool use_attached_entity_as_spawn_parent = false;
};

}  // namespace game
TMT_OBJECT(game::Spawner, (spawn_parent, prefab_location, use_attached_entity_as_spawn_parent));
