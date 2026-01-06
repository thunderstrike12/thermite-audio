#pragma once
#include "editor/core/window.hpp"
#include "engine/systems/ai/goap/components/goap_agent.hpp"
#include "engine/systems/ai/goap/components/world_state.hpp"

namespace tmt {

/**
 * Class GoapDebugger
 * Editor window for visualizing and debugging GOAP agents.
 *
 * This window allows inspecting the internal state of GOAP agents:
 *  - See currently active goals and their priority.
 *  - Examine the agent's current plan and its sequence of actions.
 *  - Inspect the world state and check which preconditions/effects are satisfied.
 *  - Display a visual representation of the agent's planning graph.
 *
 * It runs inside the editor's windowing system and updates per frame.
 */
class GoapDebugger : public IWindow {
   public:
    constexpr std::string get_title() const override { return "GOAP Debugger"; }

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override;

    void display() override;

   private:
    // Draws a detailed view of a single GOAP agent.
    void draw_details_view(GoapAgent& agent, WorldState& ws);
    // Draws a visual representation of the agent's planning graph.
    void draw_goap_graph(GoapAgent& agent, WorldState& ws);
};

}  // namespace tmt
