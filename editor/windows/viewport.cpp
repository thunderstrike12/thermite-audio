#include "viewport.hpp"
#include <imgui.h>
#include <ImGuizmo.h>
#include "editor/editor.hpp"
#include "editor/gizmo.hpp"
#include "hierarchy.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/renderer/renderer.hpp"

#include "engine/core/renderer/pipelines/ui_pipeline.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/input/input_map.hpp"
#include "editor/events/scene.hpp"
#include "editor/windows/scenes.hpp"
#include "engine/core/components/ui_component.hpp"

void tmt::Viewport::on_editor_start() {
    // Setup Input Actions
    register_input_actions();
}

void tmt::Viewport::register_input_actions() {
    engine.input_map.add_action_keys(Config::SPRINT, Key::LEFT_SHIFT);
    engine.input_map.add_action_keys(Config::FORWARD, Key::W);
    engine.input_map.add_action_keys(Config::BACKWARD, Key::S);
    engine.input_map.add_action_keys(Config::RIGHT, Key::D);
    engine.input_map.add_action_keys(Config::LEFT, Key::A);
    engine.input_map.add_action_keys(Config::UP, Key::E);
    engine.input_map.add_action_keys(Config::DOWN, Key::Q);
    engine.input_map.add_action_keys(Config::UP, Key::SPACE);
    engine.input_map.add_action_keys(Config::DOWN, Key::LEFT_CTRL);
}

void tmt::Viewport::on_editor_update(const tmt::FrameData& frame_data) {
    update_debug_camera(frame_data);
}

void tmt::Viewport::on_editor_end() {}

void tmt::Viewport::before_begin() {
    /*default min size*/
    ImGui::SetNextWindowSize(ImVec2(960, 540), ImGuiCond_FirstUseEver);

    /* Zero margin */
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
}

void tmt::Viewport::end_display() {
    ImGui::PopStyleVar();
}

std::string tmt::Viewport::get_title() const {
    const auto window_id = "###" ICON_MS_VISIBILITY " Viewport";
    const auto scene_name = engine.scenes.is_scene_loaded() ? engine.scenes.get_active_scene_info().name : "UNKNOWN";
    return scene_name + window_id;
}
int tmt::Viewport::get_window_flags() const {
    if (engine.game_controller.is_running()) return ImGuiWindowFlags_None;

    const bool is_scene_dirty = editor.windows[editor.editor_mode].get<ScenesWindow>().is_scene_dirty();
    return is_scene_dirty ? ImGuiWindowFlags_UnsavedDocument : 0;
};

void tmt::Viewport::display() {
    auto size = ImGui::GetContentRegionAvail();
    size = ImVec2(std::max(size.x, 1.0f), std::max(size.y, 1.0f));
    width = size.x;
    height = size.y;

    const ImVec2 image_pos = ImGui::GetCursorScreenPos();

    if (height <= 0.f) return;

    engine.renderer.render_view.set_viewport_size(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    ImGui::BeginChild("viewport_render", ImVec2(0, 0), 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    ImGui::Image((ImTextureRef)engine.renderer.render_view.imgui_viewport, size);
    is_hovered = ImGui::IsItemHovered();
    auto imgui_mouse_pos = ImGui::GetMousePos();
    mouse_pos.x = imgui_mouse_pos.x - image_pos.x;
    mouse_pos.y = imgui_mouse_pos.y - image_pos.y;

    const std::vector<Entity>& selected_entities = editor.windows[editor.editor_mode].get<Hierarchy>().get_selected_entities();

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
        }
    }

    // Build modifiers for bounds manipulation
    BoundsModifiers modifiers;
    modifiers.uniform_scale = shift_held;    // Shift: maintain aspect ratio
    modifiers.scale_from_center = alt_held;  // Alt: keep center fixed

    const bool gizmo_changed = editor.gizmo.manip(image_pos.x, image_pos.y, width, height, selected_entities, snap_value, modifiers);
    if (gizmo_changed) OnSceneModified::dispatch();

    const bool toolbar_buttons_hovered = toolbar(image_pos);

    selection_logic(imgui_mouse_pos, image_pos, toolbar_buttons_hovered);

    ImGui::EndChild();

    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        ImGui::SetWindowFocus();
    }
}

bool tmt::Viewport::toolbar(const ImVec2& image_pos) {
    ImGuiStyle& style = ImGui::GetStyle();

    float btn_w = ImGui::CalcTextSize(ICON_MS_LANGUAGE).x + style.FramePadding.x * 2.0f;
    float btn_h = ImGui::GetFrameHeight();
    float total_w = btn_w * 3.0f + style.ItemSpacing.x;

    // Small padding from the image border
    float pad = 4.0f;
    ImVec2 btn_pos = ImVec2(image_pos.x + width - total_w - pad, image_pos.y + pad);

    bool any_button_hovered = false;

    ImGui::SetCursorScreenPos(btn_pos);
    const char* space_button_icon = editor.gizmo.space ? ICON_MS_LANGUAGE : ICON_MS_VIEW_IN_AR;
    if (ImGui::Button(space_button_icon, ImVec2(btn_w, btn_h))) {
        editor.gizmo.space = static_cast<uint8_t>(!editor.gizmo.space);
    }
    any_button_hovered |= ImGui::IsItemHovered();

    ImGui::SameLine();
    const char* mode_button_icon = Gizmo::gizmo_op_icons[editor.gizmo.operation];
    if (ImGui::Button(mode_button_icon, ImVec2(btn_w, btn_h))) {
        ++editor.gizmo.operation;
        if (editor.gizmo.operation == Gizmo::GIZMO_OP_COUNT) editor.gizmo.operation = 0;
    }
    any_button_hovered |= ImGui::IsItemHovered();

    ImGui::SameLine();
    const char* multi_button_icon = editor.gizmo.multiselect_mode ? ICON_MS_FILTER_NONE : ICON_MS_FILTER_1;
    if (ImGui::Button(multi_button_icon, ImVec2(btn_w, btn_h))) {
        editor.gizmo.multiselect_mode = static_cast<uint8_t>(!editor.gizmo.multiselect_mode);
    }
    any_button_hovered |= ImGui::IsItemHovered();

    return any_button_hovered;
}

void tmt::Viewport::selection_logic(const ImVec2& imgui_mouse_pos, const ImVec2& image_pos, bool toolbar_buttons_hovered) {
    if (!is_hovered || engine.game_controller.is_running()) return;

    auto& hierarchy = editor.windows[Editor::Mode::SCENE].get<Hierarchy>();
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
        if (abs(delta.x) + abs(delta.y) > 5) {
            using_rect = true;

            ImGui::GetWindowDrawList()->AddRectFilled(first_click_pos, imgui_io.MousePos, ImColor(1.f, 1.f, 1.f, 0.2f));
            ImGui::GetWindowDrawList()->AddRect(first_click_pos, imgui_io.MousePos, ImColor(1.f, 1.f, 1.f, 0.6f));

            glm::ivec2 a = { first_click_pos.x - image_pos.x, first_click_pos.y - image_pos.y };
            glm::ivec2 b = { imgui_mouse_pos.x - image_pos.x, imgui_mouse_pos.y - image_pos.y };

            int x0 = std::min(a.x, b.x);
            int x1 = std::max(a.x, b.x);
            int y0 = std::min(a.y, b.y);
            int y1 = std::max(a.y, b.y);

            auto view = engine.ecs.get_registry().view<UIComponent>();
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

        if(engine.renderer.ui_pipeline.render_ui_pipeline)
        {
            auto view = engine.ecs.get_registry().view<UIComponent>();
            for (auto [entity, ui_comp] : view.each()) {
                if (AnchorHelper::is_inside(entity, { mouse_pos.x, mouse_pos.y })) {
                    hierarchy.add_entity_to_selection(entity);
                    return;
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
    auto& hierarchy = editor.windows[Editor::Mode::SCENE].get<Hierarchy>();

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

void tmt::Viewport::update_debug_camera(const tmt::FrameData& time) {
    if (engine.game_controller.is_running() || is_hovered == false) return;
    // Gather Variables to be used
    auto& input = engine.input;

    const bool enable_mouse_look = input.is_action_pressed(action::RIGHT_CLICK);
    const bool is_2d_axis_movement = input.is_action_pressed(action::LEFT_CLICK);

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

    const float dx = input.get_mouse_delta_x();
    const float dy = input.get_mouse_delta_y();

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

    const float mouse_wheel_y_delta = input.get_mouse_wheel_y();
    camera_speed *= std::pow(2.0f, mouse_wheel_y_delta * 0.15f);
    camera_speed = glm::clamp(camera_speed, Config::MIN_BASE_SPEED, Config::MAX_BASE_SPEED);

    glm::vec3 pos = transform.get_world_position();

    if (is_2d_axis_movement) {
        // 2D Axis Movement
        const bool sprint = input.is_action_pressed(Config::SPRINT);
        const glm::vec3 horizontal_move = transform.get_right() * dx * Config::MOUSE_SENSITIVITY * 0.1f;
        const glm::vec3 direction = sprint ? transform.get_forward() : transform.get_up();
        const glm::vec3 vertical_move = direction * -dy * Config::MOUSE_SENSITIVITY * 0.1f;
        pos += (horizontal_move + vertical_move);
    } else {
        // Movement (WASD + QE)
        glm::vec3 move_dir = { 0.0f, 0.0f, 0.0f };
        if (input.is_action_pressed(Config::FORWARD)) move_dir += transform.get_forward();
        if (input.is_action_pressed(Config::BACKWARD)) move_dir -= transform.get_forward();
        if (input.is_action_pressed(Config::LEFT)) move_dir -= transform.get_right();
        if (input.is_action_pressed(Config::RIGHT)) move_dir += transform.get_right();
        if (input.is_action_pressed(Config::UP)) move_dir += transform.get_up();
        if (input.is_action_pressed(Config::DOWN)) move_dir -= transform.get_up();

        if (glm::length(move_dir) > 0.0f) pos += glm::normalize(move_dir) * camera_speed * time.delta_time;
    }

    transform.set_world_position(pos);
}