#pragma once
#include "engine/core/resource.hpp"
#include "engine/core/resources/json.hpp"

// variable radius poisson disc walking to achieve poisson distribution
namespace tmt {

struct PoissonPoint {
    glm::vec3 pos;
    int entry_idx;
    float radius;
};

struct LayerEntry {
    std::vector<tmt::ResourceRef<tmt::Json>> spawnables;

    bool can_spawn_lights = false;
    float light_spawn_chance = 0.1f;

    float radius_factor = 1.f;
    float spawn_weight = 1.f;
    float belt_bias = 1.f;
};

struct PoissonField {
    std::vector<LayerEntry> layer_entries;

    int rejection_samples = 20;

    float spacing_radius = 50.f;

    glm::vec3 size;
};

struct SpatialLookupGrid {
    SpatialLookupGrid(float cell_size, glm::vec3 region_size);

    std::vector<int> grid;
    int x_dim;
    int y_dim;
    int z_dim;

    float cell_size;
    glm::vec3 region_size;

    int& operator[](int idx) { return grid[idx]; }
    int operator[](int idx) const { return grid[idx]; }
};
int get_entry_idx(const PoissonField& layer, float cum_spawn_weight);
std::vector<PoissonPoint> get_poisson_points(const PoissonField& layer);
bool point_valid(glm::vec3 point, float radius, float max_radius, const SpatialLookupGrid& grid, const std::vector<PoissonPoint>& points);

}  // namespace tmt
TMT_OBJECT(tmt::LayerEntry, (spawnables, radius_factor, spawn_weight, belt_bias, can_spawn_lights, light_spawn_chance));
TMT_OBJECT(tmt::PoissonField, (layer_entries, rejection_samples, spacing_radius));
