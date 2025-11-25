#pragma once
#include "core/system.hpp"

namespace tmt {

class Animation : public ISystem {
    // Inherited via ISystem
    constexpr virtual std::string get_name() { return "Animation System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_end() override;
};

}  // namespace tmt