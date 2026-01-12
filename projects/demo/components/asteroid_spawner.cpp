#include "asteroid_spawner.hpp"

#include <cstdlib>
#include <cmath>
#include <random>

#include "engine/core/components/voxel_renderer.hpp"

// Golden ratio constant for optimal spacing
constexpr float PHI = 1.618033988749895f;
constexpr float GOLDEN_ANGLE = 2.39996322972865332f;  // 2π / φ (in radians)

void AsteroidSpawner::start() { spawn_asteroids(); }

void AsteroidSpawner::update(const tmt::FrameData& time) {}

void AsteroidSpawner::end() {}

void AsteroidSpawner::spawn_asteroids() {
    // Initialize random seed for consistent results
    if (randomize_seed) {
        srand(time(nullptr));
    } else {
        srand(seed);
    }

    // Validate parameters
    if (asteroid_bodies.empty()) {
        // Log warning: No asteroid bodies loaded
        return;
    }

    if (min_radius >= max_radius) {
        // Log warning: min_radius must be less than max_radius
        return;
    }

    if (asteroid_count <= 0) {
        return;
    }

    // Spawn asteroids using golden ratio distribution
    for (int i = 0; i < asteroid_count; ++i) {
        glm::vec3 position = calculate_position(i, asteroid_count);
        glm::vec3 rotation = enable_random_rotation ? random_rotation() : glm::vec3(0.0f);
        float scale = random_float(min_scale_factor, max_scale_factor);

        const tmt::Entity entity = create_asteroid(position, rotation, scale);
        spawned_asteroids.push_back(entity);
    }
}

#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
void AsteroidSpawner::on_inspect(ImResponse& response) {
    const bool changed = response.get<AsteroidSpawner>().is_changed();

    if (changed) {
        for (auto& entity : spawned_asteroids) {
            tmt::engine.ecs.destroy_entity(entity, true);
        }
        spawned_asteroids.clear();

        spawn_asteroids();
    }
}
#else
void AsteroidSpawner::on_inspect(ImResponse& response) {}
#endif

glm::vec3 AsteroidSpawner::calculate_position(int index, int total) {
    const float theta = index * GOLDEN_ANGLE;
    const float normalized_index = static_cast<float>(index) / static_cast<float>(total);
    float bias_factor = normalized_index;
    if (radial_bias != 0.0f) {
        bias_factor = std::pow(normalized_index, 1.0f - radial_bias);
    }

    const float radius = std::sqrt(bias_factor * (max_radius * max_radius - min_radius * min_radius) + min_radius * min_radius);

    float x = radius * std::cos(theta);
    float z = radius * std::sin(theta);

    float y = 0.0f;
    if (use_3d_distribution) {
        y = radius * std::cos(PHI);
        x = radius * std::sin(PHI) * std::cos(theta);
        z = radius * std::sin(PHI) * std::sin(theta);
    } else {
        y = random_float(min_height_offset, max_height_offset);
    }

    return glm::vec3(x, y, z);
}

tmt::Entity AsteroidSpawner::create_asteroid(const glm::vec3& position, const glm::vec3& rotation, float scale) {
    const tmt::Entity new_entity = tmt::engine.ecs.create_entity();

    // Add voxel renderer with random asteroid body
    tmt::VoxelRenderer& renderer = tmt::engine.ecs.add_component<tmt::VoxelRenderer>(new_entity);
    int random_index = rand() % asteroid_bodies.size();
    const auto& asteroid_body = asteroid_bodies.at(random_index);
    renderer.resource = asteroid_body;

    // Set transform properties
    tmt::Transform& transform = tmt::engine.ecs.get_component<tmt::Transform>(new_entity);
    transform.set_world_position(position);
    transform.set_world_rotation(rotation);
    transform.set_world_scale(glm::vec3(scale));
    transform.set_parent(entity);

    return new_entity;
}

float AsteroidSpawner::random_float(float min, float max) {
    float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return min + random * (max - min);
}

glm::vec3 AsteroidSpawner::random_rotation() { return glm::vec3(random_float(0.0f, 360.0f), random_float(0.0f, 360.0f), random_float(0.0f, 360.0f)); }