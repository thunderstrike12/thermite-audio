#pragma once
#include "editor/core/window.hpp"
#include "engine/tools/poisson_disc_walk.hpp"
#include "engine/tools/types/color.hpp"
struct ImDrawList;

namespace std {

template <>
struct hash<glm::ivec2> {
    std::size_t operator()(const glm::ivec2& v) const noexcept {
        std::size_t h1 = std::hash<int> {}(v.x);
        std::size_t h2 = std::hash<int> {}(v.y);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

}  // namespace std

namespace tmt {

class LevelEditor : public IWindow<LevelEditor> {
   public:
    // Inherited via IWindow
    void on_inspect() override;

    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;

    constexpr std::string get_title() const override { return "Level Editor"; };

    struct Cell {
        int template_index;
    };
    struct CellTemplate {
        int seed = 42;
        bool random_seed;
        std::string name = "default";

        float height = 10.f;
        PoissonField field;
        RGBA color { glm::vec4(1.f, 0.f, 0.f, 1.f) };
    };

    std::unordered_map<glm::ivec2, Cell>& get_cells();

   private:
    BEFRIEND_VISITABLE();

    struct ViewportContext {
        float width;
        float height;
        glm::vec2 p;
        glm::mat4 view_project;
    } viewport_context;

    struct LevelCellGrouperTag {};

    std::unordered_map<std::string, std::vector<CellTemplate>> scene_pickable_cell_templates;
    int selected_template = -1;

    enum class Brush { PLACE, REMOVE, REPLACE, MAX };

    int selected_brush_mode = 0;

    bool window_displayed = false;

    void draw_cell(ImDrawList* draw_list, glm::vec3 pos, const Cell& cell, const std::vector<CellTemplate>& pickable_cell_templates, glm::vec4 col = glm::vec4(0.f));
    glm::vec3 coord_to_world(glm::ivec2 coord);
    glm::ivec2 world_to_coord(glm::vec3 coord);

    glm::vec3 place_xz_pos;
    float highlight_timer = 0.f;
    std::vector<glm::ivec2> placing_coords;

    // user editable
    bool enable_editing = false;
    bool mirror_x = false;
    float cell_size;
    glm::vec3 global_field_offset;
    std::unordered_map<std::string, std::unordered_map<glm::ivec2, Cell>> scene_cells;
};

}  // namespace tmt
TMT_OBJECT(tmt::LevelEditor::CellTemplate, (name, random_seed, seed, height, field, color));
TMT_OBJECT(tmt::LevelEditor::Cell, (template_index));
TMT_OBJECT(tmt::LevelEditor, (cell_size, global_field_offset, scene_cells, scene_pickable_cell_templates));