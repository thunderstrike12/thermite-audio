#include "viewport.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>
#include "editor/editor.hpp"
#include "editor/gizmo.hpp"
#include "hierarchy.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"

#include "engine/core/renderer/pipelines/ui_pipeline.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/input/input_map.hpp"
#include "editor/events/scene.hpp"
#include "editor/windows/scenes.hpp"
#include "engine/core/components/ui_component.hpp"
#include "engine/core/components/image_renderer.hpp"
#include "game_flow.hpp"

#include "editor/imgui/tools/buttons.hpp"
#include "engine/core/components/text_renderer.hpp"
#include "engine/tools/fps_limiter.hpp"

void tmt::Viewport::on_editor_start() {
    if (auto value = editor.save_data.fps_limit) {
        engine.fps_limiter.set_target_fps(*value);
    }
}

void tmt::Viewport::on_editor_update(const tmt::FrameData& frame_data) {
    update_debug_camera(frame_data);

    const bool f11_down = ImGui::IsKeyPressed(ImGuiKey_F11, false);
    const bool is_playing = engine.game_controller.is_running();
    if (f11_down && is_playing == false) {
        engine.window.toggle_fullscreen();
    }
}

void tmt::Viewport::on_editor_end() {}

void tmt::Viewport::before_begin() {
    /*default min size*/
    ImGui::SetNextWindowSize(ImVec2(960, 540), ImGuiCond_FirstUseEver);

    /* Zero margin */
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    y_frame_padding = ImGui::GetStyle().FramePadding.y;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 8.0f));
}

void tmt::Viewport::end_display() {
    ImGui::PopStyleVar(2);
}

std::string tmt::Viewport::get_title() const {
    const auto window_id = "###" ICON_MS_VISIBILITY " Viewport";
    const auto scene_name = engine.scenes.is_scene_loaded() ? engine.scenes.get_active_scene_info().name : "UNKNOWN";
    return scene_name + window_id;
}
int tmt::Viewport::get_window_flags() const {
    int flags = ImGuiWindowFlags_MenuBar;

    if (!engine.game_controller.is_running()) {
        const bool is_scene_dirty = editor.systems[editor.editor_mode].get<ScenesWindow>().is_scene_dirty();
        if (is_scene_dirty) flags |= ImGuiWindowFlags_UnsavedDocument;
    }

    return flags;
}

void tmt::Viewport::start_game() {
    engine.game_controller.start_game();
}

void tmt::Viewport::end_game() {
    engine.game_controller.end_game();
}

void tmt::Viewport::pause_game() {
    engine.game_controller.pause_game();
}

void tmt::Viewport::resume_game() {
    engine.game_controller.resume_game();
}

void tmt::Viewport::on_inspect() {
    toolbar();

    auto size = ImGui::GetContentRegionAvail();
    size = ImVec2(std::max(size.x, 1.0f), std::max(size.y, 1.0f));
    width = size.x;
    height = size.y;

    viewport_pos = { ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y };
    is_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

    if (height <= 0.f) return;

    if (!engine.renderer.screenshot_settings.request_capture) engine.renderer.render_view.set_viewport_size(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    ImGui::BeginChild("viewport_render", ImVec2(0, 0), 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    ImGui::Image((ImTextureRef)engine.renderer.render_view.imgui_viewport, size);
    is_hovered = ImGui::IsItemHovered();
    auto imgui_mouse_pos = ImGui::GetMousePos();
    mouse_pos.x = imgui_mouse_pos.x - viewport_pos.x;
    mouse_pos.y = imgui_mouse_pos.y - viewport_pos.y;
    viewport_drawlist = ImGui::GetWindowDrawList();

    const std::vector<Entity>& selected_entities = editor.systems[editor.editor_mode].get<Hierarchy>().get_selected_entities();

    float snap_value = 0.0f;
    const bool ctrl_held = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    const bool shift_held = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
    const bool alt_held = ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);

    if (ctrl_held) {
        const auto operation = Gizmo::OPERATIONS[editor.gizmo.operation];
        switch (operation) {
            case ImGuizmo::OPERATION::TRANSLATE:
                snap_value = editor.save_data.snap_values.move;
                break;
            case ImGuizmo::OPERATION::ROTATE:
                snap_value = editor.save_data.snap_values.rotation;
                break;
            case ImGuizmo::OPERATION::SCALE:
                snap_value = editor.save_data.snap_values.scale;
                break;
            case ImGuizmo::OPERATION::BOUNDS:
                snap_value = editor.save_data.snap_values.move;  // Use move snap for bounds
                break;
            default:
                break;
        }
    }

    const bool wants_to_capture_keyboard = ImGui::GetIO().WantCaptureKeyboard;
    if (wants_to_capture_keyboard == false && using_debug_camera == false) {
        if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
            editor.gizmo.operation = 0;  // Translate
        } else if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
            editor.gizmo.operation = 1;  // Rotate
        } else if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
            editor.gizmo.operation = 2;  // Scale
        } else if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
            editor.gizmo.operation = 3;  // Bounds
        } else if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
            snap_to_entity();
        }
    }

    // Build modifiers for bounds manipulation
    BoundsModifiers modifiers;
    modifiers.uniform_scale = shift_held;    // Shift: maintain aspect ratio
    modifiers.scale_from_center = alt_held;  // Alt: keep center fixed

    const bool gizmo_changed = editor.gizmo.manip(viewport_pos.x, viewport_pos.y, width, height, selected_entities, snap_value, modifiers);
    if (gizmo_changed) OnSceneModified::dispatch();

    if (allow_selection) selection_logic(imgui_mouse_pos, viewport_pos, false);

    ImGui::EndChild();

    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        ImGui::SetWindowFocus();
    }
    if (is_hovered && engine.input.get_game_preferred_mouse_lock() && engine.game_controller.is_running() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        engine.input.lock_mouse(true);
        engine.input.set_mouse_relative_to_window(true);
        engine.input.set_game_preferred_mouse_lock(false);
    }
}

void tmt::Viewport::snap_to_entity() {
    const auto* hierarchy = editor.systems[editor.editor_mode].try_get<Hierarchy>();
    Entity selected_entity = hierarchy->get_first_selected_entity();
    if (hierarchy == nullptr || !engine.ecs.valid(selected_entity)) return;

    auto& transform = engine.ecs.get_component<Transform>(selected_entity);
    glm::vec3 target_pos = transform.get_world_position();
    glm::vec3 target_scale = transform.get_world_scale();

    const auto* target_renderer = engine.ecs.try_get_component<VoxelRenderer>(selected_entity);

    float distance = 10.0f;  // fallback

    Camera& camera = engine.renderer.get_debug_camera();
    Transform& camTransform = engine.renderer.get_debug_transform();

    if (target_renderer && target_renderer->resource) {
        ResourceRef<VoxelVolume> volume = target_renderer->resource;

        if (volume) {
            // Get voxel grid size
            glm::vec3 voxelSize = glm::vec3(volume->size);

            // Apply world scale
            glm::vec3 worldSize = voxelSize * target_scale;

            glm::vec3 halfExtents = worldSize * 0.5f;

            float radius = glm::length(halfExtents);

            float fovRadians = glm::radians(camera.fov);

            distance = radius / std::tan(fovRadians * 0.5f);

            distance *= 0.2f;  // padding
        }
    }

    glm::vec3 forward = camTransform.get_forward();
    glm::vec3 newCamPos = target_pos - forward * distance;

    camTransform.set_world_position(newCamPos);
    camTransform.look_at(target_pos, glm::vec3(0, 1, 0));

    // Sync yaw/pitch
    glm::vec3 dir = glm::normalize(target_pos - newCamPos);
    camera.yaw = glm::degrees(atan2(dir.z, dir.x));
    camera.pitch = glm::degrees(asin(dir.y));
}

void tmt::Viewport::toolbar() {
    if (!ImGui::BeginMenuBar()) return;

    ImGuiStyle& style = ImGui::GetStyle();

    // Override button padding back to normal
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, y_frame_padding));

    // Vertically center buttons within the (now taller) bar
    // float bar_height = ImGui::GetCurrentWindow()->MenuBarHeight;
    // float btn_height = ImGui::GetFrameHeightWithSpacing();  // height with the new smaller padding
    // float center_y = ImGui::GetCursorPosY() + (bar_height - btn_height) * 0.5f - style.FramePadding.y;
    // ImGui::SetCursorPosY(center_y);

    /* ===== Left side: Gizmo controls ===== */

    /* Gizmo Space Toggle (Global/Local) */
    const char* space_label = editor.gizmo.space ? ICON_MS_LANGUAGE " Global" : ICON_MS_VIEW_IN_AR " Local";
    if (ImGui::Button(space_label)) {
        editor.gizmo.space = static_cast<uint8_t>(!editor.gizmo.space);
    }

    ImGui::Spacing();

    /* Gizmo Operation Buttons (Translate/Rotate/Scale) */
    const bool translate_active = (editor.gizmo.operation == 0);
    const bool rotate_active = (editor.gizmo.operation == 1);
    const bool scale_active = (editor.gizmo.operation == 2);
    const bool rect_active = (editor.gizmo.operation == 3);

    if (translate_active) ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
    if (ImGui::Button(ICON_MS_DRAG_PAN "##Translate")) {
        editor.gizmo.operation = 0;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate (W)");
    if (translate_active) ImGui::PopStyleColor();

    if (rotate_active) ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
    if (ImGui::Button(ICON_MS_ROTATE_RIGHT "##Rotate")) {
        editor.gizmo.operation = 1;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate (E)");
    if (rotate_active) ImGui::PopStyleColor();

    if (scale_active) ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
    if (ImGui::Button(ICON_MS_ZOOM_OUT_MAP "##Scale")) {
        editor.gizmo.operation = 2;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale (R)");
    if (scale_active) ImGui::PopStyleColor();

    if (rect_active) ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonActive]);
    if (ImGui::Button(ICON_MS_ACTIVITY_ZONE "##Bounds")) {
        editor.gizmo.operation = 3;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Bounds (T, UI only)");
    if (rect_active) ImGui::PopStyleColor();

    ImGui::Spacing();

    /* ===== Center: Game Flow Controls (icon-only) ===== */
    float icon_btn_width = ImGui::GetFrameHeight();                    // Square buttons
    float game_flow_width = icon_btn_width * 2 + style.ItemSpacing.x;  // Play + Pause buttons

    float left_cursor = ImGui::GetCursorPosX();
    float menu_bar_width = ImGui::GetWindowWidth();
    float center_pos = (menu_bar_width - game_flow_width) * 0.5f;

    // Only center if there's enough space
    if (center_pos > left_cursor) {
        ImGui::SetCursorPosX(center_pos);
    }

    // Play/Stop button
    if (engine.game_controller.is_playing()) {
        if (ImGui::Button(ICON_MS_STOP "##Stop")) {
            // end_game();
            editor.systems[editor.editor_mode].get<GameFlow>().end_game();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ctrl+P: To Stop");
    } else {
        if (ImGui::Button(ICON_MS_PLAY_ARROW "##Play")) {
            const bool shift_down = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
            editor.systems[editor.editor_mode].get<GameFlow>().start_game(shift_down);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ctrl+P: To Start\n(Hold) Shift: for Fullscreen");
    }

    // Pause/Resume button (only shown when playing)
    const bool can_interact = engine.game_controller.is_playing();

    if (can_interact == false) ImGui::BeginDisabled();
    if (engine.game_controller.is_paused()) {
        if (ImGui::Button(ICON_MS_PLAY_ARROW "##Resume")) {
            editor.systems[editor.editor_mode].get<GameFlow>().resume_game();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ctrl+Shift+P: To Resume");
    } else {
        if (ImGui::Button(ICON_MS_PAUSE "##Pause")) {
            editor.systems[editor.editor_mode].get<GameFlow>().pause_game();
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ctrl+Shift+P: To Pause");
    }
    if (can_interact == false) ImGui::EndDisabled();

    /* ===== Right side: Display info ===== */
    const bool is_unlimited = engine.fps_limiter.is_enabled() == false;
    const float current_limit = is_unlimited ? -1.f : engine.fps_limiter.get_target_fps();

    const std::string fps_display = ICON_MS_SPEED + std::string(" ") + (is_unlimited ? "Unlimited FPS" : std::to_string(static_cast<int>(current_limit)) + " FPS");
    float right_width = ImGui::CalcTextSize(ICON_MS_ASPECT_RATIO " 0000x0000").x + style.ItemSpacing.x + style.FramePadding.x;
    right_width += ImGui::CalcTextSize(fps_display.c_str()).x + style.ItemSpacing.x + style.FramePadding.x;

    float avail = ImGui::GetContentRegionAvail().x;
    if (avail > right_width) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - right_width);
    }

    /* Resolution display */
    char res_text[32];
    snprintf(res_text, sizeof(res_text), ICON_MS_ASPECT_RATIO " %dx%d", static_cast<int>(width), static_cast<int>(height));
    ImGui::Text("%s", res_text);

    /* fps limit dropdown */
    if (ImGui::BeginMenu(fps_display.c_str())) {
        const float limits[] = { 30.f, 60.f, 120.f, 144.f, 240.f };

        for (float limit : limits) {
            std::string label = std::to_string(static_cast<int>(limit));
            if (current_limit == limit) label = ICON_MS_CHECK " " + label;
            if (ImGui::MenuItem(label.c_str())) {
                engine.fps_limiter.set_target_fps(limit);
                editor.save_data.fps_limit = limit;
            }
        }

        std::string label = "Unlimited";
        if (is_unlimited) label = ICON_MS_CHECK " " + label;
        if (ImGui::MenuItem(label.c_str())) {
            engine.fps_limiter.disable();
            editor.save_data.fps_limit = std::nullopt;
        }

        ImGui::EndMenu();
    }

    ImGui::PopStyleVar();  // Pop the FramePadding override
    ImGui::EndMenuBar();
}

void tmt::Viewport::selection_logic(const ImVec2& imgui_mouse_pos, const glm::vec2& image_pos, bool toolbar_buttons_hovered) {
    if (!is_hovered || engine.game_controller.is_running()) return;

    auto& hierarchy = editor.systems[Editor::Mode::SCENE].get<Hierarchy>();
    const std::vector<Entity>& selected_entities = hierarchy.get_selected_entities();
    auto& imgui_io = ImGui::GetIO();

    static ImVec2 first_click_pos;
    static bool using_rect = false;

    bool not_multiple_select_modifier = !imgui_io.KeyCtrl && !imgui_io.KeyShift;

    if (imgui_io.MouseReleased[0]) using_rect = false;
    if (imgui_io.MouseDown[0] && (!editor.gizmo.hovered() || using_rect)) {
        if (imgui_io.MouseClicked[0]) {
            first_click_pos = imgui_io.MousePos;
        }
        ImVec2 delta = imgui_io.MousePos - first_click_pos;

        // if delta has moved by 5 pixels, start rectangle logic
        if (abs(delta.x) + abs(delta.y) > 5 && allow_rectangle_select) {
            using_rect = true;

            viewport_drawlist->AddRectFilled(first_click_pos, imgui_io.MousePos, ImColor(1.f, 1.f, 1.f, 0.2f));
            viewport_drawlist->AddRect(first_click_pos, imgui_io.MousePos, ImColor(1.f, 1.f, 1.f, 0.6f));

            glm::ivec2 a = { first_click_pos.x - image_pos.x, first_click_pos.y - image_pos.y };
            glm::ivec2 b = { imgui_mouse_pos.x - image_pos.x, imgui_mouse_pos.y - image_pos.y };

            int x0 = std::min(a.x, b.x);
            int x1 = std::max(a.x, b.x);
            int y0 = std::min(a.y, b.y);
            int y1 = std::max(a.y, b.y);

            auto view = engine.ecs.view<UIComponent>();
            static std::vector<Entity> tracking_rect_entities;
            tracking_rect_entities.clear();
            for (int x = x0; x < x1; x += 5) {
                for (int y = y0; y < y1; y += 5) {
                    const tmt::Ray selection_ray = tmt::engine.renderer.render_view.pixel_ray({ x, y });
                    const tmt::Hit hit = tmt::engine.renderer.trace_ray(selection_ray);

                    if (!hit.miss()) {
                        tracking_rect_entities.push_back(hit.entity);
                        hierarchy.add_entity_to_selection(hit.entity);
                    }
                }
            }

            // cleanup when area shrinks
            if (not_multiple_select_modifier) {
                for (auto entity : selected_entities) {
                    if (std::find(tracking_rect_entities.begin(), tracking_rect_entities.end(), entity) == tracking_rect_entities.end()) {
                        hierarchy.remove_entity_from_selection(entity);
                    }
                }
            }
        }
    }
    if (imgui_io.MouseClicked[0] && !editor.gizmo.hovered() && !toolbar_buttons_hovered) {
        const tmt::Ray selection_ray = tmt::engine.renderer.render_view.pixel_ray({ mouse_pos.x, mouse_pos.y });
        const tmt::Hit hit = tmt::engine.renderer.trace_ray(selection_ray);

        if (not_multiple_select_modifier) hierarchy.clear_selection();

        if (engine.renderer.ui_pipeline.render_ui_pipeline) {
            auto image_view = engine.ecs.view<UIComponent, ImageRenderer, Transform>();
            auto text_view = engine.ecs.view<UIComponent, TextRenderer, Transform>();

            std::vector<std::tuple<Entity, UIComponent, float>> ui_entities;
            ui_entities.reserve(image_view.size_hint() + text_view.size_hint());

            for (auto [entity, ui_comp, image_renderer, transform] : image_view.each()) {
                const float z_distance = transform.get_world_position().z;
                ui_entities.emplace_back(entity, ui_comp, z_distance);
            }
            for (auto [entity, ui_comp, text_renderer, transform] : text_view.each()) {
                const float z_distance = transform.get_world_position().z;
                ui_entities.emplace_back(entity, ui_comp, z_distance);
            }

            std::sort(ui_entities.begin(), ui_entities.end(), [](const std::tuple<Entity, UIComponent, float>& a, const std::tuple<Entity, UIComponent, float>& b) {
                return std::get<2>(a) > std::get<2>(b);
            });

            for (const auto& [entity, ui_comp, z_distance] : ui_entities) {
                const auto& transform = engine.ecs.get_component<Transform>(entity);
                const auto& ui_component = engine.ecs.get_component<UIComponent>(entity);
                if (AnchorHelper::is_inside(entity, { mouse_pos.x, mouse_pos.y }, ui_component, transform)) {
                    hierarchy.add_entity_to_selection(entity);
                    // we only want to open the tree if the selected entity has a parent
                    if (transform.has_parent()) {
                        Entity parent = transform.get_parent();
                        force_open_recurse_upwards(parent);
                    }
                    break;
                }
            }
        }

        if (!hit.miss()) {
            if (hierarchy.is_entity_selected(hit.entity) && !(not_multiple_select_modifier))
                hierarchy.remove_entity_from_selection(hit.entity);
            else {
                hierarchy.add_entity_to_selection(hit.entity);
                Entity first_entity = hierarchy.get_first_selected_entity();

                // only care about opening the first selected
                if (first_entity == hit.entity) {
                    auto& transform = engine.ecs.get_component<Transform>(hit.entity);

                    // we only want to open the tree if the selected entity has a parent
                    if (transform.has_parent()) {
                        Entity parent = transform.get_parent();
                        force_open_recurse_upwards(parent);
                    }
                }
            }
        }
    }
}

void tmt::Viewport::force_open_recurse_upwards(Entity entity) {
    auto& hierarchy = editor.systems[Editor::Mode::SCENE].get<Hierarchy>();

    hierarchy.add_entity_to_forced_open(entity);

    auto& transform = engine.ecs.get_component<Transform>(entity);
    if (transform.has_parent()) {
        Entity parent = transform.get_parent();
        force_open_recurse_upwards(parent);
    }
}

void tmt::Viewport::on_retrieve_mouse_state(MouseOverride& event) {
    event.x = mouse_pos.x;
    event.y = mouse_pos.y;
    event.handled = true;
}

void tmt::Viewport::on_block_input_request(OnBlockInputEvent& event) {
    if (is_focused == false) {
        event.block = true;
    }
    event.handled = true;
}

void tmt::Viewport::on_game_start() {
    ImGui::SetWindowFocus(get_title().c_str());
}

void tmt::Viewport::update_debug_camera(const tmt::FrameData& time) {
    if (engine.game_controller.is_running() || is_hovered == false) return;
    // Gather Variables to be used
    auto& input = engine.input;

    const bool enable_mouse_look = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool is_2d_axis_movement = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    if (!enable_mouse_look) {
        /* Show mouse cursor */
        input.set_mouse_relative_to_window(false);
        input.lock_mouse(false);
        using_debug_camera = false;
        return;
    }
    using_debug_camera = true;
    /* Cancel any text fields in editor you may interact with */
    ImGui::SetWindowFocus();

    /* Hide mouse cursor */
    input.set_mouse_relative_to_window(true);
    input.lock_mouse(true);

    Camera& camera = engine.renderer.get_debug_camera();
    Transform& transform = engine.renderer.get_debug_transform();

    // const float dx = ImGui::GetIO().MouseDelta.x;
    // const float dy = ImGui::GetIO().MouseDelta.y;
    // printf("Mouse Delta: %f, %f\n", dx, dy);
    const float dx = input.get_mouse_delta_engine_x();
    const float dy = input.get_mouse_delta_engine_y();

    if (is_2d_axis_movement == false) {
        // Mouse Look
        camera.yaw -= dx * Config::MOUSE_SENSITIVITY;
        camera.pitch -= dy * Config::MOUSE_SENSITIVITY;
        camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

        glm::vec3 front = {};
        front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        front.y = sin(glm::radians(camera.pitch));
        front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        front = glm::normalize(front);
        transform.look_at(transform.get_world_position() + front, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    const float mouse_wheel_y_delta = ImGui::GetIO().MouseWheel;
    camera_speed *= std::pow(2.0f, mouse_wheel_y_delta * 0.15f);
    camera_speed = glm::clamp(camera_speed, Config::MIN_BASE_SPEED, Config::MAX_BASE_SPEED);

    glm::vec3 pos = transform.get_world_position();

    if (is_2d_axis_movement) {
        // 2D Axis Movement
        const bool sprint = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
        const glm::vec3 horizontal_move = transform.get_right() * dx * Config::MOUSE_SENSITIVITY * 0.1f;
        const glm::vec3 direction = sprint ? transform.get_forward() : transform.get_up();
        const glm::vec3 vertical_move = direction * -dy * Config::MOUSE_SENSITIVITY * 0.1f;
        pos += (horizontal_move + vertical_move);
    } else {
        // Movement (WASD + QE)
        glm::vec3 move_dir = { 0.0f, 0.0f, 0.0f };
        if (ImGui::IsKeyDown(ImGuiKey_W)) move_dir += transform.get_forward();
        if (ImGui::IsKeyDown(ImGuiKey_S)) move_dir -= transform.get_forward();
        if (ImGui::IsKeyDown(ImGuiKey_A)) move_dir -= transform.get_right();
        if (ImGui::IsKeyDown(ImGuiKey_D)) move_dir += transform.get_right();
        if (ImGui::IsKeyDown(ImGuiKey_E)) move_dir += transform.get_up();
        if (ImGui::IsKeyDown(ImGuiKey_Space)) move_dir += transform.get_up();
        if (ImGui::IsKeyDown(ImGuiKey_Q)) move_dir -= transform.get_up();
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) move_dir -= transform.get_up();

        if (glm::length(move_dir) > 0.0f) pos += glm::normalize(move_dir) * camera_speed * time.delta_time;
    }

    transform.set_world_position(pos);
}
