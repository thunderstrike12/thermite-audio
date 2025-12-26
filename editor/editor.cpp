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
#include "editor/windows/audio_mixer.hpp"
#include "editor/windows/profiler_tracy.hpp"
#include "editor/windows/scenes.hpp"
#include "editor/windows/imgui_demo.hpp"
#include "editor/windows/console.hpp"

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
    windows.add<AudioMixer>();
    windows.add<ScenesWindow>();
    windows.add<Console>();
    windows.add<ImguiDemo>();

    for (auto& window : windows) {
        window->on_editor_start();
    }
}

void Editor::on_engine_update(const FrameData& time) {
    imgui_manager.new_frame();

    main_menu_bar();

    for (auto& window : windows) {
        window->on_editor_update(time);
    }

    auto& open_windows = editor.save_data.open_windows;
    for (auto& window : windows) {
        const ImGuiWindowFlags_ flags = static_cast<ImGuiWindowFlags_>(window->get_window_flags());
        const auto& name = window->get_title();
        if (open_windows.contains(name) == false) {
            open_windows[name] = window->default_open();
        }
        bool& open = open_windows[name];

        if (open) {
            window->before_begin();
            ImGui::Begin(name.c_str(), window->is_closable() ? &open : nullptr, flags);
            window->display();
            ImGui::End();
            window->end_display();
        }
    }

    imgui_manager.end_frame();
}

void Editor::on_engine_fixed_update(const FrameData& time) {
    for (auto& window : windows) {
        window->on_editor_fixed_update(time);
    }
}

void Editor::on_engine_end() {
    for (auto& window : windows) {
        window->on_editor_end();
    }

    tmt::Log::info("Shutting down Thermite Editor...");
    imgui_manager.deinit();
    save_data.save();
}

void Editor::main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            for (const auto& window : windows) {
                const auto& name = window->get_title();
                bool& open = save_data.open_windows[name];
                ImGui::MenuItem(name.c_str(), nullptr, &open);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

}  // namespace tmt
