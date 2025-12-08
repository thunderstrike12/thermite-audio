#pragma once
#include "engine/events/engine.hpp"

#include "editor/core/window.hpp"
#include "engine/core/system_collection.hpp"

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
    SystemCollection<IWindow> systems;
};

/* Singleton */
extern Editor editor;

}  // namespace tmt