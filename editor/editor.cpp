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
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/pipelines/di_pipeline.hpp"
#include "engine/core/scenes.hpp"

#include "editor/imgui/manager.hpp"
#include "editor/core/font_manager.hpp"
#include "engine/tools/profiler.hpp"
#include "engine/tools/file_dialog.hpp"

/* Windows */
#include "editor/windows/hierarchy.hpp"
#include "editor/windows/viewport.hpp"
#include "editor/windows/game_flow.hpp"
#include "editor/windows/inspector.hpp"
#include "editor/windows/goap_debugger.hpp"
#include "editor/windows/goap_action_editor.hpp"
#include "editor/windows/font_control.hpp"
#include "editor/windows/profiler_tracy.hpp"
#include "editor/windows/audio_mixer.hpp"
#include "editor/windows/scenes.hpp"
#include "editor/windows/asset_browser.hpp"
#include "editor/windows/imgui_demo.hpp"
#include "editor/windows/console.hpp"
#include "editor/windows/motion_math.hpp"
#include "editor/windows/debug_lines.hpp"

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
    windows.add<GoapActionEditor>();
    windows.add<FontControl>();
    windows.add<AudioMixer>();
    windows.add<ScenesWindow>();
    windows.add<AssetBrowser>();
    windows.add<Console>();
    windows.add<ImguiDemo>();
    windows.add<MotionMathPreview>();
    windows.add<DebugLines>();

    for (auto& window : windows) {
        window->on_editor_start();
    }
}

void Editor::on_engine_update(const FrameData& time) {
    TMT_ZONE_SCOPED_N("Editor Update");
    {
        TMT_ZONE_SCOPED_N("ImGui New Frame");
        imgui_manager.new_frame();
    }

    main_menu_bar();

    for (auto& window : windows) {
        window->on_editor_update(time);
    }

    auto& open_windows = editor.save_data.open_windows;
    for (auto& window : windows) {
        const auto& name = window->get_title();
        TMT_ZONE_SCOPED_STRING(name);

        const ImGuiWindowFlags_ flags = static_cast<ImGuiWindowFlags_>(window->get_window_flags());
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

    {
        TMT_ZONE_SCOPED_N("ImGui End Frame");
        imgui_manager.end_frame();
    }
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
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Assets...")) {
                windows.get<AssetBrowser>().import_assets_dialog();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("Save Scene")) {
                engine.scenes.serialize_active_scene();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows")) {
            for (const auto& window : windows) {
                const auto& name = window->get_title();
                bool& open = save_data.open_windows[name];
                ImGui::MenuItem(name.c_str(), nullptr, &open);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Renderer")) {
            /* List of display mode labels */
            const char* DISPLAY_MODE_LABELS[] {"Default", "Steps (0..128)", "Visibility", "Depth (0..100)", "Normals", "Albedo", "Illuminance"};

            static uint32_t display_mode_index = 0u;
            const std::string display_mode = DISPLAY_MODE_LABELS[display_mode_index];

            if (ImGui::BeginMenu(("Display Mode (" + display_mode + ")").c_str())) {
                /* Render all display mode options */
                constexpr uint32_t COUNT = sizeof(DISPLAY_MODE_LABELS) / sizeof(char*);
                const char* SELECTED_PREFIX = "* ";
                const char* DEFAULT_PREFIX = "";
                for (uint32_t i = 0u; i < COUNT; ++i) {
                    const bool selected = engine.renderer.display_mode == magic_enum::enum_cast<DisplayMode>(i).value_or(DisplayMode::DEFAULT);
                    const std::string prefix = selected ? SELECTED_PREFIX : DEFAULT_PREFIX;

                    if (ImGui::MenuItem((prefix + DISPLAY_MODE_LABELS[i]).c_str())) {
                        display_mode_index = i;
                        engine.renderer.display_mode = magic_enum::enum_cast<DisplayMode>(i).value_or(DisplayMode::DEFAULT);
                    }
                }
                ImGui::EndMenu();
            }

            /* List of shading rate labels */
            const char* SHADING_RATE_LABELS[] {"Full-Rate", "Half-Rate (1:2)", "Quarter-Rate (1:4)"};

            static uint32_t shading_rate_index = 0u;
            const std::string shading_rate = SHADING_RATE_LABELS[shading_rate_index];

            if (ImGui::BeginMenu(("Shading Rate (" + shading_rate + ")").c_str())) {
                /* Render all shading rate options */
                constexpr uint32_t COUNT = sizeof(SHADING_RATE_LABELS) / sizeof(char*);
                const char* SELECTED_PREFIX = "* ";
                const char* DEFAULT_PREFIX = "";
                for (uint32_t i = 0u; i < COUNT; ++i) {
                    const bool selected = engine.renderer.di_pipeline.get_shading_rate() == magic_enum::enum_cast<ShadingRate>(i).value_or(ShadingRate::FULL_RATE);
                    const std::string prefix = selected ? SELECTED_PREFIX : DEFAULT_PREFIX;

                    if (ImGui::MenuItem((prefix + SHADING_RATE_LABELS[i]).c_str())) {
                        shading_rate_index = i;
                        engine.renderer.di_pipeline.set_shading_rate(magic_enum::enum_cast<ShadingRate>(i).value_or(ShadingRate::FULL_RATE));
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

}  // namespace tmt
