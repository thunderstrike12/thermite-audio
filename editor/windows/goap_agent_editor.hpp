#pragma once
#include "editor/core/window.hpp"
#include <engine/systems/ai/goap/components/goap_agent_type_registry.hpp>

namespace tmt {

/**
 * Class GoapAgentEditor
 * Editor window for creating agent types.
 *
 * This window allows the creation of agent types:
 *  - Adding a new agent type
 *  - Adding or removing actions to the types
 *  - Adding or removing goals to the types
 *  - Adding or removing worls facts
 *
 */
class GoapAgentEditor : public IWindow<> {
   public:
    constexpr std::string get_title() const override { return "GOAP Agents"; }

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override;

    void on_inspect() override;

   private:
    void draw_agent_type_node(GoapAgentType& type);

    void draw_actions_section(GoapAgentType& type);
    void draw_goals_section(GoapAgentType& type);
    void draw_world_state_section(GoapAgentType& type);
};

}  // namespace tmt
