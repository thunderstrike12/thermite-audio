#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/entity.hpp"
#include "medium_enemy.hpp"
#include "../../../editor/all.hpp"

namespace game {

struct Missile : public tmt::GameComponent<Missile> {
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Missile"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void explode();
    void empty_check();

    glm::vec3 velocity = glm::vec3(0, 0, 0);

    tmt::ResourceRef<tmt::Stencil> stencil;
    tmt::Entity enemy_entity;

    float life_time = 0.0f;
    glm::vec3 offset = glm::vec3(0, 0, 0);
    float damage = 10.0f;
};

}  // namespace game
TMT_GAME_COMPONENT_EMPTY(game::Missile);
