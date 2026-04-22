#pragma once
#include "editor/core/window.hpp"
#include "engine/systems/gameplay/level_config.hpp"

struct ImDrawList;

namespace tmt {

class LevelEditor : public IWindow<> {
   public:
    // Inherited via IWindow
    void on_inspect() override;

    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;

    constexpr std::string get_title() const override { return "Level Editor"; };

    std::unordered_map<glm::ivec2, Cell>& get_cells();

   private:
    BEFRIEND_VISITABLE();

    struct ViewportContext {
        float width;
        float height;
        glm::vec2 p;
        glm::mat4 view_project;
    } viewport_context;

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

    LevelConfiguration level_configuration;
};

}  // namespace tmt