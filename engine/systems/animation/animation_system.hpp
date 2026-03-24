#pragma once

#include <cstdint>
#include "engine/core/ecs.hpp"
#include "engine/systems/animation/rig_model.hpp"
#include "engine/systems/animation/components/constraints.hpp"
#include "engine/events/game.hpp"
#include "engine/events/debug.hpp"

namespace tmt {

class RigModelManager : public ISystem, OnDrawLines {
   public:
    // Inherited from base class ISystem
    constexpr std::string get_name() override { return "animation system"; }
    // Inherited from base class OnDrawLines
    constexpr std::string get_name() const override { return "RigModelManager"; }

    void on_start() override {}
    void on_update(const FrameData& time) override;
    void on_end() override {}

    // Inherited via OnDrawLines
    void on_draw_lines() const override;
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
