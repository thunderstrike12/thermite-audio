#include "asteroid_field_component.hpp"
#include "engine/tools/random.hpp"
#include "engine/tools/prefab_helper.hpp"

#include "engine/engine.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/ecs.hpp"

namespace game {

std::vector<AsteroidPoissonPoint> AsteroidFieldComponent::get_poisson_points(const FAsteroidFieldLayer& layer) {
    std::vector<AsteroidPoissonPoint> radii_points;

    if (layer.layer_entries.empty()) return radii_points;

    float cell_size = layer.spacing_radius / sqrtf(3.f);

    glm::vec3 region_size = glm::vec3(layer_width, layer_height, layer.layer_depth);

    SpatialLookupGrid grid(cell_size, region_size);

    float max_size_factor = 0.f;
    for (auto& layer : layer.layer_entries) {
        if (layer.radius_factor > max_size_factor) max_size_factor = layer.radius_factor;
    }
    float max_radius = layer.spacing_radius * max_size_factor;

    // assign initial point in this layer
    float x0 = Random::rand_range(0.f, grid.region_size.x);
    float y0 = Random::rand_range(0.f, grid.region_size.y);
    float z0 = Random::rand_range(0.f, grid.region_size.z);

    int x = x0 / grid.cell_size;
    int y = y0 / grid.cell_size;
    int z = z0 / grid.cell_size;

    int init_idx = x * grid.y_dim * grid.z_dim + y * grid.z_dim + z;

    int init_entry_idx = Random::rand_range(0, layer.layer_entries.size() - 1);

    AsteroidPoissonPoint init_point;
    init_point.radius = layer.spacing_radius * layer.layer_entries[init_entry_idx].radius_factor;
    init_point.pos = { x0, y0, z0 };
    init_point.entry_idx = init_entry_idx;
    radii_points.push_back(init_point);

    grid.grid[init_idx] = 0;

    // Robert-Bridson disc sampling
    std::vector<int> active_spawn_list;
    active_spawn_list.push_back(0);

    float cum_spawn_weight = 0.f;
    float cum_belt_weight = 0.f;
    std::for_each(layer.layer_entries.begin(), layer.layer_entries.end(), [&cum_belt_weight, &cum_spawn_weight](const FAsteroidLayerEntry& entry) {
        cum_belt_weight += entry.belt_bias;
        cum_spawn_weight += entry.spawn_weight;
    });

    while (!active_spawn_list.empty()) {
        bool found_candidate = false;

        int idx = Random::rand_range(0, active_spawn_list.size() - 1);
        glm::vec3 selected_p = radii_points[active_spawn_list[idx]].pos;

        for (int i = 0; i < layer.rejection_samples; i++) {
            float rand_weight = Random::rand_range(0.f, cum_spawn_weight);
            float acc = 0.f;
            int entry_idx = std::numeric_limits<int>::max();
            for (int i = 0; i < layer.layer_entries.size(); i++) {
                acc += layer.layer_entries[i].spawn_weight;

                if (rand_weight < acc) {
                    entry_idx = i;
                    break;
                }
            }

            glm::vec3 rand_unit_vec = Random::rand_unit_vec();

            float linear_relation = (selected_p.y / (layer_height * 0.5f)) - 1.f;
            float belt_tendency = linear_relation;

            rand_unit_vec.y += -belt_tendency * layer.layer_entries[entry_idx].belt_bias;
            rand_unit_vec = glm::normalize(rand_unit_vec);

            float selected_spacing_radius = radii_points[active_spawn_list[idx]].radius;
            float candidate_radius = layer.layer_entries[entry_idx].radius_factor * layer.spacing_radius;
            glm::vec3 candidate_point = selected_p + rand_unit_vec * Random::rand_range(selected_spacing_radius + candidate_radius, 2.f * (selected_spacing_radius + candidate_radius));

            if (point_valid(candidate_point, candidate_radius, max_radius, grid, radii_points)) {
                AsteroidPoissonPoint a_p;
                a_p.entry_idx = entry_idx;
                a_p.pos = candidate_point;
                a_p.radius = candidate_radius;

                radii_points.push_back(a_p);
                active_spawn_list.push_back(radii_points.size() - 1);

                // candidate_point += region_size * 0.5f;

                int cell_x = static_cast<int>(candidate_point.x / cell_size);
                int cell_y = static_cast<int>(candidate_point.y / cell_size);
                int cell_z = static_cast<int>(candidate_point.z / cell_size);

                int idx = cell_x * grid.y_dim * grid.z_dim + cell_y * grid.z_dim + cell_z;
                grid[idx] = radii_points.size() - 1;

                found_candidate = true;
                break;
            }
        }

        if (!found_candidate) {
            active_spawn_list.erase(active_spawn_list.begin() + idx);
        }
    }

    return radii_points;
}

bool AsteroidFieldComponent::point_valid(glm::vec3 candidate, float candidate_radius, float max_radius, const SpatialLookupGrid& grid, const std::vector<AsteroidPoissonPoint>& points) {
    // candidate += grid.region_size * 0.5f;

    if (candidate.x < 0.f || candidate.x > grid.region_size.x || candidate.y < 0.f || candidate.y > grid.region_size.y || candidate.z < 0.f || candidate.z > grid.region_size.z) {
        return false;
    }

    const int cell_x = static_cast<int>(candidate.x / grid.cell_size);
    const int cell_y = static_cast<int>(candidate.y / grid.cell_size);
    const int cell_z = static_cast<int>(candidate.z / grid.cell_size);

    const int search_dim = std::ceil(max_radius / grid.cell_size);

    const int start_search_x = std::max(cell_x - search_dim, 0);
    const int end_search_x = std::min(cell_x + search_dim + 1, grid.x_dim);
    const int start_search_y = std::max(cell_y - search_dim, 0);
    const int end_search_y = std::min(cell_y + search_dim + 1, grid.y_dim);
    const int start_search_z = std::max(cell_z - search_dim, 0);
    const int end_search_z = std::min(cell_z + search_dim + 1, grid.z_dim);

    for (int x = start_search_x; x < end_search_x; x++) {
        for (int y = start_search_y; y < end_search_y; y++) {
            for (int z = start_search_z; z < end_search_z; z++) {
                int idx = x * grid.y_dim * grid.z_dim + y * grid.z_dim + z;
                int existing_point_index = grid[idx];

                if (existing_point_index != -1) {
                    glm::vec3 temp = points[existing_point_index].pos - candidate;
                    float sqr_distance = glm::dot(temp, temp);

                    float min_allowed = candidate_radius + points[existing_point_index].radius;

                    if (sqr_distance < min_allowed * min_allowed) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void AsteroidFieldComponent::spawn_field() {
    if (asteroid_layers.empty()) return;

    Random::set_seed(seed);

    float cum_weight = 0.f;
    for (int i = 0; i < asteroid_layers.size(); i++) {
        cum_weight += asteroid_layers[i].space_weight;
    }

    for (int i = 0; i < asteroid_layers.size(); i++) {
        auto& zero_layer = asteroid_layers[i];
        zero_layer.layer_depth = field_depth * (zero_layer.space_weight / cum_weight);
    }

    // poisson distribution sampling
    unsigned int it = 0;
    for (auto& layer : asteroid_layers) {
        std::vector<AsteroidPoissonPoint> radii_points = get_poisson_points(layer);

        for (auto& point : radii_points) {
            auto spawn_obj = layer.layer_entries[point.entry_idx].spawnable;
            float size_factor = layer.layer_entries[point.entry_idx].radius_factor;

            float pitch = Random::rand_range(0.f, 360.f);
            float yaw = Random::rand_range(0.f, 360.f);
            float roll = Random::rand_range(0.f, 360.f);

            auto instantiated = tmt::PrefabHelper::instantiate_prefab(spawn_obj->file_location, entity);

            auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(instantiated);

            transform.set_local_position(point.pos + glm::vec3(0.f, 0.f, it * layer.layer_depth));
            transform.set_local_rotation(glm::vec3(pitch, yaw, roll));
        }
        ++it;
    }
}

void AsteroidFieldComponent::clear_children() {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    for (auto child : transform.get_all_children()) {
        tmt::engine.ecs.destroy_entity(child);
    }
}

void AsteroidFieldComponent::start() {}

void AsteroidFieldComponent::update(const tmt::FrameData& time) {}

void AsteroidFieldComponent::end() {}

void AsteroidFieldComponent::draw_debug_lines() const {
    if (!draw_debug) return;

    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    glm::vec3 min = transform.get_world_position();
    glm::vec3 max = min + glm::vec3(layer_width, layer_height, field_depth);
    tmt::engine.polyline.use_color({ 1.f, 1.f, 1.f, 1.f });
    tmt::engine.polyline.draw_aabb(min, max);

    unsigned int it = 1;
    for (auto& layer : asteroid_layers) {
        glm::vec3 layer_min = { min.x, min.y, min.z + it * layer.layer_depth };
        glm::vec3 layer_max = { max.x, max.y, min.z + it * layer.layer_depth };
        glm::vec3 layer_bmin = { min.x, max.y, min.z + it * layer.layer_depth };
        glm::vec3 layer_bmax = { max.x, min.y, min.z + it * layer.layer_depth };

        tmt::engine.polyline.draw_line(layer_min, layer_max);
        tmt::engine.polyline.draw_line(layer_bmin, layer_bmax);

        ++it;
    }
}

AsteroidFieldComponent::SpatialLookupGrid::SpatialLookupGrid(float _cell_size, glm::vec3 _region_size) {
    region_size = _region_size;
    cell_size = _cell_size;

    x_dim = std::ceil(region_size.x / cell_size);
    y_dim = std::ceil(region_size.y / cell_size);
    z_dim = std::ceil(region_size.z / cell_size);

    int num_grid = x_dim * y_dim * z_dim;
    grid = std::vector<int>(num_grid, -1);
}

}  // namespace game
