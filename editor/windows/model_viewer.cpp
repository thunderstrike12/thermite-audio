#include "model_viewer.hpp"

#include "editor.hpp"
#include "ImGuizmo.h"
#include "viewport.hpp"
#include "core/systems/undo_redo/voxel_edit_diff.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/tools/file_dialog.hpp"

#include "editor/windows/palette.hpp"
#include "editor/windows/node_hierarchy.hpp"
#include "editor/windows/brush.hpp"
#include "editor/shared/colors.hpp"

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
    glm::vec3 p1 = start;
    glm::vec3 p2 = start + up * height;

    const glm::vec3 right_vector = right * UNITS_PER_VOXEL;
    const size_t x_line_count = static_cast<size_t>(std::ceil(length * static_cast<float>(VOXELS_PER_UNIT))) - 1;
    for (size_t i = 0; i < x_line_count; i++) {
        p1 += right_vector;
        p2 += right_vector;

        engine.polyline.draw_line(p1, p2);
    }

    p1 = start;
    p2 = start + right * length;

    const glm::vec3 up_vector = up * UNITS_PER_VOXEL;
    const size_t y_line_count = static_cast<size_t>(std::ceil(height * static_cast<float>(VOXELS_PER_UNIT))) - 1;
    for (size_t i = 0; i < y_line_count; i++) {
        p1 += up_vector;
        p2 += up_vector;

        engine.polyline.draw_line(p1, p2);
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
bool draw_face(const FaceData& info, const Transform& transform, const glm::vec3& half_extent, const glm::vec3 color = glm::vec3(1.0f)) {
    const glm::vec3 camera_position = engine.renderer.get_debug_transform().get_world_position();
    if (glm::dot(info.world_normal, camera_position - info.face_center) >= 0.0f) return false;

    glm::vec3 box_half_extent = half_extent * transform.get_world_scale();
    box_half_extent[info.normal_index] = 0.0f;

    // Set up the local points of the face.
    glm::vec3 p0;
    const int32_t i0 = info.normal_index;
    const int32_t i1 = (i0 + 1) % 3;
    const int32_t i2 = 3 - i0 - i1;
    p0[i0] = half_extent[i0] * info.normal_sign;
    p0[i1] = half_extent[i1];
    p0[i2] = half_extent[i2];

    glm::vec3 p1 = p0;
    p1[i1] *= -1.0f;

    glm::vec3 p2 = p1;
    p2[i2] *= -1.0f;

    glm::vec3 p3 = p2;
    p3[i1] *= -1.0f;

    // Convert the local points to world space.
    const glm::mat4& world_matrix = transform.get_world_matrix();
    p0 = world_matrix * glm::vec4 { p0, 1.0f };
    p1 = world_matrix * glm::vec4 { p1, 1.0f };
    p2 = world_matrix * glm::vec4 { p2, 1.0f };
    p3 = world_matrix * glm::vec4 { p3, 1.0f };

    // Draw the lines between the points to make the grid face.
    engine.polyline.use_line_width(2.0f, true);
    engine.polyline.use_color(color);

    engine.polyline.draw_line(p0, p1);
    engine.polyline.draw_line(p1, p2);
    engine.polyline.draw_line(p2, p3);
    engine.polyline.draw_line(p3, p0);

    return true;
}

// Do some additional calculations to get the necessary info to draw the grid of the face that was hit/pointed to.
void draw_selected_face(const FaceData& info, const glm::vec3& half_extent, const Transform& transform) {
    glm::vec3 min = -half_extent;
    min[info.normal_index] = half_extent[info.normal_index] * info.normal_sign;
    min = transform.get_world_matrix() * glm::vec4 { min, 1.0f };

    engine.polyline.use_line_width(4.0f, false);
    engine.polyline.use_color(glm::vec4 { 0.8f, 0.8f, 0.8f, 0.4f });

    const glm::vec3 scaled_right = transform.get_right() * transform.get_world_scale();
    const glm::vec3 scaled_up = transform.get_up() * transform.get_world_scale();
    const glm::vec3 scaled_forward = transform.get_forward() * transform.get_world_scale();

    switch (info.normal_index) {
        case 0:
            draw_face_grid(min, scaled_forward, scaled_up, half_extent[2] * 2.0f, half_extent[1] * 2.0f);
            break;

        case 1:
            draw_face_grid(min, scaled_right, scaled_forward, half_extent[0] * 2.0f, half_extent[2] * 2.0f);
            break;

        case 2:
            draw_face_grid(min, scaled_right, scaled_up, half_extent[0] * 2.0f, half_extent[1] * 2.0f);
            break;

        default:
            break;
    }
}

void draw_selection(const Hit& hit, const glm::vec3& half_extent, const Transform& transform, const glm::vec3& color) {
    engine.polyline.use_line_width(3.0f);
    engine.polyline.use_color(color);

    const glm::vec3 local_voxel_pos = glm::vec3 { hit.coord } * UNITS_PER_VOXEL - half_extent + VOXEL_SIZE_HALF;
    const glm::vec3 world_voxel_pos = transform.get_world_matrix() * glm::vec4 { local_voxel_pos, 1.0f };

    const glm::vec3 local_scale = transform.get_world_scale();

    engine.polyline.draw_obb(world_voxel_pos, VOXEL_SIZE_HALF * local_scale, transform.get_world_rotation());
}

std::vector<FaceData> draw_valid_faces(const Transform& transform, const glm::vec3& half_extent, const glm::vec3 color = glm::vec3(1.0f)) {
    std::vector<FaceData> valid_faces;

    for (int32_t i = 0; i < 6; i++) {
        FaceData face = calculate_face_info(i, transform, half_extent);

        if (draw_face(face, transform, half_extent, color)) valid_faces.push_back(face);  // Don't add the face to the vector if `draw_face` returned false aka we can't see it.
    }

    return valid_faces;
}

bool handle_selection(
    const std::vector<FaceData>& valid_faces, const Brush::State brush_state, const Ray& ray, Hit& hit, const Entity entity, const Transform& transform, const glm::vec3& half_extent
) {
    const glm::mat4 world_to_local_matrix = glm::inverse(transform.get_world_matrix());

    // Create a local version of the ray to simplify the aabb test later.
    const Ray local_ray {
        world_to_local_matrix * glm::vec4 { ray.origin, 1.0f },
        world_to_local_matrix * glm::vec4 { ray.dir, 0.0f },
    };

    // Do an aabb test with the bounding box of the voxel object, this tells use if the user is pointing at a face of the grid, and thus we should draw the grid.
    const float far = intersect_aabb(local_ray, -half_extent, half_extent);
    const bool is_face_hit = (far > 0.0f && hit.distance >= far);

    if (!is_face_hit) {
        if (brush_state.tool != Brush::Tool::COLOR_PICKER && brush_state.mode == Brush::Mode::ATTACH) {
            // If we can hit a face, that means we want to work on top of the voxel the mouse is pointing at, we get the voxel coord by adjusting it here.
            const glm::vec3 local_normal = glm::normalize(glm::vec3(world_to_local_matrix * glm::vec4 { hit.normal, 0.0f }));
            const glm::uvec3 voxel_grid_normal { glm::round(local_normal) };
            hit.coord += voxel_grid_normal;
        }

        // If we know we didn't hit a face, we can early return by checking if the hit was on this entity.
        return hit.entity == entity;
    }

    // Loop over all the valid grid faces to see which one was hit.
    const glm::vec3 local_hit_position = local_ray.origin + local_ray.dir * far;
    for (const FaceData& face : valid_faces) {
        // Check if the displayed face was the face that was hit, then update the "hit" variable's members with the new info.
        const float extent_axis = half_extent[face.normal_index] * face.normal_sign;

        constexpr float SMALL_FLOAT = std::numeric_limits<float>::epsilon() * 100.0f;

        const bool is_hit_deeper = (glm::distance(local_ray.origin, local_hit_position) > hit.distance);
        const bool is_hitting_face = (glm::distance(local_hit_position[face.normal_index], extent_axis) < SMALL_FLOAT);
        if (is_hit_deeper || !is_hitting_face) continue;

        hit.entity = entity;
        hit.distance = far;

        const glm::vec3 world_hit_position = (ray.origin + ray.dir * hit.distance) - (face.world_normal * VOXEL_SIZE_HALF);
        const glm::vec3 local_position = world_to_local_matrix * glm::vec4 { world_hit_position, 1.0f };

        hit.coord = (local_position + half_extent) * static_cast<float>(VOXELS_PER_UNIT);

        draw_selected_face(face, half_extent, transform);
    }

    const bool can_interact_with_face = (brush_state.tool != Brush::Tool::COLOR_PICKER && (brush_state.mode == Brush::Mode::ATTACH || brush_state.tool == Brush::Tool::BOX));
    return hit.entity == entity && can_interact_with_face;
}

void set_voxel(const std::unique_ptr<Svt64>& svt, const glm::uvec3& coord, VoxelEditDiff& diff, const MaterialIndex material_index) {
    const Material* voxel_material = svt->get_voxel(coord.x, coord.y, coord.z);
    const MaterialIndex voxel_material_index = svt->palette.material_to_index(voxel_material);
    const bool is_voxel_empty = (voxel_material == nullptr);

    if (!is_voxel_empty && voxel_material_index == material_index) return;

    svt->set_voxel(coord.x, coord.y, coord.z, material_index);

    diff.add_change(coord, (is_voxel_empty ? nullptr : &voxel_material_index), &material_index);
}

void remove_voxel(const std::unique_ptr<Svt64>& svt, const glm::uvec3& coord, VoxelEditDiff& diff) {
    const Material* voxel_material = svt->get_voxel(coord.x, coord.y, coord.z);
    const MaterialIndex voxel_material_index = svt->palette.material_to_index(voxel_material);
    const bool is_voxel_empty = (voxel_material == nullptr);

    if (is_voxel_empty) return;

    svt->remove_voxel(coord.x, coord.y, coord.z);

    diff.add_change(coord, (is_voxel_empty ? nullptr : &voxel_material_index));
}

void paint_voxel(const std::unique_ptr<Svt64>& svt, const glm::uvec3& coord, VoxelEditDiff& diff, const MaterialIndex material_index) {
    const Material* voxel_material = svt->get_voxel(coord.x, coord.y, coord.z);
    const bool is_voxel_empty = (voxel_material == nullptr);
    if (is_voxel_empty) return;

    const MaterialIndex voxel_material_index = svt->palette.material_to_index(voxel_material);
    if (voxel_material_index == material_index) return;

    svt->set_voxel(coord.x, coord.y, coord.z, material_index);

    diff.add_change(coord, &voxel_material_index, &material_index);
}

// Iterate the selected volume of voxels to: attach, remove or paint any of the voxels in that volume (handles undo/redo as well).
void modify_voxel_volume(const ResourceRef<VoxelVolume>& model, const Brush::Mode brush_mode, const glm::uvec3& min, const glm::uvec3& max, const MaterialIndex material_index) {
    VoxelEditDiff diff { model->uuid };

    for (uint32_t x = min.x; x <= max.x; x++) {
        for (uint32_t y = min.y; y <= max.y; y++) {
            for (uint32_t z = min.z; z <= max.z; z++) {
                const glm::uvec3 coord { x, y, z };

                switch (brush_mode) {
                    case Brush::Mode::ATTACH:
                        set_voxel(model->blas, coord, diff, material_index);
                        break;

                    case Brush::Mode::REMOVE:
                        remove_voxel(model->blas, coord, diff);
                        break;

                    case Brush::Mode::PAINT:
                        paint_voxel(model->blas, coord, diff, material_index);
                        break;
                }
            }
        }
    }

    model->set_dirty();

    switch (brush_mode) {
        case Brush::Mode::ATTACH:
            diff.commit("Attach Voxel(s)");
            break;

        case Brush::Mode::REMOVE:
            diff.commit("Remove Voxel(s)");
            break;

        case Brush::Mode::PAINT:
            diff.commit("Paint Voxel(s)");
            break;
    }
}

// Update the voxels based on the current selected brush.
void handle_brush(const Brush::State brush_state, const ResourceRef<VoxelVolume>& model, const Hit& hit, const MaterialIndex material_index) {
    switch (brush_state.tool) {
        case Brush::Tool::COLOR_PICKER: {
            if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) break;

            const Material* picked_material = model->blas->get_voxel(hit.coord.x, hit.coord.y, hit.coord.z);
            if (picked_material == nullptr) break;

            Palette& palette = editor.systems[Editor::Mode::VOXEL].get<Palette>();
            palette.set_selected_material_index(model->blas->palette.material_to_index(picked_material));
            break;
        }

        case Brush::Tool::SINGLE: {
            if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) break;

            VoxelEditDiff diff { model->uuid };
            switch (brush_state.mode) {
                case Brush::Mode::ATTACH:
                    set_voxel(model->blas, hit.coord, diff, material_index);
                    diff.commit("Attach Voxel(s)");
                    break;

                case Brush::Mode::REMOVE:
                    remove_voxel(model->blas, hit.coord, diff);
                    diff.commit("Remove Voxel(s)");
                    break;

                case Brush::Mode::PAINT:
                    paint_voxel(model->blas, hit.coord, diff, material_index);
                    diff.commit("Paint Voxel(s)");
                    break;
            }
            model->set_dirty();

            break;
        }

        case Brush::Tool::BOX: {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseReleased(ImGuiMouseButton_Left)) break;

            static glm::uvec3 start_coord { std::numeric_limits<uint32_t>::max() };
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) start_coord = hit.coord;
            const glm::uvec3& end_coord = hit.coord;

            // Find the bounded start and end coordinates.
            const glm::uvec3 start_bounded = glm::min(start_coord, model->size - 1u);
            const glm::uvec3 end_bounded = glm::min(end_coord, model->size - 1u);

            // Calculate the half extent of the selected volume of voxels (requires min and max to correctly account for the start and end voxels).
            const glm::uvec3 min = glm::min(start_bounded, end_bounded);
            const glm::uvec3 max = glm::max(start_bounded, end_bounded);

            const Transform& transform = engine.ecs.get_component<Transform>(hit.entity);
            const glm::vec3 half_extent = glm::vec3 { model->size } * VOXEL_SIZE_HALF;

            // Find the world min and max position, use those to find the center and half extent.
            const glm::vec3 world_min = transform.get_world_matrix() * glm::vec4 { glm::vec3(min) * UNITS_PER_VOXEL - half_extent, 1.0f };
            const glm::vec3 world_max = transform.get_world_matrix() * glm::vec4 { glm::vec3(max + 1u) * UNITS_PER_VOXEL - half_extent, 1.0f };
            const glm::vec3 select_half_extent = glm::vec3(max - min + 1u) * VOXEL_SIZE_HALF * transform.get_world_scale();
            const glm::vec3 select_center = (world_min + world_max) * 0.5f;

            engine.polyline.draw_obb(select_center, select_half_extent, transform.get_world_rotation());

            // Releasing the mouse button to modify the area.
            if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left)) break;

            modify_voxel_volume(model, brush_state.mode, min, max, material_index);
            break;
        }

        default:
            break;
    }
}

void use_gizmo(const NodeHierarchy& hierarchy, const ImVec2& window_pos, const ImVec2& window_size) {
    const std::vector<Entity>& selected_entities = hierarchy.get_selected_entities();

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

    const bool is_using_camera = editor.systems[Editor::Mode::SCENE].get<Viewport>().is_using_debug_camera();
    const bool wants_to_capture_keyboard = ImGui::GetIO().WantCaptureKeyboard;
    if (wants_to_capture_keyboard == false && !is_using_camera) {
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

    editor.gizmo.manip(window_pos.x, window_pos.y, window_size.x, window_size.y, selected_entities, snap_value, modifiers);
}

}  // namespace

void ModelViewer::on_inspect() {
    const ImVec2 content_start { 0.0f, ImGui::GetFrameHeight() };
    ImGui::SetCursorPos(content_start);

    const ImVec2 size = ImGui::GetWindowSize() - content_start;
    if (size.x <= 0.0f || size.y <= 0.0f) return;

    const ImVec2 window_pos = ImGui::GetWindowPos() + ImVec2 { 0.0f, ImGui::GetFrameHeight() };

    engine.renderer.render_view.set_viewport_size(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
    ImGui::Image(engine.renderer.render_view.imgui_viewport, size);

    const Brush::State brush_state = editor.systems[Editor::Mode::VOXEL].get<Brush>().get_brush_state();
    const bool is_tool_gizmo = brush_state.tool == Brush::Tool::GIZMO;
    if (is_tool_gizmo) {
        NodeHierarchy& hierarchy = editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>();

        use_gizmo(hierarchy, window_pos, size);

        const bool can_select = (ImGui::IsWindowHovered() && !ImGuizmo::IsOver());
        if (can_select && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const Ray mouse_ray = engine.renderer.render_view.pixel_ray(mouse_position);
            const Hit hit = engine.renderer.trace_ray(mouse_ray);

            if (!ImGui::GetIO().KeyCtrl) {
                if (hit.miss())
                    hierarchy.clear_selected_entities();
                else
                    hierarchy.set_selected_entity(hit.entity);
            } else {
                if (!hierarchy.is_entity_selected(hit.entity))
                    editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().add_selected_entity(hit.entity);
                else
                    editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>().remove_selected_entity(hit.entity);
            }
        }
    }

    // Set up the window and mouse variables in the viewport to update the camera movement in the update function later.
    Viewport& viewport = editor.systems[Editor::Mode::SCENE].get<Viewport>();
    viewport.is_hovered = ImGui::IsItemHovered();
    const ImVec2 mouse_pos = ImGui::GetMousePos() - ImGui::GetWindowPos();
    mouse_position.x = viewport.mouse_pos.x = mouse_pos.x - content_start.x;
    mouse_position.y = viewport.mouse_pos.y = mouse_pos.y - content_start.y;

    // Get the selected entity in the hierarchy and return early if it's invalid.
    NodeHierarchy& node_hierarchy = editor.systems[Editor::Mode::VOXEL].get<NodeHierarchy>();
    const Entity selected_entity = node_hierarchy.get_first_selected_entity();

    // Draw bounding boxes of all selected entities.
    for (const Entity e : node_hierarchy.get_selected_entities()) {
        if (e == selected_entity || !engine.ecs.has_component<VoxelRenderer>(e)) continue;

        const Transform& transform = engine.ecs.get_component<Transform>(e);
        const ResourceRef<VoxelVolume>& resource = engine.ecs.get_component<VoxelRenderer>(e).resource;
        const glm::vec3 half_extent = glm::vec3 { resource->size } * VOXEL_SIZE_HALF;
        draw_valid_faces(transform, half_extent, colors::SELECTED);
    }

    if (!engine.ecs.valid(selected_entity) || !engine.ecs.has_component<VoxelRenderer>(selected_entity)) return;

    // Get the necessary values for handling selection (entity transform, voxel resource, ray cast for selection, etc).
    const Transform& transform = engine.ecs.get_component<Transform>(selected_entity);
    const ResourceRef<VoxelVolume>& resource = engine.ecs.get_component<VoxelRenderer>(selected_entity).resource;
    const glm::vec3 half_extent = glm::vec3 { resource->size } * VOXEL_SIZE_HALF;

    const Ray mouse_ray = engine.renderer.render_view.pixel_ray(mouse_position);
    Hit hit = engine.renderer.trace_ray(mouse_ray);

    if (glm::any(glm::greaterThanEqual(hit.coord, resource->size))) hit.entity = entt::null;

    const std::vector<FaceData> valid_faces = draw_valid_faces(transform, half_extent, colors::SELECTED);

    // Don't calculate voxel hits when the cursor isn't over the window or using gizmo.
    if (!ImGui::IsWindowHovered() || is_tool_gizmo) return;

    // Return early if no valid voxel was selected.
    if (!handle_selection(valid_faces, brush_state, mouse_ray, hit, selected_entity, transform, half_extent)) return;

    const MaterialIndex material_index = editor.systems[Editor::Mode::VOXEL].get<Palette>().get_selected_material_indices().front();
    const Material& material = resource->blas->palette.entries[material_index];
    draw_selection(hit, half_extent, transform, material.albedo.unpack());

    handle_brush(brush_state, resource, hit, material_index);
}

void ModelViewer::on_editor_update(const FrameData& time) {
    editor.systems[Editor::Mode::SCENE].get<Viewport>().update_debug_camera(time);
}

}  // namespace tmt