#pragma once
#include "engine/core/system.hpp"

namespace tmt {

class Gameplay : public ISystem {
   public:
    constexpr virtual std::string get_name() override { return "Gameplay"; };

    void on_start() override;

    void on_update(const tmt::FrameData& time) override;

    void on_end() override;

    void on_fixed_update(const tmt::FrameData&) override;
};

}  // namespace tmt