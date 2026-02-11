#include "model_viewer.hpp"

#include "editor.hpp"
#include "ImGuizmo.h"
#include "viewport.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/input/input.hpp"
#include "engine/tools/file_dialog.hpp"

#include "editor/windows/palette.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "editor/windows/brush.hpp"

namespace tmt {

namespace {

/* Ray AABB intersection function. */
float intersect_aabb(const Ray& ray, const glm::vec3 box_min, const glm::vec3 box_max) {
    const glm::vec3 t_to_min = (box_min - ray.origin) * ray.rcp_dir;
    const glm::vec3 t_to_max = (box_max - ray.origin) * ray.rcp_dir;
    const glm::vec3 t_min = glm::min(t_to_min, t_to_max);
    const glm::vec3 t_max = glm::max(t_to_min, t_to_max);
    const float t_near = glm::max(glm::max(glm::max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = glm::min(glm::min(t_max.x, t_max.y), t_max.z);

    return t_near > t_far ? BIG_F32 : t_far;
}

// Draw the grid of a bounding box face.
void draw_face_grid(const glm::vec3& start, const glm::vec3& right, const glm::vec3& up, const float length, const float height) {
    glm::vec3 point_one = start;
    glm::vec3 point_two = start + up * height;
    const glm::vec3 right_vector = right / static_cast<float>(VOXELS_PER_UNIT);
    const size_t x_line_count = static_cast<size_t>(std::ceil(length * static_cast<float>(VOXELS_PER_UNIT))) - 1;
    for (size_t i = 0; i < x_line_count; i++) {
        point_one += right_vector;
        point_two += right_vector;

        engine.polyline.draw_line(point_one, point_two);
    }

    point_one = start;
    point_two = start + right * length;
    const glm::vec3 up_vector = up / static_cast<float>(VOXELS_PER_UNIT);
    const size_t y_line_count = static_cast<size_t>(std::ceil(height * static_cast<float>(VOXELS_PER_UNIT))) - 1;
    for (size_t i = 0; i < y_line_count; i++) {
        point_one += up_vector;
        point_two += up_vector;

        engine.polyline.draw_line(point_one, point_two);
    }
}

struct FaceData {
    float normal_sign { 0.0f };
    int32_t normal_index { -1 };

    glm::vec3 world_normal {};
    glm::vec3 face_center {};
};

// Use the current face index and selection transform to calculate the necessary data for drawing later.
FaceData calculate_face_info(const int32_t face_index, const Transform& transform, const glm::vec3& half_extent) {
    const std::div_t& result = std::div(face_index, 2);

    glm::vec3 face_normal {};
    const float sign = (result.rem == 0 ? 1.0f : -1.0f);
    face_normal[result.quot] = sign;

    FaceData info { .normal_sign = sign, .normal_index = result.quot };
    info.world_normal = transform.get_world_matrix() * glm::vec4 { face_normal, 0.0f };
    info.face_center = transform.get_world_matrix() * glm::vec4 { face_normal * half_extent, 1.0f };

    return info;
}

// Draw a single face of a voxel grid bounding box.
bool draw_face(const FaceData& info, const Transform& transform, const glm::vec3& half_extent) {
    const glm::vec3 camera_position = engine.renderer.get_debug_transform().get_world_position();
    if (glm::dot(info.world_normal, camera_position - info.face_center) >= 0.0f) return false;

    glm::vec3 box_half_extent = half_extent;
    box_half_extent[info.normal_index] = 0.0f;

    engine.polyline.use_line_width(2.0f, true);
    engine.polyline.use_color(glm::vec4 { 0.8f, 0.8f, 0.8f, 1.0f });

    engine.polyline.draw_obb(info.face_center, box_half_extent, transform.get_world_rotation());

    return true;
}

// Do some additional calculations to get the necessary info to draw the grid of the face that was hit/pointed to.
void draw_selected_face(const FaceData& info, const glm::vec3& half_extent, const Transform& transform) {
    glm::vec3 min = -half_extent;
    min[info.normal_index] = half_extent[info.normal_index] * info.normal_sign;
    min = transform.get_world_matrix() * glm::vec4 { min, 1.0f };

    engine.polyline.use_line_width(4.0f, false);
    engine.polyline.use_color(glm::vec4 { 0.8f, 0.8f, 0.8f, 0.4f });
    switch (info.normal_index) {
        case 0:
            draw_face_grid(min, transform.get_forward(), transform.get_up(), half_extent[2] * 2.0f, half_extent[1] * 2.0f);
            break;

        case 1:
            draw_face_grid(min, transform.get_right(), transform.get_forward(), half_extent[0] * 2.0f, half_extent[2] * 2.0f);
            break;

        case 2:
            draw_face_grid(min, transform.get_right(), transform.get_up(), half_extent[0] * 2.0f, half_extent[1] * 2.0f);
            break;

        default:
            break;
    }
}

void draw_selection(const Hit& hit, const glm::vec3& half_extent, const Transform& transform) {
    engine.polyline.use_line_width(2.0f);
    engine.polyline.use_color(glm::vec4 { 1.0f, 0.5f, 0.5f, 1.0f });

    const glm::vec3 local_voxel_pos = glm::vec3 { hit.coord } * UNITS_PER_VOXEL - half_extent + VOXEL_SIZE_HALF;
    const glm::vec3 world_voxel_pos = transform.get_world_matrix() * glm::vec4 { local_voxel_pos, 1.0f };

    engine.polyline.draw_obb(world_voxel_pos, glm::vec3 { VOXEL_SIZE_HALF }, transform.get_world_rotation());
}

bool handle_selection(const Ray& mouse_ray, Hit& hit, const bool can_hit_face, const Entity entity, const Transform& transform, const ResourceRef<VoxelVolume>& resource) {
    const glm::vec3 half_extent = glm::vec3 { resource->size } * VOXEL_SIZE_HALF;
    const glm::mat4 world_to_local_matrix = glm::inverse(transform.get_world_matrix());

    Ray local_ray {};
    float far = 0.0f;
    bool is_valid_hit = false;
    if (can_hit_face) {
        // If we can hit a face, that means we want to work on top of the voxel the mouse is pointing at, we get the voxel coord by adjusting it here.
        const glm::vec3 local_normal = world_to_local_matrix * glm::vec4 { hit.normal, 0.0f };
        const glm::uvec3 voxel_grid_normal { glm::round(local_normal) };
        hit.coord += voxel_grid_normal;

        if (glm::any(glm::greaterThanEqual(hit.coord, resource->size))) hit.entity = entt::null;

        // Create a local version of the ray to simplify the aabb test later.
        local_ray = Ray {
            world_to_local_matrix * glm::vec4 { mouse_ray.origin, 1.0f },
            world_to_local_matrix * glm::vec4 { mouse_ray.dir, 0.0f },
        };

        // Do an aabb test with the bounding box of the voxel object, this tells use if the user is pointing at a face of the grid, and thus we should draw the grid.
        far = intersect_aabb(local_ray, -half_extent, half_extent);
        is_valid_hit = (far > 0.0f && hit.distance >= far);
    }

    // Loop over all the grid's outer faces and draw them.
    for (int32_t i = 0; i < 6; i++) {
        FaceData face = calculate_face_info(i, transform, half_extent);
        if (!draw_face(face, transform, half_extent) || !is_valid_hit) continue;  // Exit if we didn't draw the face (the face can't be viewed and thus can't be selected).

        // Check if the displayed face was the face that was hit, then update the "hit" variable's members with the new info.
        const glm::vec3 local_hit_position = local_ray.origin + local_ray.dir * far;
        const float extent_axis = half_extent[face.normal_index] * face.normal_sign;

        constexpr float SMALL_FLOAT = std::numeric_limits<float>::epsilon() * 100.0f;

        const bool is_hit_deeper = (glm::distance(local_ray.origin, local_hit_position) > hit.distance);
        const bool is_hitting_face = (glm::distance(local_hit_position[face.normal_index], extent_axis) < SMALL_FLOAT);
        if (is_hit_deeper || !is_hitting_face) continue;

        hit.entity = entity;
        hit.distance = far;

        const glm::vec3 position = (mouse_ray.origin + mouse_ray.dir * hit.distance) - (face.world_normal * VOXEL_SIZE_HALF);
        const glm::vec3 local_position = glm::inverse(transform.get_world_matrix()) * glm::vec4 { position, 1.0f };

        hit.coord = (local_position + half_extent) * static_cast<float>(VOXELS_PER_UNIT);

        draw_selected_face(face, half_extent, transform);
    }

    if (hit.entity != entity) return false;

    draw_selection(hit, half_extent, transform);

    return true;
}

// Update the voxels based on the current selected brush.
void modify_voxels(const Brush::Mode brush_mode, const ResourceRef<VoxelVolume>& model, const Hit& hit) {
    switch (brush_mode) {
        case Brush::Mode::PAINT: {
            if (!engine.input.is_mouse_button_pressed(MouseButton::LEFT)) break;

            const MaterialIndex material_index = editor.windows[Editor::Mode::VOXEL].get<Palette>().get_selected_material_index();
            model->blas->set_voxel(hit.coord.x, hit.coord.y, hit.coord.z, material_index);

            model->set_dirty();
            break;
        }

        case Brush::Mode::COLOR_PICKER: {
            if (!engine.input.is_mouse_button_pressed(MouseButton::LEFT)) break;

            const Material* picked_material = model->blas->get_voxel(hit.coord.x, hit.coord.y, hit.coord.z);
            if (picked_material == nullptr) break;

            editor.windows[Editor::Mode::VOXEL].get<Palette>().set_selected_material_index(static_cast<MaterialIndex>(picked_material - model->blas->palette.entries));
            break;
        }

        case Brush::Mode::ADD: {
            if (!engine.input.is_mouse_button_just_pressed(MouseButton::LEFT)) break;

            const MaterialIndex material_index = editor.windows[Editor::Mode::VOXEL].get<Palette>().get_selected_material_index();

            model->blas->set_voxel(hit.coord.x, hit.coord.y, hit.coord.z, material_index);
            model->set_dirty();
            break;
        }

        case Brush::Mode::REMOVE: {
            if (!engine.input.is_mouse_button_just_pressed(MouseButton::LEFT)) break;

            model->blas->remove_voxel(hit.coord.x, hit.coord.y, hit.coord.z);
            model->set_dirty();
            break;
        }

        default:
            break;
    }
}

}  // namespace

void ModelViewer::display() {
    const ImVec2 content_start { 0.0f, ImGui::GetFrameHeight() };
    ImGui::SetCursorPos(content_start);

    const ImVec2 size = ImGui::GetWindowSize() - content_start;
    if (size.x <= 0.0f || size.y <= 0.0f) return;

    const ImVec2 window_pos = ImGui::GetWindowPos() + ImVec2 { 0.0f, ImGui::GetFrameHeight() };

    engine.renderer.render_view.set_viewport_size(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
    ImGui::Image(engine.renderer.render_view.imgui_viewport, size);

    const Brush::Mode brush_mode = editor.windows[Editor::Mode::VOXEL].get<Brush>().get_active_mode();
    if (brush_mode == Brush::Mode::MULTI_TOOL) {
        const Entity selected_entity = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().get_selected_entity();

        editor.gizmo.manip(window_pos.x, window_pos.y, size.x, size.y, { &selected_entity, &selected_entity + 1 });

        if (!ImGuizmo::IsOver() && engine.input.is_mouse_button_just_pressed(MouseButton::LEFT)) {
            const Ray mouse_ray = engine.renderer.render_view.pixel_ray(mouse_position);
            const Hit hit = engine.renderer.trace_ray(mouse_ray);

            editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>().set_selected_entity(hit.entity);
        }
    }

    // Set up the window and mouse variables in the viewport to update the camera movement in the update function later.
    Viewport& viewport = editor.windows[Editor::Mode::SCENE].get<Viewport>();
    viewport.is_hovered = ImGui::IsItemHovered();
    const ImVec2 mouse_pos = ImGui::GetMousePos() - ImGui::GetWindowPos();
    mouse_position.x = viewport.mouse_pos.x = mouse_pos.x - content_start.x;
    mouse_position.y = viewport.mouse_pos.y = mouse_pos.y - content_start.y;

    // Get the selected entity in the hierarchy and return early if it's invalid.
    NodeHierarchy& node_hierarchy = editor.windows[Editor::Mode::VOXEL].get<NodeHierarchy>();
    const Entity selected_entity = node_hierarchy.get_selected_entity();

    if (!engine.ecs.valid(selected_entity) || !engine.ecs.has_component<VoxelRenderer>(selected_entity)) return;

    // Get the necessary values for handling selection (entity transform, voxel resource, ray cast for selection, etc).
    const Transform& transform = engine.ecs.get_component<Transform>(selected_entity);
    const ResourceRef<VoxelVolume>& resource = engine.ecs.get_component<VoxelRenderer>(selected_entity).resource;

    const Ray mouse_ray = engine.renderer.render_view.pixel_ray(mouse_position);
    Hit hit = engine.renderer.trace_ray(mouse_ray);

    const bool can_hit_face = (brush_mode == Brush::Mode::ADD);

    // Handle drawing the selection and calculating the selected voxel position.
    if (!handle_selection(mouse_ray, hit, can_hit_face, selected_entity, transform, resource)) return;  // Return early if no valid voxel was selected.

    modify_voxels(brush_mode, resource, hit);
}

void ModelViewer::on_editor_update(const FrameData& time) {
    editor.windows[Editor::Mode::SCENE].get<Viewport>().update_debug_camera(time);
}

}  // namespace tmt