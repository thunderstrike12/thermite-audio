#pragma once
#include "editor/core/window.hpp"
#include "engine/systems/ai/goap/components/goap_action_overrides.hpp"

namespace tmt {

/**
 * Class GoapActionEditor
 * Editor window for inspecting and editing GOAP action overrides.
 *
 * This window allows inspecting the actions:
 *  - Selecting an action from the registry
 *  - Editing override cost
 *  - Editing preconditions and effects
 *  - Adding or removing individual facts
 *  - Resetting cost, preconditions, or effects to defaults
 *
 * Integrates with GoapActionOverrides.
 */
class GoapActionEditor : public IWindow {
   public:
    constexpr std::string get_title() const override { return "GOAP Actions"; }

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override;

    void display() override;
};

}  // namespace tmt
