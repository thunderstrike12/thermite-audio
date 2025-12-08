#if THERMITE_EDITOR
#pragma message(" THERMITE_EDITOR=1 ")
#else
/* Shouldn't happen */
#error THERMITE_EDITOR must be defined to 1 in editor builds
#endif

#include "editor/editor.hpp"

#include "engine/engine.hpp"

#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/window.hpp"

#include "editor/imgui/manager.hpp"

/* Singleton */
tmt::Editor tmt::editor;

namespace tmt {

Editor::Editor() : imgui_manager(*new ImGuiManager()) {}

Editor::~Editor() { delete &imgui_manager; }

void Editor::init() { Log::info("Thermite Editor initialized."); }

void Editor::on_engine_init(const ApplicationSpecs&) {
    tmt::Log::info("Starting Thermite Editor...");
    imgui_manager.init();
}

void Editor::on_engine_update(const FrameData&) {
    imgui_manager.new_frame();

    ImGui::Begin("Camera");
    ImGui::End();

    imgui_manager.end_frame();
}

void Editor::on_engine_fixed_update(const FrameData&) {}

void Editor::on_engine_end() {
    tmt::Log::info("Shutting down Thermite Editor...");
    imgui_manager.deinit();
}

}  // namespace tmt