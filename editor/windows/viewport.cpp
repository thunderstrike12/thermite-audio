#include "viewport.hpp"
#include <imgui.h>
#include "editor/editor.hpp"
#include "hierarchy.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/input/input_map.hpp"
#include "editor/events/scene.hpp"
#include "editor/windows/scenes.hpp"

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

void tmt::Viewport::on_editor_update(const tmt::FrameData& frame_data) { update_debug_camera(frame_data); }

void tmt::Viewport::on_editor_end() {}

void tmt::Viewport::before_begin() {
    /*default min size*/
    ImGui::SetNextWindowSize(ImVec2(960, 540), ImGuiCond_FirstUseEver);

    /* Zero margin */
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
}

void tmt::Viewport::end_display() { ImGui::PopStyleVar(); }

std::string tmt::Viewport::get_title() const {
    const auto window_id = "###" ICON_MS_VISIBILITY " Viewport";
    const auto scene_name = engine.scenes.is_scene_loaded() ? engine.scenes.get_active_scene_info().name : "UNKNOWN";
    return scene_name + window_id;
}
int tmt::Viewport::get_window_flags() const {
    if (engine.game_controller.is_running()) return ImGuiWindowFlags_None;

    const bool is_scene_dirty = editor.windows[Editor::Mode::SCENE].get<ScenesWindow>().is_scene_dirty();
    return is_scene_dirty ? ImGuiWindowFlags_UnsavedDocument : 0;
};

void tmt::Viewport::display() {
    auto size = ImGui::GetContentRegionAvail();
    size = ImVec2(std::max(size.x, 1.0f), std::max(size.y, 1.0f));
    width = size.x;
    height = size.y;

    const glm::vec2 image_pos = glm::vec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);

    if (height <= 0.f) return;

    engine.renderer.render_view.set_viewport_size(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    ImGui::BeginChild("viewport_render", ImVec2(0, 0), 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    ImGui::Image((ImTextureRef)engine.renderer.render_view.imgui_viewport, size);
    is_hovered = ImGui::IsItemHovered();
    auto imgui_mouse_pos = ImGui::GetMousePos();
    mouse_pos.x = imgui_mouse_pos.x - image_pos.x;
    mouse_pos.y = imgui_mouse_pos.y - image_pos.y;

    const std::vector<Entity>& selected_entities = editor.windows[Editor::Mode::SCENE].get<Hierarchy>().get_selected_entities();

    const bool gizmo_changed = editor.gizmo.manip(image_pos.x, image_pos.y, width, height, selected_entities);
    if (gizmo_changed) OnSceneModified::dispatch();

    toolbar(image_pos);

    ImGui::EndChild();

    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        ImGui::SetWindowFocus();
    }
}

void tmt::Viewport::toolbar(const glm::vec2& image_pos) {
    ImGuiStyle& style = ImGui::GetStyle();

    float btn_w = ImGui::CalcTextSize(ICON_MS_LANGUAGE).x + style.FramePadding.x * 2.0f;
    float btn_h = ImGui::GetFrameHeight();
    float total_w = btn_w * 3.0f + style.ItemSpacing.x;

    // Small padding from the image border
    float pad = 4.0f;
    ImVec2 btn_pos = ImVec2(image_pos.x + width - total_w - pad, image_pos.y + pad);

    ImGui::SetCursorScreenPos(btn_pos);
    const char* space_button_icon = editor.gizmo.space ? ICON_MS_LANGUAGE : ICON_MS_VIEW_IN_AR;
    if (ImGui::Button(space_button_icon, ImVec2(btn_w, btn_h))) {
        editor.gizmo.space = static_cast<uint8_t>(!editor.gizmo.space);
    }

    ImGui::SameLine();
    const char* mode_button_icon = Gizmo::gizmo_op_icons[editor.gizmo.operation];
    if (ImGui::Button(mode_button_icon, ImVec2(btn_w, btn_h))) {
        ++editor.gizmo.operation;
        if (editor.gizmo.operation == Gizmo::GIZMO_OP_COUNT) editor.gizmo.operation = 0;
    }

    ImGui::SameLine();
    const char* multi_button_icon = editor.gizmo.multiselect_mode ? ICON_MS_FILTER_NONE : ICON_MS_FILTER_1;
    if (ImGui::Button(multi_button_icon, ImVec2(btn_w, btn_h))) {
        editor.gizmo.multiselect_mode = static_cast<uint8_t>(!editor.gizmo.multiselect_mode);
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
        return;
    }
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
        glm::vec3 move_dir = {0.0f, 0.0f, 0.0f};
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