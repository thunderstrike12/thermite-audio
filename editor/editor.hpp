#pragma once
#include "engine/events/engine.hpp"

#include "editor/core/window.hpp"
#include "engine/core/collection.hpp"
#include "editor/shared/save_data.hpp"

namespace tmt {

/* Forward declares */
class ImGuiManager;
class FontManager;

class Editor : public OnEngineInit, public OnEngineUpdate, public OnEngineFixedUpdate, public OnEngineEnd {
   public:
    Editor();
    ~Editor();

    void init();

    Collection<IWindow> windows;

    ImGuiManager& imgui_manager;

    SaveData save_data;

   private:
    void on_engine_init(const ApplicationSpecs& specs) override;
    void on_engine_update(const FrameData& time) override;
    void on_engine_fixed_update(const FrameData& time) override;
    void on_engine_end() override;

    void main_menu_bar();
};

/* Singleton */
extern Editor editor;

}  // namespace tmt