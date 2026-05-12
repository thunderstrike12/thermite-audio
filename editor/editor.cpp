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
#include "engine/core/renderer/pipelines/lighting_pipeline.hpp"
#include "engine/core/renderer/pipelines/ui_pipeline.hpp"
#include "engine/core/scenes.hpp"
#include "engine/tools/player_data.hpp"

#include "editor/imgui/manager.hpp"
#include "editor/core/systems/font_manager.hpp"
#include "editor/core/systems/undo_redo/undo_redo_manager.hpp"
#include "engine/tools/profiler.hpp"
#include "engine/tools/file_dialog.hpp"
#include "editor/gizmo.hpp"

#include "editor/core/systems/pop_up/pop_up.hpp"

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
#include "editor/windows/physics_layers_editor.hpp"
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
#include "editor/windows/rig_state_controller.hpp"
#include "editor/windows/ui.hpp"
#include "editor/windows/player_data.hpp"
#include "editor/windows/voxel_editor_lights.hpp"

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
    Log::info("Starting Thermite Editor...");
    imgui_manager.init();
    gizmo.init();

    mode_handlers[Mode::SCENE] = std::make_unique<SceneMode>();
    mode_handlers[Mode::VOXEL] = std::make_unique<VoxelMode>();
    mode_handlers[Mode::PREFAB] = std::make_unique<PrefabMode>();

    systems[Mode::SCENE].add<Hierarchy>();
    systems[Mode::SCENE].add<GameFlow>();
    systems[Mode::SCENE].add<Inspector>();
    systems[Mode::SCENE].add<Viewport>();
    systems[Mode::SCENE].add<Profiler>();
    systems[Mode::SCENE].add<GoapDebugger>();
    systems[Mode::SCENE].add<GoapActionEditor>();
    systems[Mode::SCENE].add<GoapAgentEditor>();
    systems[Mode::SCENE].add<PhysicsLayersEditor>();
    systems[Mode::SCENE].add<FontControl>();
    systems[Mode::SCENE].add<AudioMixer>();
    systems[Mode::SCENE].add<ScenesWindow>();
    systems[Mode::SCENE].add<AssetBrowser>();
    systems[Mode::SCENE].add<Console>();
    systems[Mode::SCENE].add<ImguiDemo>();
    systems[Mode::SCENE].add<MotionMathPreview>();
    systems[Mode::SCENE].add<DebugLines>();
    systems[Mode::SCENE].add<EcsInspector>();
    systems[Mode::SCENE].add<Rendering>();
    systems[Mode::SCENE].add<UndoRedoManager>();
    systems[Mode::SCENE].add<EditorSettingsWindow>();
    systems[Mode::SCENE].add<BuildPackager>();
    systems[Mode::SCENE].add<UIEditor>();
    systems[Mode::SCENE].add<PlayerDataWindow>();
    systems[Mode::SCENE].add<RigStateController>();

    engine.scenes.register_scene<VoxelEditScene>();
    systems[Mode::VOXEL].add<ModelViewer>();
    systems[Mode::VOXEL].add<NodeHierarchy>();
    systems[Mode::VOXEL].add<Palette>();
    systems[Mode::VOXEL].add<MaterialEditor>();
    systems[Mode::VOXEL].add<Brush>();
    systems[Mode::VOXEL].add<Console>();
    systems[Mode::VOXEL].add<UndoRedoManager>();
    systems[Mode::VOXEL].add<VoxelEditorLights>();

    engine.scenes.register_scene<PrefabEditScene>();
    systems[Mode::PREFAB].add<Hierarchy>();
    systems[Mode::PREFAB].add<Inspector>();
    systems[Mode::PREFAB].add<Viewport>();
    systems[Mode::PREFAB].add<AssetBrowser>();
    systems[Mode::PREFAB].add<Console>();
    systems[Mode::PREFAB].add<ScenesWindow>();
    systems[Mode::PREFAB].add<UndoRedoManager>();
    systems[Mode::PREFAB].add<UIEditor>();
    systems[Mode::PREFAB].add<DebugLines>();
    systems[Mode::PREFAB].add<RigStateController>();

    for (auto& [mode, collection] : systems) {
        for (const auto& system : collection) {
            system->on_editor_start();
        }
    }

    save_data.load();
}

void Editor::on_engine_update(const FrameData& time) {
    TMT_ZONE_SCOPED_N("Editor Update");
    {
        TMT_ZONE_SCOPED_N("ImGui New Frame");
        imgui_manager.new_frame();
    }

    main_menu_bar();
    main_status_bar();

    // Call on_editor_update for all systems
    for (const auto& system : systems[editor_mode]) {
        system->on_editor_update(time);
    }

    // Render windows (only for systems that are IWindow instances)
    auto& open_windows = editor.save_data.open_windows;
    for (const auto& system : systems[editor_mode]) {
        IWindowBase* window = dynamic_cast<IWindowBase*>(system.get());
        if (!window) continue;  // Skip non-window systems

        const auto& name = window->get_title();
        TMT_ZONE_SCOPED_STRING(name);

        const ImGuiWindowFlags_ flags = static_cast<ImGuiWindowFlags_>(window->get_window_flags());
        if (open_windows.contains(name) == false) {
            open_windows[name] = window->default_open();
        }
        bool& open = open_windows[name];

        if (open) {
            window->before_begin();
            if (ImGui::Begin(name.c_str(), window->is_closable() ? &open : nullptr, flags)) {
                window->on_inspect();
            }
            ImGui::End();
            window->end_display();
        }
    }

    PopUpManager::update();
    NotificationManager::update(time.delta_time);

    {
        TMT_ZONE_SCOPED_N("ImGui End Frame");
        imgui_manager.end_frame();
    }
}

void Editor::on_engine_fixed_update(const FrameData& time) {
    for (const auto& system : systems[editor_mode]) {
        system->on_editor_fixed_update(time);
    }
}

void Editor::on_engine_end() {
    mode_handlers.clear();

    for (auto& [mode, collection] : systems) {
        for (const auto& system : collection) {
            system->on_editor_end();
        }
    }

    tmt::Log::info("Shutting down Thermite Editor...");
    imgui_manager.deinit();
    save_data.save();
}

void Editor::main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        /* Mode-specific menu (Scene, Prefab, etc.) */
        mode_handlers[editor_mode]->display_main_menu();

        if (ImGui::BeginMenu("Windows")) {
            for (const auto& system : systems[editor_mode]) {
                IWindowBase* window = dynamic_cast<IWindowBase*>(system.get());
                if (!window) continue;  // Only show actual windows in the menu

                const auto& name = window->get_title();
                bool& open = save_data.open_windows[name];
                ImGui::MenuItem(name.c_str(), nullptr, &open);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Renderer")) {
            /* List of display mode labels */
            static const std::vector<std::string> DISPLAY_MODE_LABELS { "Default", "Steps (0..128)", "Visibility", "Depth (0..100)", "Normals",
                                                                        "Albedo",  "Illuminance",    "Cache",      "Motion Vectors", "Lights" };

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

                if (engine.renderer.display_mode != DisplayMode::DEFAULT) {
                    engine.renderer.enable_taa = false;
                }
            }

            RendererSettings& settings = engine.player_data.get<RendererSettings>("RendererSettings");

            /* List of shading rate labels */
            static const std::vector<std::string> SHADING_RATE_LABELS { "Full-Rate", "Half-Rate (1:2)", "Quarter-Rate (1:4)" };
            const std::string& diff_shading_rate = SHADING_RATE_LABELS[magic_enum::enum_index<ShadingRate>(settings.diff_shading_rate).value_or(0)];
            const std::string& spec_shading_rate = SHADING_RATE_LABELS[magic_enum::enum_index<ShadingRate>(settings.spec_shading_rate).value_or(0)];

            if (ImGui::BeginMenu(("Diffuse Shading Rate (" + diff_shading_rate + ")").c_str())) {
                /* Render all shading rate options */
                for (uint32_t i = 0u; i < SHADING_RATE_LABELS.size(); ++i) {
                    const bool selected = settings.diff_shading_rate == magic_enum::enum_cast<ShadingRate>(i).value_or(ShadingRate::FULL_RATE);

                    if (ImGui::MenuItem(SHADING_RATE_LABELS[i].c_str(), nullptr, selected)) {
                        settings.diff_shading_rate = magic_enum::enum_cast<ShadingRate>(i).value_or(ShadingRate::FULL_RATE);
                        engine.renderer.render_view.update_gbuffers();
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(("Specular Shading Rate (" + spec_shading_rate + ")").c_str())) {
                /* Render all shading rate options */
                for (uint32_t i = 0u; i < SHADING_RATE_LABELS.size(); ++i) {
                    const bool selected = settings.spec_shading_rate == magic_enum::enum_cast<ShadingRate>(i).value_or(ShadingRate::FULL_RATE);

                    if (ImGui::MenuItem(SHADING_RATE_LABELS[i].c_str(), nullptr, selected)) {
                        settings.spec_shading_rate = magic_enum::enum_cast<ShadingRate>(i).value_or(ShadingRate::FULL_RATE);
                        engine.renderer.render_view.update_gbuffers();
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Enable UI Pipeline", nullptr, engine.renderer.ui_pipeline.render_ui_pipeline)) {
                engine.renderer.ui_pipeline.render_ui_pipeline = !engine.renderer.ui_pipeline.render_ui_pipeline;
            }

            if (ImGui::MenuItem("Toggle TAA", nullptr, engine.renderer.enable_taa)) {
                engine.renderer.enable_taa = !engine.renderer.enable_taa;
            }

            if (ImGui::BeginMenu("Bloom")) {
                ImGui::SliderFloat("Radius (px)", &settings.bloom_radius, 0.0f, 1.0f);
                ImGui::SliderFloat("Threshold (lum)", &settings.bloom_threshold, 0.0f, 10.0f);
                ImGui::SliderFloat("Trail (%)", &settings.bloom_trail, 0.0f, 1.0f);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Auto Exposure")) {
                ImGui::DragFloat("Key Value", &settings.autox_key_value, 0.01f, 0.001f, 100.0f);
                ImGui::DragFloat("Min (lum)", &settings.autox_lum_min, 0.01f, 0.0f, 100000.0f);
                ImGui::DragFloat("Max (lum)", &settings.autox_lum_max, 0.01f, 0.0f, 100000.0f);
                ImGui::SliderFloat("Response", &settings.autox_response, 0.01f, 10.0f);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Fog")) {
                ImGui::DragFloat("Base Step Size", &settings.fog_base_step_size, 0.01f, 0.001f, 0.1f);
                int count = (int)settings.fog_step_count;
                if (ImGui::DragInt("Step Count", &count, 1.0f, 16, 128)) {
                    settings.fog_step_count = (uint32_t)count;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        ImGui::Dummy({ 5, 0 });
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::Dummy({ 5, 0 });

        {
            ImGui::BeginDisabled(engine.game_controller.is_playing());
            const bool is_scene_mode = (editor_mode == Mode::SCENE);
            const bool is_voxel_mode = (editor_mode == Mode::VOXEL);

            if (is_scene_mode) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }
            if (ImGui::Button("Scene##Switch")) {
                switch_mode(Mode::SCENE);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scene Editing Mode");
            if (is_scene_mode) {
                ImGui::PopStyleColor();
            }
            if (is_voxel_mode) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }
            if (ImGui::Button("Voxel##Switch")) {
                switch_mode(Mode::VOXEL);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Voxel Editing Mode");
            if (is_voxel_mode) {
                ImGui::PopStyleColor();
            }
            ImGui::EndDisabled();
        }

        /* Scene switcher dropdown - right-aligned */
        {
            const auto& scenes = engine.scenes.get_registered_scenes();
            const auto& active_scene = engine.scenes.get_active_scene();
            const std::string scene_name = active_scene ? std::string(active_scene->get_name()) : "None";

            const float combo_width = 200.0f;
            const float menu_bar_width = ImGui::GetWindowWidth();
            const float padding = ImGui::GetStyle().ItemSpacing.x;

            ImGui::SameLine(menu_bar_width - combo_width - padding);
            ImGui::SetNextItemWidth(combo_width);
            if (ImGui::BeginCombo("##SceneSwitcher", scene_name.c_str())) {
                for (const auto& [type_index, scene_info] : scenes) {
                    if (type_index == typeid(VoxelEditScene)) continue;
                    if (type_index == typeid(PrefabEditScene)) continue;

                    bool is_selected = (active_scene && typeid(*active_scene) == type_index);
                    if (ImGui::Selectable(scene_info.name.c_str(), is_selected)) {
                        auto current_mode = editor_mode;
                        if (current_mode != Mode::SCENE) {
                            switch_mode(Mode::SCENE);
                        }
                        engine.scenes.enqueue_scene(type_index);
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::EndMainMenuBar();
    }
}

void Editor::main_status_bar() {
    const auto flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    const float font_size_multiplier = 0.75f;
    ImGui::PushFont(NULL, ImGui::GetStyle().FontSizeBase * font_size_multiplier);

    const auto text_height = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y * 2.0f;

    if (ImGui::BeginViewportSideBar("##StatusBar", NULL, ImGuiDir_Down, text_height, flags)) {
        ImGui::Text("Thermite Editor");
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        ImGui::Text("%s", engine.app_specs().name.c_str());
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        constexpr const char* version = BUILD_VERSION;
        if (version[0] != '\0') {
            ImGui::Text("%s", version);
        } else {
            ImGui::TextDisabled("dev (no version provided)");
        }
        ImGui::SameLine();
        const float fps_anchor_x = ImGui::GetCursorPosX();
        const float fps_reserved = ImGui::CalcTextSize("FPS: 999").x + ImGui::GetStyle().ItemSpacing.x;
        ImGui::Text("fps: %.0f", ImGui::GetIO().Framerate);
        ImGui::SameLine(fps_anchor_x + fps_reserved);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        ImGui::Text("ms: %.2f", 1000.0f / ImGui::GetIO().Framerate);
        ImGui::SameLine();

        ImGui::End();
    }

    ImGui::PopFont();
}

}  // namespace tmt
