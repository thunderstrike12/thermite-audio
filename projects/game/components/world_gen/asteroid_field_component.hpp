#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/voxel_volume.hpp"

namespace game {

struct AsteroidPoissonPoint {
    glm::vec3 pos;
    int entry_idx;
    float radius;
};

struct FAsteroidLayerEntry {
    tmt::ResourceRef<tmt::Json> spawnable;

    float radius_factor = 1.f;
    float spawn_weight = 1.f;
    float belt_bias = 1.f;
};

struct FAsteroidFieldLayer {
    std::vector<FAsteroidLayerEntry> layer_entries;

    int rejection_samples = 20;

    float spacing_radius = 50.f;

    float space_weight = 1.f;

    float layer_spacing = 0.f;

    float layer_depth;
};

struct AsteroidFieldComponent : public tmt::GameComponent<AsteroidFieldComponent> {
    using GameComponent::GameComponent;

    unsigned int seed = 42;
    float field_depth = 1000.f;
    float layer_width = 100.f;
    float layer_height = 100.f;
    bool draw_debug = false;

    std::vector<FAsteroidFieldLayer> asteroid_layers;

    void spawn_field();
    void clear_children();

    static std::string_view get_name() { return "Asteroid Field Component"; }

   private:
    std::vector<AsteroidPoissonPoint> get_poisson_points(const FAsteroidFieldLayer& layer);

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

    bool point_valid(glm::vec3 point, float radius, float max_radius, const SpatialLookupGrid& grid, const std::vector<AsteroidPoissonPoint>& points);

    // Inherited via GameComponent
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    // Inherited via OnEngineUpdate
    void draw_debug_lines() const override;
};

}  // namespace game
TMT_OBJECT(game::FAsteroidLayerEntry, (spawnable, radius_factor, spawn_weight, belt_bias));
TMT_OBJECT(game::FAsteroidFieldLayer, (layer_entries, rejection_samples, spacing_radius, space_weight, layer_spacing));
TMT_OBJECT(game::AsteroidFieldComponent, (seed, field_depth, layer_width, layer_height, asteroid_layers, draw_debug));
