#pragma once
#include "engine/events/engine.hpp"

#include "engine/core/collection.hpp"
#include "editor/core/window.hpp"
#include "editor/shared/save_data.hpp"
#include "editor/gizmo.hpp"

namespace tmt {

/* Forward declares */
class IEditorMode;
class ImGuiManager;
class FontManager;

class Editor : public OnEngineInit, public OnEngineUpdate, public OnEngineFixedUpdate, public OnEngineEnd {
   public:
    enum class Mode : uint8_t {
        SCENE,
        VOXEL,
        PREFAB,
    };

    Editor();
    ~Editor();

    void init();

    void switch_mode(Mode new_mode, const std::any& meta_data = {});

    Mode editor_mode { Mode::SCENE };
    std::map<Mode, std::unique_ptr<IEditorMode>> mode_handlers;
    std::map<Mode, Collection<IWindow>> windows;
    ImGuiManager& imgui_manager;

    SaveData save_data;
    Gizmo gizmo;

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