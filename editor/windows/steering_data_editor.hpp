#pragma once
#include "editor/core/window.hpp"
#include "engine/systems/ai/steering/components/steering_params_overrides.hpp"

namespace tmt {

/**
 * Class SteeringDataEditor
 * Editor window for inspecting and editing steering agent paramater overrides.
 *
 * This window allows inspecting the steeering data:
 *  - changing paramaters of the steering agent
 *
 * Integrates with SteeringParamsOverrides.
 */
class SteeringDataEditor : public IWindow {
   public:
    constexpr std::string get_title() const override { return "Steering data"; }

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override;

    void on_inspect() override;
};

}  // namespace tmt
