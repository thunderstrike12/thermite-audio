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

#include "editor/windows/hierarchy.hpp"
#include "editor/windows/game_flow.hpp"

#include "windows/profiler_tracy.hpp"

/* Singleton */
tmt::Editor tmt::editor;

namespace tmt {

Editor::Editor() : imgui_manager(*new ImGuiManager()) {}

Editor::~Editor() { delete &imgui_manager; }

void Editor::init() { Log::info("Thermite Editor initialized."); }

void Editor::on_engine_init(const ApplicationSpecs&) {
    tmt::Log::info("Starting Thermite Editor...");
    imgui_manager.init();

    windows.add<Hierarchy>();
    windows.add<GameFlow>();
    windows.add<Profiler>();

    for (auto& system : windows) {
        system->on_editor_start();
    }
}

void Editor::on_engine_update(const FrameData& time) {
    imgui_manager.new_frame();

    for (auto& system : windows) {
        system->on_editor_update(time);
    }

    for (auto& system : windows) {
        ImGui::Begin(system->get_title().c_str());
        system->display();
        ImGui::End();
    }

    ImGui::Begin("Viewport");

    auto size = ImGui::GetContentRegionAvail();

    engine.renderer.set_viewport_size(size.x, size.y);

    ImGui::Image((ImTextureRef)engine.renderer.get_imgui_viewport(), size);

    ImGui::End();

    imgui_manager.end_frame();
}

void Editor::on_engine_fixed_update(const FrameData& time) {
    for (auto& system : windows) {
        system->on_editor_fixed_update(time);
    }
}

void Editor::on_engine_end() {
    for (auto& system : windows) {
        system->on_editor_end();
    }

    tmt::Log::info("Shutting down Thermite Editor...");
    imgui_manager.deinit();
}

}  // namespace tmt
