#include "viewport.hpp"
#include <imgui.h>
#include "ImGuizmo.h"
#include "editor/editor.hpp"
#include "hierarchy.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/input/input.hpp"

void tmt::Viewport::on_editor_start() {
    ImGuizmo::AllowAxisFlip(false);

    // Setup Input Actions
    register_input_actions();

    setup_gizmo_style();
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

void tmt::Viewport::display() {
    auto size = ImGui::GetContentRegionAvail();
    size = ImVec2(std::max(size.x, 1.0f), std::max(size.y, 1.0f));
    width = size.x;
    height = size.y;

    const glm::vec2 image_pos = glm::vec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);

    ImGuizmo::SetRect(image_pos.x, image_pos.y, width, height);

    if (height <= 0.f) return;

    engine.renderer.render_view.set_viewport_size(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    ImGui::BeginChild("viewport_render", ImVec2(0, 0), 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    ImGui::Image((ImTextureRef)engine.renderer.render_view.imgui_viewport, size);
    is_hovered = ImGui::IsItemHovered();

    ImGuizmo::SetDrawlist();

    gizmo_manip();

    toolbar(image_pos);

    ImGui::EndChild();
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
    const char* space_button_icon = gizmo_space ? ICON_MS_LANGUAGE : ICON_MS_VIEW_IN_AR;
    if (ImGui::Button(space_button_icon, ImVec2(btn_w, btn_h))) {
        gizmo_space = static_cast<uint8_t>(!gizmo_space);
    }

    ImGui::SameLine();
    const char* mode_button_icon = gizmo_op_icons[gizmo_operation];
    if (ImGui::Button(mode_button_icon, ImVec2(btn_w, btn_h))) {
        ++gizmo_operation;
        if (gizmo_operation == GIZMO_OP_COUNT) gizmo_operation = 0;
    }

    ImGui::SameLine();
    const char* multi_button_icon = gizmo_multiselect_mode ? ICON_MS_FILTER_NONE : ICON_MS_FILTER_1;
    if (ImGui::Button(multi_button_icon, ImVec2(btn_w, btn_h))) {
        gizmo_multiselect_mode = static_cast<uint8_t>(!gizmo_multiselect_mode);
    }
}

void tmt::Viewport::gizmo_manip() {
    if (engine.game_controller.is_running()) return;

    auto& hierarchy = editor.windows.get<Hierarchy>();
    auto& selected_entities = hierarchy.get_selected_entities();
    if (selected_entities.empty()) return;

    Transform& transform = engine.renderer.get_debug_transform();
    Camera& camera = engine.renderer.get_debug_camera();

    auto view = glm::inverse(transform.get_world_matrix());
    const float aspect_ratio = width / height;
    auto perspective = glm::perspective(glm::radians(camera.fov), aspect_ratio, 0.05f, 1000.0f);

    // relative to first
    if (gizmo_multiselect_mode == 0) {
        Entity selected_entity = hierarchy.get_first_selected_entity();
        Transform& selected_transform = engine.ecs.get_component<Transform>(selected_entity);
        auto& selected_matrix = selected_transform.get_world_matrix();
        glm::mat4 imguizmo_input_matrix = selected_matrix;

        glm::mat4 delta;
        ImGuizmo::Manipulate(
            &view[0][0], &perspective[0][0], static_cast<ImGuizmo::OPERATION>(gizmo_operations[gizmo_operation]), static_cast<ImGuizmo::MODE>(gizmo_space), &imguizmo_input_matrix[0][0],
            &delta[0][0]
        );
        selected_transform.set_world_matrix(imguizmo_input_matrix);

        for (auto entity : selected_entities) {
            if (entity == selected_entity) continue;

            Transform& multi_select_transform = engine.ecs.get_component<Transform>(entity);
            auto& multi_matrix = multi_select_transform.get_world_matrix();

            multi_select_transform.set_world_matrix(delta * multi_matrix);
        }
    } else {  // relative to average
        uint32_t amount = static_cast<uint32_t>(selected_entities.size());

        glm::vec3 avg_translation = glm::vec3(0.f);
        glm::quat avg_rotation = glm::quat();
        glm::vec3 scale = glm::vec3(1.f);

        float weight = 1.f / static_cast<float>(amount);

        for (auto entity : selected_entities) {
            Transform& multi_select_transform = engine.ecs.get_component<Transform>(entity);
            auto& multi_matrix = multi_select_transform.get_world_matrix();

            avg_translation += glm::vec3(multi_matrix[3]);
            avg_rotation += weight * multi_select_transform.get_world_rotation();
        }
        avg_translation /= static_cast<float>(amount);
        avg_rotation = glm::normalize(avg_rotation);

        glm::mat4 avg = glm::translate(glm::mat4(1.f), avg_translation) * glm::mat4_cast(avg_rotation) * glm::scale(glm::mat4(1.f), scale);
        glm::mat4 delta;
        ImGuizmo::Manipulate(
            &view[0][0], &perspective[0][0], static_cast<ImGuizmo::OPERATION>(gizmo_operations[gizmo_operation]), static_cast<ImGuizmo::MODE>(gizmo_space), &avg[0][0], &delta[0][0]
        );
        for (auto entity : selected_entities) {
            Transform& multi_select_transform = engine.ecs.get_component<Transform>(entity);
            auto& multi_matrix = multi_select_transform.get_world_matrix();

            multi_select_transform.set_world_matrix(delta * multi_matrix);
        }
    }
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

void tmt::Viewport::setup_gizmo_style() {
    using namespace ImGuizmo;
    // style..
    auto& style = ImGuizmo::GetStyle();
    style.TranslationLineThickness = 7.0f;
    style.TranslationLineArrowSize = 11.0f;
    style.RotationLineThickness = 5.0f;
    style.RotationOuterLineThickness = 6.0f;
    style.ScaleLineThickness = 7.0f;
    style.ScaleLineCircleSize = 11.0f;
    style.HatchedAxisLineThickness = 5.0f;
    style.CenterCircleSize = 10.0f;

    // Slightly neon, softer axis colors
    style.Colors[DIRECTION_X] = ImVec4(0.98f, 0.36f, 0.47f, 1.00f);  // soft coral red
    style.Colors[DIRECTION_Y] = ImVec4(0.39f, 0.86f, 0.55f, 1.00f);  // mint/teal
    style.Colors[DIRECTION_Z] = ImVec4(0.38f, 0.67f, 0.98f, 1.00f);  // sky blue

    // Matching planes, with lower alpha
    style.Colors[PLANE_X] = ImVec4(0.98f, 0.36f, 0.47f, 0.28f);
    style.Colors[PLANE_Y] = ImVec4(0.39f, 0.86f, 0.55f, 0.28f);
    style.Colors[PLANE_Z] = ImVec4(0.38f, 0.67f, 0.98f, 0.28f);

    // Selection: warm golden accent
    style.Colors[SELECTION] = ImVec4(1.00f, 0.84f, 0.39f, 0.85f);

    // Inactive: cooler desaturated grey-blue
    style.Colors[INACTIVE] = ImVec4(0.32f, 0.35f, 0.42f, 0.85f);

    // Lines: slightly brighter/cleaner on dark background
    style.Colors[TRANSLATION_LINE] = ImVec4(0.60f, 0.63f, 0.70f, 0.80f);
    style.Colors[SCALE_LINE] = ImVec4(0.92f, 0.92f, 0.96f, 0.90f);

    // Rotation highlight: vivid magenta/orange mix
    style.Colors[ROTATION_USING_BORDER] = ImVec4(1.00f, 0.50f, 0.78f, 1.00f);
    style.Colors[ROTATION_USING_FILL] = ImVec4(1.00f, 0.50f, 0.78f, 0.45f);

    // Hatched axis lines: subtle, light on dark
    style.Colors[HATCHED_AXIS_LINES] = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);

    // Text: soft white with subtle shadow
    style.Colors[TEXT] = ImVec4(0.96f, 0.97f, 0.99f, 1.00f);
    style.Colors[TEXT_SHADOW] = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
}
