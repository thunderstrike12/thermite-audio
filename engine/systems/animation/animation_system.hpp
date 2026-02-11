#pragma once
#include <variant>
#include <cstdint>
#include "engine/core/ecs.hpp"
#include "rig_model.hpp"

namespace tmt {

class RigModelManager : public ISystem {
   public:
    std::string get_name() override;

    void on_start() override;
    void on_update(const FrameData& time) override;
    void inspect(float);
    void on_end() override;
};

}  // namespace tmt
