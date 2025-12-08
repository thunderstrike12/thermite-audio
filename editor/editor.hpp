#pragma once
#include "engine/events/engine.hpp"
#include "editor/windows/hierarchy.hpp"

namespace tmt {

/* Forward declares */
class ImGuiManager;

class Editor : public OnEngineInit, public OnEngineUpdate, public OnEngineFixedUpdate, public OnEngineEnd {
   public:
    Editor();
    ~Editor();

    void init();

   private:
    void on_engine_init(const ApplicationSpecs& specs) override;
    void on_engine_update(const FrameData& time) override;
    void on_engine_fixed_update(const FrameData& time) override;
    void on_engine_end() override;

    ImGuiManager& imgui_manager;
    Hierarchy hierarchy;
};

/* Singleton */
extern Editor editor;

}  // namespace tmt