#pragma once
#include <variant>
#include <cstdint>
#include "engine/core/ecs.hpp"
#include "rig_model.hpp"
#include "engine/systems/animation/components/constraints.hpp"
#include "engine/events/game.hpp"
#include "engine/events/debug.hpp"

namespace tmt {

class RigModelManager : public ISystem, OnGameStart, OnDrawLines {
   public:
    std::string get_name() override;

    void on_game_start() override;
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_end() override;

    // Inherited via OnDrawLines
    void on_draw_lines() const override;
    constexpr std::string get_name() const override { return "RigModelManager"; };
};

class AnimationConstraintSystem : public ISystem {
   public:
    // Inherited via ISystem
    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void on_end() override;
    std::string get_name() override;

   private:
};

}  // namespace tmt
