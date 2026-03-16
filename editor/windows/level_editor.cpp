#include "level_editor.hpp"

#include "editor/editor.hpp"
#include "editor/windows/viewport.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/engine.hpp"
#include "imgui.h"
#include "engine/core/polyline.hpp"
#include "engine/tools/random.hpp"
#include "engine/core/logger.hpp"
#include "engine/tools/prefab_helper.hpp"
#include "engine/core/scenes.hpp"
#include "editor/core/systems/undo_redo/level_editor_diff.hpp"
#include "editor/imgui/types/all.hpp"
#include <ImReflect.hpp>

namespace tmt {

void tmt::LevelEditor::on_inspect() {
    const std::string& scene_name = engine.scenes.get_active_scene_info().name;
    auto& pickable_cell_templates = scene_pickable_cell_templates[scene_name];
    auto& cells = scene_cells[scene_name];

    window_displayed = true;

    auto& viewport = editor.systems[Editor::Mode::SCENE].get<Viewport>();

    ImGui::Checkbox("Enable level editing", &enable_editing);
    ImGui::Checkbox("Mirror X", &mirror_x);
    ImGui::DragFloat("Cell size", &cell_size);
    ImGui::DragFloat3("Global field offset", &global_field_offset[0]);

    static const char* brush_previews[static_cast<int>(Brush::MAX)] = { "PLACE", "REMOVE", "REPLACE" };
    if (ImGui::BeginCombo("Brush Mode", brush_previews[selected_brush_mode])) {
        for (int brush = 0; brush < static_cast<int>(Brush::MAX); brush++) {
            const bool is_selected = (selected_brush_mode == brush);

            if (ImGui::Selectable(brush_previews[brush], is_selected)) {
                selected_brush_mode = brush;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // cell template picker
    static const char* preview = "None";
    if (selected_template >= 0 && selected_template < static_cast<int>(pickable_cell_templates.size())) {
        preview = pickable_cell_templates[selected_template].name.c_str();
    } else
        preview = "None";

    if (ImGui::BeginCombo("Cell Template", preview)) {
        for (int i = 0; i < static_cast<int>(pickable_cell_templates.size()); ++i) {
            const bool is_selected = (selected_template == i);

            if (ImGui::Selectable(pickable_cell_templates[i].name.c_str(), is_selected)) {
                selected_template = i;
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    auto response = ImReflect::Input("Cell templates", &pickable_cell_templates);
    if (response.get<std::vector>().has_erased()) {
        size_t erased_idx = response.get<std::vector>().get_erased_index();
        if (erased_idx == selected_template) selected_template = -1;

        std::vector<glm::ivec2> erasing_coords;
        for (auto& [coord, cell] : cells) {
            if (cell.template_index == erased_idx) {
                erasing_coords.push_back(coord);
            }
        }
        for (auto& coord : erasing_coords) {
            cells.erase(coord);
        }
    }

    if (ImGui::Button("Generate cells")) {
        auto view = engine.ecs.view<LevelCellGrouperTag>();
        tmt::Entity grouper_entity;
        if (view.begin() == view.end()) {
            grouper_entity = engine.ecs.create_entity("Level Cell Objects");
            engine.ecs.add_component<LevelCellGrouperTag>(grouper_entity);
        } else {
            grouper_entity = view.front();
        }

        auto& grouper_trans = engine.ecs.get_component<Transform>(grouper_entity);
        if (grouper_trans.has_children()) {
            for (auto ent : grouper_trans.get_children()) {
                engine.ecs.destroy_entity(ent);
            }
        }

        for (auto& cell_template : pickable_cell_templates) {
            cell_template.field.size = { cell_size, cell_template.height, cell_size };
        }

        for (auto& [coord, cell] : cells) {
            auto& cell_template = pickable_cell_templates[cell.template_index];

            cell_template.field.size = { cell_size, cell_template.height, cell_size };

            if (cell_template.random_seed)
                Random::set_seed(Random::irand());
            else
                Random::set_seed(cell_template.seed);

            auto local_points = tmt::get_poisson_points(cell_template.field);

            glm::vec3 cell_pos = coord_to_world(coord);

            // spawning
            for (auto& point : local_points) {
                auto spawn_obj = cell_template.field.layer_entries[point.entry_idx].spawnable;
                // float size_factor = cell_template.field.layer_entries[point.entry_idx].radius_factor;

                float pitch = Random::rand_range(0.f, 360.f);
                float yaw = Random::rand_range(0.f, 360.f);
                float roll = Random::rand_range(0.f, 360.f);

                auto instantiated = tmt::PrefabHelper::instantiate_prefab(spawn_obj->file_location, grouper_entity);

                auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(instantiated);

                glm::vec3 min = cell_pos - glm::vec3(cell_size * 0.5f, cell_template.height * 0.5f, cell_size * 0.5f);

                transform.set_local_position(min + point.pos);
                transform.set_local_rotation(glm::vec3(pitch, yaw, roll));
            }
        }
    }

    ImDrawList* draw_list = viewport.get_drawlist();

    if (enable_editing) {
        for (auto [coord, cell] : cells) {
            draw_cell(draw_list, coord_to_world(coord), cell, pickable_cell_templates);
        }

        const float max_alpha = 0.2f;
        const float diff_amplitude = 0.15f;
        float alpha = max_alpha - diff_amplitude * (glm::sin(highlight_timer * 2.f) + 1.f) * 0.5f;

        if (viewport.get_is_hovered() && selected_template != -1) {
            for (const auto& coord : placing_coords) {
                draw_cell(draw_list, coord_to_world(coord), Cell { .template_index = selected_template }, pickable_cell_templates, glm::vec4(1.f, 1.f, 1.f, alpha));
            }

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                LevelEditorDiff level_editor_diff;

                switch (selected_brush_mode) {
                    case static_cast<int>(Brush::PLACE):
                        for (const auto& coord : placing_coords) {
                            auto pair = cells.try_emplace(coord, Cell { .template_index = selected_template });
                            if (pair.second) {
                                level_editor_diff.added_placements.emplace(coord, pair.first->second);
                            }
                        }
                        break;
                    case static_cast<int>(Brush::REMOVE):
                        for (const auto& coord : placing_coords) {
                            if (cells.contains(coord)) {
                                Cell erased_cell = cells[coord];

                                cells.erase(coord);
                                level_editor_diff.removed_placements.emplace(coord, erased_cell);
                            }
                        }
                        break;
                    case static_cast<int>(Brush::REPLACE):
                        for (const auto& coord : placing_coords) {
                            if (cells.contains(coord) && cells[coord].template_index != selected_template) {
                                Cell prev = cells[coord];
                                cells[coord] = Cell { .template_index = selected_template };

                                level_editor_diff.replaced_placements.emplace(coord, std::make_pair(prev, cells[coord]));
                            }
                        }
                        break;
                }
                // auto& cell_template = pickable_cell_templates[selected_template];

                level_editor_diff.commit("");
            }
        }
    }
}

void tmt::LevelEditor::on_editor_start() {};

void tmt::LevelEditor::on_editor_update(const tmt::FrameData& time) {
    auto& viewport = editor.systems[Editor::Mode::SCENE].get<Viewport>();
    viewport.set_allow_selection((!enable_editing) || (!window_displayed));
    window_displayed = false;

    if (enable_editing) {
        auto& camera = engine.renderer.get_debug_camera();
        auto& camera_transform = engine.renderer.get_debug_transform();

        auto view = glm::inverse(camera_transform.get_world_matrix());
        float aspect = viewport.get_width() / viewport.get_height();
        auto project = glm::perspective(glm::radians(camera.fov), aspect, 0.01f, 1000.f);
        auto mouse_pos = viewport.get_mouse_pos();

        viewport_context.height = viewport.get_height();
        viewport_context.width = viewport.get_width();
        viewport_context.p = viewport.get_window_pos();
        viewport_context.view_project = project * view;

        glm::vec4 viewport4 { 0.f, 0.f, viewport.get_width(), viewport.get_height() };

        glm::vec3 near = { mouse_pos.x, viewport_context.height - mouse_pos.y, 0.0f };
        glm::vec3 far { mouse_pos.x, viewport_context.height - mouse_pos.y, 1.f };

        glm::vec3 ray_origin = glm::unProject(near, view, project, viewport4);
        glm::vec3 ray_far = glm::unProject(far, view, project, viewport4);
        glm::vec3 ray_dir = glm::normalize(ray_far - ray_origin);

        float t = -(ray_origin.y / ray_dir.y);

        placing_coords.clear();
        if (t > 0.f) {
            // possible xz plane intersection
            place_xz_pos = ray_origin + ray_dir * t;
            placing_coords.push_back(world_to_coord(place_xz_pos));

            if (mirror_x) {
                glm::vec3 place_xz_mirror = { -place_xz_pos.x, 0.f, place_xz_pos.z };
                placing_coords.push_back(world_to_coord(place_xz_mirror));
            }
        }

        highlight_timer += time.delta_time;
    }
}

void LevelEditor::draw_cell(ImDrawList* draw_list, glm::vec3 pos, const Cell& cell, const std::vector<CellTemplate>& pickable_cell_templates, glm::vec4 col) {
    float extent = cell_size * 0.5f;
    float height_extent = pickable_cell_templates[cell.template_index].height * 0.5f;
    glm::vec3 local_vertices[8] = { glm::vec3(-extent, -height_extent, -extent), glm::vec3(extent, -height_extent, -extent), glm::vec3(extent, height_extent, -extent),
                                    glm::vec3(-extent, height_extent, -extent),  glm::vec3(-extent, -height_extent, extent), glm::vec3(extent, -height_extent, extent),
                                    glm::vec3(extent, height_extent, extent),    glm::vec3(-extent, height_extent, extent) };

    ImVec2 screen_vertices[8];
    for (int i = 0; i < 8; i++) {
        glm::vec4 p = viewport_context.view_project * glm::vec4(local_vertices[i] + pos, 1.f);

        if (p.w < 0.f) {
            return;
        }

        p *= 0.5f / p.w;
        p += glm::vec4(0.5f, 0.5f, 0.f, 0.f);
        p.y = 1.f - p.y;
        p.x *= viewport_context.width;
        p.y *= viewport_context.height;
        p.x += viewport_context.p.x;
        p.y += viewport_context.p.y;

        screen_vertices[i] = { p.x, p.y };
    }

    // 6 faces of the cube
    static const int faces[6][4] = { { 0, 1, 2, 3 }, { 5, 4, 7, 6 }, { 4, 0, 3, 7 }, { 1, 5, 6, 2 }, { 3, 2, 6, 7 }, { 4, 5, 1, 0 } };
    glm::vec4 fill_color;
    glm::vec4 edge_color;
    if (glm::dot(col, col) > 0.01f) {
        fill_color = col;
        edge_color = { col.r, col.g, col.b, 1.f };
    } else {
        fill_color = pickable_cell_templates[cell.template_index].color.get() - glm::vec4(0.2f);
        edge_color = pickable_cell_templates[cell.template_index].color.get();
    }

    ImU32 pack_col = ImGui::ColorConvertFloat4ToU32({ fill_color.r, fill_color.g, fill_color.b, fill_color.a });

    for (int i = 0; i < 6; i++) {
        ImVec2 face_quad[4];
        for (int j = 0; j < 4; j++) {
            face_quad[j] = screen_vertices[faces[i][j]];
        }
        draw_list->AddConvexPolyFilled(face_quad, 4, pack_col);
    }

    engine.polyline.use_color(edge_color);
    engine.polyline.use_line_width(5.f);
    engine.polyline.draw_aabb(pos - glm::vec3(extent, height_extent, extent), { pos + glm::vec3(extent, height_extent, extent) });
}

glm::vec3 LevelEditor::coord_to_world(glm::ivec2 coord) {
    return global_field_offset + glm::vec3(coord.x * cell_size + cell_size * 0.5f, 0.f, coord.y * cell_size + cell_size * 0.5f);
}

glm::ivec2 LevelEditor::world_to_coord(glm::vec3 pos) {
    pos -= global_field_offset;
    return glm::ivec2(glm::floor(pos.x / cell_size), glm::floor(pos.z / cell_size));
}

void tmt::LevelEditor::on_editor_end() {}

std::unordered_map<glm::ivec2, LevelEditor::Cell>& LevelEditor::get_cells() {
    const std::string& scene_name = engine.scenes.get_active_scene_info().name;
    return scene_cells[scene_name];
}

}  // namespace tmt
