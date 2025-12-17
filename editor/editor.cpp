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
#include "engine/core/window.hpp"

#include "editor/imgui/manager.hpp"
#include "editor/core/font_manager.hpp"

/* Windows */
#include "editor/windows/hierarchy.hpp"
#include "editor/windows/viewport.hpp"
#include "editor/windows/game_flow.hpp"
#include "editor/windows/inspector.hpp"
#include "editor/windows/goap_debugger.hpp"
#include "editor/windows/font_control.hpp"
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
    save_data.load();

    windows.add<Hierarchy>();
    windows.add<GameFlow>();
    windows.add<Inspector>();
    windows.add<Viewport>();
    windows.add<Profiler>();
    windows.add<GoapDebugger>();
    windows.add<FontControl>();

    for (auto& system : windows) {
        system->on_editor_start();
    }
}

void Editor::on_engine_update(const FrameData& time) {
    imgui_manager.new_frame();

    main_menu_bar();

    for (auto& system : windows) {
        system->on_editor_update(time);
    }

    for (auto& system : windows) {
        const ImGuiWindowFlags_ flags = static_cast<ImGuiWindowFlags_>(system->get_window_flags());
        const auto& name = system->get_title();
        if (editor.save_data.open_windows.contains(name) == false) {
            editor.save_data.open_windows[name] = true;
        }
        bool& open = editor.save_data.open_windows[name];

        if (open) {
            system->before_begin();
            ImGui::Begin(name.c_str(), system->is_closable() ? &open : nullptr, flags);
            system->display();
            ImGui::End();
            system->end_display();
        }
    }

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
    save_data.save();
}

void Editor::main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            for (const auto& system : windows) {
                const auto& name = system->get_title();
                bool& open = save_data.open_windows[name];
                ImGui::MenuItem(name.c_str(), nullptr, &open);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

}  // namespace tmt
