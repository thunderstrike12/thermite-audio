#pragma once
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class Wallet : public tmt::GameComponent<Wallet> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Wallet"; }

    void start() override {}
    void update(const tmt::FrameData& time) override {}
    void end() override {}

    // Currencies
    float dollars = 0.0f;

    // Resources
    float gold = 0.0f;
    float silver = 0.0f;
    float copper = 0.0f;
    float iron = 0.0f;
    float enemy_cores = 0.0f;
};

}  // namespace game
TMT_OBJECT(game::Wallet, (dollars, gold, silver, copper, iron, enemy_cores));