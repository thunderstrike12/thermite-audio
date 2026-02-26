#include "windows/build_packager.hpp"
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
#include "engine/core/renderer/pipelines/di_pipeline.hpp"
#include "engine/core/renderer/pipelines/ui_pipeline.hpp"
#include "engine/core/scenes.hpp"

#include "editor/imgui/manager.hpp"
#include "editor/core/systems/font_manager.hpp"
#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"
#include "engine/tools/profiler.hpp"
#include "engine/tools/file_dialog.hpp"
#include "editor/gizmo.hpp"

/* Modes */
#include "editor/modes/scene.hpp"
#include "editor/modes/voxel.hpp"
#include "editor/modes/prefab.hpp"

/* Windows */
#include "editor/windows/hierarchy.hpp"
#include "editor/windows/viewport.hpp"
#include "editor/windows/game_flow.hpp"
#include "editor/windows/inspector.hpp"
#include "editor/windows/goap_debugger.hpp"
#include "editor/windows/goap_action_editor.hpp"
#include "editor/windows/goap_agent_editor.hpp"
#include "editor/windows/font_control.hpp"
#include "editor/windows/profiler_tracy.hpp"
#include "editor/windows/audio_mixer.hpp"
#include "editor/windows/scenes.hpp"
#include "editor/windows/asset_browser.hpp"
#include "editor/windows/imgui_demo.hpp"
#include "editor/windows/console.hpp"
#include "editor/windows/motion_math.hpp"
#include "editor/windows/debug_lines.hpp"
#include "editor/windows/ecs.hpp"
#include "editor/windows/rendering.hpp"
#include "editor/windows/model_viewer.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "editor/windows/palette.hpp"
#include "editor/windows/material_editor.hpp"
#include "editor/windows/brush.hpp"
#include "editor/windows/editor_settings.hpp"
#include "editor/windows/ui.hpp"

/* Singleton */
tmt::Editor tmt::editor;

namespace tmt {

Editor::Editor() : imgui_manager(*new ImGuiManager()) {}

Editor::~Editor() {
    delete &imgui_manager;
}

void Editor::init() {
    Log::info("Thermite Editor initialized.");
}

void Editor::switch_mode(Mode new_mode, const std::any& meta_data) {
    // if (editor_mode == new_mode) return;

    mode_handlers[editor_mode]->on_switch_away();
    editor_mode = new_mode;
    mode_handlers[editor_mode]->on_switch_to(meta_data);
}

void Editor::on_engine_init(const ApplicationSpecs&) {
    tmt::Log::info("Starting Thermite Editor...");
    imgui_manager.init();
    save_data.load();
    gizmo.init();

    mode_handlers[Mode::SCENE] = std::make_unique<SceneMode>();
    mode_handlers[Mode::VOXEL] = std::make_unique<VoxelMode>();
    mode_handlers[Mode::PREFAB] = std::make_unique<PrefabMode>();

    windows[Mode::SCENE].add<Hierarchy>();
    windows[Mode::SCENE].add<GameFlow>();
    windows[Mode::SCENE].add<Inspector>();
    windows[Mode::SCENE].add<Viewport>();
    windows[Mode::SCENE].add<Profiler>();
    windows[Mode::SCENE].add<GoapDebugger>();
    windows[Mode::SCENE].add<GoapActionEditor>();
    windows[Mode::SCENE].add<GoapAgentEditor>();
    windows[Mode::SCENE].add<FontControl>();
    windows[Mode::SCENE].add<AudioMixer>();
    windows[Mode::SCENE].add<ScenesWindow>();
    windows[Mode::SCENE].add<AssetBrowser>();
    windows[Mode::SCENE].add<Console>();
    windows[Mode::SCENE].add<ImguiDemo>();
    windows[Mode::SCENE].add<MotionMathPreview>();
    windows[Mode::SCENE].add<DebugLines>();
    windows[Mode::SCENE].add<EcsInspector>();
    windows[Mode::SCENE].add<Rendering>();
    windows[Mode::SCENE].add<UndoRedoManager>();
    windows[Mode::SCENE].add<EditorSettingsWindow>();
    windows[Mode::SCENE].add<BuildPackager>();
    windows[Mode::SCENE].add<UIEditor>();

    engine.scenes.register_scene<VoxelEditScene>();
    windows[Mode::VOXEL].add<ModelViewer>();
    windows[Mode::VOXEL].add<NodeHierarchy>();
    windows[Mode::VOXEL].add<Palette>();
    windows[Mode::VOXEL].add<MaterialEditor>();
    windows[Mode::VOXEL].add<Brush>();
    windows[Mode::VOXEL].add<Console>();
    windows[Mode::VOXEL].add<UndoRedoManager>();

    engine.scenes.register_scene<PrefabEditScene>();
    windows[Mode::PREFAB].add<Hierarchy>();
    windows[Mode::PREFAB].add<Inspector>();
    windows[Mode::PREFAB].add<Viewport>();
    windows[Mode::PREFAB].add<AssetBrowser>();
    windows[Mode::PREFAB].add<Console>();
    windows[Mode::PREFAB].add<ScenesWindow>();
    windows[Mode::PREFAB].add<UndoRedoManager>();
    windows[Mode::PREFAB].add<UIEditor>();

    for (auto& [mode, collection] : windows) {
        for (const auto& window : collection) {
            window->on_editor_start();
        }
    }
}

void Editor::on_engine_update(const FrameData& time) {
    TMT_ZONE_SCOPED_N("Editor Update");
    {
        TMT_ZONE_SCOPED_N("ImGui New Frame");
        imgui_manager.new_frame();
    }

    main_menu_bar();

    for (const auto& window : windows[editor_mode]) {
        window->on_editor_update(time);
    }

    auto& open_windows = editor.save_data.open_windows;
    for (const auto& window : windows[editor_mode]) {
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
    for (const auto& window : windows[editor_mode]) {
        window->on_editor_fixed_update(time);
    }
}

void Editor::on_engine_end() {
    mode_handlers.clear();

    for (auto& [mode, collection] : windows) {
        for (const auto& window : collection) {
            window->on_editor_end();
        }
    }

    tmt::Log::info("Shutting down Thermite Editor...");
    imgui_manager.deinit();
    save_data.save();
}

void Editor::main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        mode_handlers[editor_mode]->display_main_menu();

        if (ImGui::BeginMenu("Windows")) {
            for (const auto& window : windows[editor_mode]) {
                const auto& name = window->get_title();
                bool& open = save_data.open_windows[name];
                ImGui::MenuItem(name.c_str(), nullptr, &open);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Renderer")) {
            /* List of display mode labels */
            static const std::vector<std::string> DISPLAY_MODE_LABELS { "Default", "Steps (0..128)", "Visibility", "Depth (0..100)", "Normals", "Albedo", "Illuminance" };

            const uint32_t display_mode_index = (uint32_t)magic_enum::enum_index<DisplayMode>(engine.renderer.display_mode).value_or(0u);
            const std::string& display_mode = DISPLAY_MODE_LABELS[display_mode_index];

            if (ImGui::BeginMenu(("Display Mode (" + display_mode + ")").c_str())) {
                /* Render all display mode options */
                for (uint32_t i = 0u; i < DISPLAY_MODE_LABELS.size(); ++i) {
                    const bool selected = engine.renderer.display_mode == magic_enum::enum_cast<DisplayMode>(i).value_or(DisplayMode::DEFAULT);

                    if (ImGui::MenuItem(DISPLAY_MODE_LABELS[i].c_str(), nullptr, selected)) {
                        engine.renderer.display_mode = magic_enum::enum_cast<DisplayMode>(i).value_or(DisplayMode::DEFAULT);
                    }
                }
                ImGui::EndMenu();
            }

            /* List of shading rate labels */
            static const std::vector<std::string> SHADING_RATE_LABELS { "Full-Rate", "Half-Rate (1:2)", "Quarter-Rate (1:4)" };

            static uint32_t shading_rate_index = 0u;
            const std::string& shading_rate = SHADING_RATE_LABELS[shading_rate_index];

            if (ImGui::BeginMenu(("DI Shading Rate (" + shading_rate + ")").c_str())) {
                /* Render all shading rate options */
                for (uint32_t i = 0u; i < SHADING_RATE_LABELS.size(); ++i) {
                    const bool selected = engine.renderer.render_view.get_shading_rate_di() == magic_enum::enum_cast<ShadingRate>(i).value_or(ShadingRate::FULL_RATE);

                    if (ImGui::MenuItem(SHADING_RATE_LABELS[i].c_str(), nullptr, selected)) {
                        shading_rate_index = i;
                        engine.renderer.render_view.set_shading_rate_di(magic_enum::enum_cast<ShadingRate>(i).value_or(ShadingRate::FULL_RATE));
                    }
                }
                ImGui::EndMenu();
            }
            
            if(ImGui::MenuItem("Enable UI Pipeline", nullptr, engine.renderer.ui_pipeline.render_ui_pipeline))
            {
                engine.renderer.ui_pipeline.render_ui_pipeline = !engine.renderer.ui_pipeline.render_ui_pipeline;            
            }

            ImGui::EndMenu();
        }

        ImGui::BeginDisabled(engine.game_controller.is_playing());
        if (ImGui::BeginMenu("Editor Mode")) {
            for (auto&& [mode, handler] : mode_handlers) {
                const bool is_selected = (editor_mode == mode);
                if (!ImGui::MenuItem(handler->get_name().c_str(), nullptr, is_selected) || is_selected) continue;

                switch_mode(mode);
            }
            ImGui::EndMenu();
        }
        ImGui::EndDisabled();

        ImGui::EndMainMenuBar();
    }
}

}  // namespace tmt
