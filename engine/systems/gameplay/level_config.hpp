#pragma once
#include <glm/glm.hpp>
#include "engine/tools/poisson_disc_walk.hpp"
#include "engine/tools/types/color.hpp"
#include <functional>

template <glm::qualifier Q>
struct std::hash<glm::vec<2, int, Q>> {
    size_t operator()(const glm::vec<2, int, Q>& v) const noexcept {
        size_t seed = std::hash<int> {}(v.x);
        seed ^= std::hash<int> {}(v.y) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
        return seed;
    }
};

namespace tmt {

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

    std::vector<RGBA> possible_light_colors;
};

struct LevelConfiguration {
    std::unordered_map<glm::ivec2, Cell> scene_cells;
    std::vector<CellTemplate> scene_pickable_cell_templates;

    float cell_size;
    float cell_margin;
    glm::vec3 global_field_offset;
};

}  // namespace tmt
TMT_OBJECT(tmt::CellTemplate, (name, random_seed, seed, height, field, color, possible_light_colors));
TMT_OBJECT(tmt::Cell, (template_index));
TMT_OBJECT(tmt::LevelConfiguration, (cell_size, cell_margin, global_field_offset, scene_cells, scene_pickable_cell_templates));
